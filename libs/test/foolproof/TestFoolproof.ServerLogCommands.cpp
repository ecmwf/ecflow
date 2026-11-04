// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Returns the configuration file of this host for the given port.
///
/// @param[in] directory The start-up directory of the server
/// @param[in] port The port of the server
/// @return The path of `<host>.<port>.ecf.cfg` in the start-up directory.
///
fs::path configuration_file(const fs::path& directory, std::uint32_t port) {
    return directory / (ecf::Host{}.name() + "." + std::to_string(port) + ".ecf.cfg");
}

///
/// @brief Returns a configuration document with a home, a log, a scheduling interval of one second, and the given
/// options.
///
/// @param[in] options The other members of `/ecflow/server`, as JSON text; empty for none
/// @return The document.
///
std::string configuration_document(const std::string& options) {
    return R"({"version": 1, "ecflow": {"server": {"variables": {"ECF_HOME": "home", "ECF_LOG": "server.log",
              "ECF_CHECKMODE": "CHECK_NEVER", "ECF_INTERVAL": 1})" +
           (options.empty() ? "" : ", " + options) + "}}}";
}

///
/// @brief Returns the suite `/s`, with a task `/s/t` whose commands only succeed or fail, and refer to a credential.
///
/// @param[in] job_cmd The value of ECF_JOB_CMD
/// @return The definition.
///
defs_ptr definition(const std::string& job_cmd) {
    auto defs = Defs::create();
    auto s    = defs->add_suite("s");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));
    s->addVariable(Variable::new_variable("ECF_JOB_CMD", job_cmd));
    s->addVariable(Variable::new_variable("ECF_KILL_CMD", "true kill %API_TOKEN% %ECF_RID%"));
    s->addVariable(Variable::new_variable("ECF_STATUS_CMD", "true status %API_TOKEN%"));
    s->add_task("t");
    return defs;
}

///
/// @brief Returns the lines of the server log, without their time stamps, once the server has flushed it.
///
/// @param[in,out] client A client of the server, used to make the server flush its log
/// @param[in] log The path of the log
/// @return The lines, each as `<type>: <text>`.
///
std::vector<std::string> server_log(ClientInvoker& client, const fs::path& log) {
    BOOST_REQUIRE_MESSAGE(client.pingServer() == 0, client.errorMsg());
    std::vector<std::string> lines;
    std::istringstream contents(ecf::test::foolproof::read_file_bytes(log));
    for (std::string line; std::getline(contents, line);) {
        const auto stamp = line.find("] ");
        const auto type  = line.find(":[");
        lines.push_back(stamp == std::string::npos || type == std::string::npos
                            ? line
                            : line.substr(0, type) + ": " + line.substr(stamp + 2));
    }
    return lines;
}

///
/// @brief Polls the server log until it holds a line starting with each of the given prefixes, or a timeout expires.
///
/// @param[in,out] client A client of the server
/// @param[in] log The path of the log
/// @param[in] prefixes The prefixes expected
/// @return The lines of the log.
///
std::vector<std::string>
wait_for_lines(ClientInvoker& client, const fs::path& log, const std::vector<std::string>& prefixes) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (true) {
        auto lines   = server_log(client, log);
        auto missing = std::find_if(prefixes.begin(), prefixes.end(), [&lines](const std::string& prefix) {
            return std::none_of(
                lines.begin(), lines.end(), [&prefix](const std::string& line) { return line.rfind(prefix, 0) == 0; });
        });
        if (missing == prefixes.end()) {
            return lines;
        }
        BOOST_REQUIRE_MESSAGE(std::chrono::steady_clock::now() < deadline, "Missing in the log: " << *missing);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerLogCommands)

///
/// @brief Verifies that a server configured to log commands records the spawn, context and termination of each
/// command, without the value of a credential.
///
BOOST_AUTO_TEST_CASE(test_server_logs_the_commands_when_enabled) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();
    fs::create_directories(startup.path() / "home" / "s");
    std::ofstream(startup.path() / "home" / "s" / "t.ecf") << "echo t\n";

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();
    WithTestFile configuration(NamedTestFile{configuration_file(startup.path(), port.value()).string()},
                               configuration_document(R"("log": {"commands": {"enabled": true}})"));

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());
    BOOST_REQUIRE_MESSAGE(client.load(definition("true submit %API_TOKEN% %ECF_JOB%")) == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.restartServer() == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.begin("s") == 0, client.errorMsg());

    const auto log = startup.path() / "home" / "server.log";
    wait_for_lines(client, log, {"EXT: ECF_JOB_CMD exit "});
    BOOST_REQUIRE_MESSAGE(client.kill("/s/t") == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.status("/s/t") == 0, client.errorMsg());

    const std::vector<std::string> expected{"EXT: ECF_JOB_CMD spawn ",
                                            "EXT: ECF_JOB_CMD context ",
                                            "EXT: ECF_JOB_CMD exit ",
                                            "EXT: ECF_KILL_CMD spawn ",
                                            "EXT: ECF_KILL_CMD context ",
                                            "EXT: ECF_KILL_CMD exit ",
                                            "EXT: ECF_STATUS_CMD spawn ",
                                            "EXT: ECF_STATUS_CMD context ",
                                            "EXT: ECF_STATUS_CMD exit "};
    const auto lines = wait_for_lines(client, log, expected);

    for (const auto& line : lines) {
        BOOST_CHECK_MESSAGE(line.find("abc123") == std::string::npos, "Credential in the log: " << line);
        if (line.rfind("EXT: ", 0) == 0) {
            BOOST_CHECK_MESSAGE(line.find("path(/s/t)") != std::string::npos, "Unexpected record: " << line);
        }
        if (line.rfind("EXT: ECF_STATUS_CMD spawn ", 0) == 0) {
            BOOST_CHECK_MESSAGE(line.find("cmd[ true status *** ]") != std::string::npos, line);
        }
        if (line.rfind("EXT: ECF_STATUS_CMD context ", 0) == 0) {
            BOOST_CHECK_MESSAGE(line.find(R"("API_TOKEN":"***")") != std::string::npos, line);
        }
    }
}

///
/// @brief Verifies that a server not configured to log commands writes no record of them.
///
BOOST_AUTO_TEST_CASE(test_server_logs_no_command_by_default) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();
    fs::create_directories(startup.path() / "home" / "s");
    std::ofstream(startup.path() / "home" / "s" / "t.ecf") << "echo t\n";

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();
    WithTestFile configuration(NamedTestFile{configuration_file(startup.path(), port.value()).string()},
                               configuration_document({}));

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());
    // The job command fails, so that its termination is recorded by the existing ERR record
    BOOST_REQUIRE_MESSAGE(client.load(definition("exit 1")) == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.restartServer() == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.begin("s") == 0, client.errorMsg());

    const auto lines = wait_for_lines(client, startup.path() / "home" / "server.log", {"ERR: ECF_JOB_CMD PID("});
    for (const auto& line : lines) {
        BOOST_CHECK_MESSAGE(line.rfind("EXT: ", 0) != 0, "Unexpected record: " << line);
    }
}

///
/// @brief Verifies that a server refuses a value of the log of commands that is not a boolean.
///
BOOST_AUTO_TEST_CASE(test_server_refuses_an_invalid_log_of_commands) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto port       = ecf::test::foolproof::reserve_port();
    const auto file = configuration_file(startup.path(), port.value());
    WithTestFile configuration(NamedTestFile{file.string()},
                               configuration_document(R"("log": {"commands": {"enabled": "yes"}})"));

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, startup.path());
    const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
    BOOST_REQUIRE_MESSAGE(exited, "An invalid log of commands did not stop start-up: " << output);
    BOOST_CHECK_EQUAL(status, 1);
    BOOST_CHECK_MESSAGE(output.find(file.filename().string() +
                                    ": /ecflow/server/log/commands/enabled: expected a boolean, not a string") !=
                            std::string::npos,
                        output);

    const auto error = ecf::test::foolproof::probe_no_listener(port.value());
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Expected connection refusal after failed startup, found: " << error.message());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

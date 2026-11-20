// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <fstream>
#include <future>
#include <list>
#include <sstream>
#include <string>
#include <tuple>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "SCPort.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/EcfPortLock.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::Permissions;
using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The loading of checkpoints at start-up (A7.6, R3.5, D11.4, R11.12): an invalid attribute makes the checkpoint
/// unusable, named with its file (and line, for a syntax error); the server starts from a valid backup, logging the
/// event, and does not start at all without one.
///

namespace {

const Credentials admin{"admin", "admin-secret"};

const std::string server_attribute = R"([{"user": "admin", "permissions": "rwxlops"},)"
                                     R"( {"user": "alice", "permissions": "r"}])";

///
/// @brief Returns the text of a checkpoint holding one suite with an attribute.
///
/// @param[in] name The name of the suite
/// @param[in] attribute The attribute of the suite, in the command-line form
/// @return The text, as written by Defs::write_to_checkpt_file().
///
std::string checkpoint(const std::string& name, const std::string& attribute) {
    Defs defs;
    defs.add_suite(name)->add_permissions(Permissions::parse_cli(attribute));
    const auto file = ecf::test::scaffold::MakeDirectory{}.create();
    const auto path = file.path() / "checkpoint";
    defs.write_to_checkpt_file(path.string());
    std::ifstream in(path);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

///
/// @brief Returns the number of the first line holding a text.
///
/// @param[in] text The lines
/// @param[in] what The text looked for
/// @return The number, from 1; 0 when no line holds it.
///
int line_of(const std::string& text, const std::string& what) {
    std::istringstream in(text);
    int number = 0;
    for (std::string line; std::getline(in, line);) {
        ++number;
        if (line.find(what) != std::string::npos) {
            return number;
        }
    }
    return 0;
}

///
/// @brief Reserves a port for a server launched directly, and releases it on destruction.
///
class Reservation {
public:
    Reservation()
        : port_{ecf::SCPort::next()} {}
    Reservation(const Reservation&)            = delete;
    Reservation& operator=(const Reservation&) = delete;
    ~Reservation() { ecf::test::scaffold::EcfPortLock::remove(port_); }

    ///
    /// @brief Returns the port reserved.
    ///
    /// @return The port.
    ///
    [[nodiscard]] const std::string& port() const { return port_; }

private:
    std::string port_;
};

///
/// @brief Starts a server in a directory of its own, with the given checkpoint files, expecting it to exit.
///
/// The environment variables read by servers are cleared first, as by PermissionsServer, so that the server reads
/// only the files of the test.
///
/// @param[in] primary The text of the primary checkpoint
/// @param[in] backup The text of the backup checkpoint; none when empty
/// @param[in] attribute The server attribute of the configuration file, in the JSON form; none when empty
/// @return The exit status of the server, its output, and the paths of its primary and backup checkpoints.
///
std::tuple<int, std::string, std::string, std::string>
start_expecting_exit(const std::string& primary,
                     const std::string& backup,
                     const std::string& attribute = server_attribute) {
    std::list<ecf::test::scaffold::WithoutTestEnvironmentVariable> cleared;
    for (const auto* name : {"ECF_HOME",
                             "ECF_PORT",
                             "ECF_LOG",
                             "ECF_CHECK",
                             "ECF_CHECKOLD",
                             "ECF_PASSWD",
                             "ECF_CUSTOM_PASSWD",
                             "ECF_LISTS",
                             "ECF_SSL",
                             "ECF_USER",
                             "ECF_HOST",
                             "ECF_HOST_PROTOCOL",
                             "ECF_PASSWORD"}) {
        cleared.emplace_back(name);
    }
    const auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    const Reservation reservation;
    const auto& port = reservation.port();
    const auto host  = ecf::Host{}.name();
    const auto home  = directory.path() / "home";
    fs::create_directories(home);
    std::ofstream(directory.path() / (host + "." + port + ".ecf.cfg"))
        << R"({"version": 1, "ecflow": {"server": {)"
        << (attribute.empty() ? std::string() : R"("permissions": )" + attribute + ", ")
        << R"("variables": {"ECF_HOME": "home", "ECF_LOG": "server.log"}}}})";
    const auto primary_path = home / ecf::Host{}.prefix_host_and_port(port, Ecf::CHECKPT());
    const auto backup_path  = home / ecf::Host{}.prefix_host_and_port(port, Ecf::BACKUP_CHECKPT());
    std::ofstream(primary_path) << primary;
    if (!backup.empty()) {
        std::ofstream(backup_path) << backup;
    }

    ecf::test::scaffold::Process server(
        fs::path(ecf::File::find_ecf_server_path()), {"--port=" + port}, directory.path());
    auto status = std::async(std::launch::async, [&server]() { return server.wait(); });
    if (status.wait_for(std::chrono::seconds(30)) != std::future_status::ready) {
        server.terminate();
        BOOST_FAIL("the server did not exit");
    }
    return {status.get(), server.read_stdout() + server.read_stderr(), primary_path.string(), backup_path.string()};
}

///
/// @brief Returns the text of a checkpoint holding suite <name>, with a syntax error in its attribute.
///
/// @param[in] name The name of the suite
/// @param[out] line The number of the line of the error
/// @return The text.
///
std::string invalid_checkpoint(const std::string& name, int& line) {
    auto text            = checkpoint(name, "admin:rwxlop,alice:r");
    line                 = line_of(text, R"("permissions":"r","user":"alice")");
    const auto attribute = text.find(R"("permissions":"r","user":"alice")");
    BOOST_REQUIRE(line > 0 && attribute != std::string::npos);
    text.replace(attribute, std::string(R"("permissions":"r")").size(), R"("permissions":"q")");
    return text;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsStartup)

///
/// @brief A7.6: an invalid attribute at line N of the checkpoint, and no backup: the server exits with a non-zero
///        status naming the file and the line, before opening its port.
///
BOOST_AUTO_TEST_CASE(test_a7_6_invalid_checkpoint_without_backup) {
    ECF_NAME_THIS_TEST();

    int line                                 = 0;
    const auto text                          = invalid_checkpoint("s", line);
    const auto [status, output, primary, _b] = start_expecting_exit(text, "");
    BOOST_CHECK_NE(status, 0);
    BOOST_CHECK_MESSAGE(output.find("no usable checkpoint") != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find(primary) != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("line number " + std::to_string(line)) != std::string::npos,
                        "line " << line << " not named in: " << output);
}

///
/// @brief An attribute naming a user the server attribute does not list (D2.15) makes the checkpoint unusable too:
///        without a backup, the server does not start, naming the file, the line and the node.
///
BOOST_AUTO_TEST_CASE(test_unlisted_user_in_checkpoint_without_backup) {
    ECF_NAME_THIS_TEST();

    const auto text                          = checkpoint("s", "admin:rwxlop,dave:r");
    const auto line                          = line_of(text, R"("user":"dave")");
    const auto [status, output, primary, _b] = start_expecting_exit(text, "");
    BOOST_CHECK_NE(status, 0);
    BOOST_CHECK_MESSAGE(output.find(primary) != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("/s names the user 'dave'") != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("(line number " + std::to_string(line) + ")") != std::string::npos,
                        "line " << line << " not named in: " << output);
}

///
/// @brief Both checkpoints invalid, the primary by its syntax and the backup by an unlisted user: the server does not
///        start, naming both files and their reasons.
///
BOOST_AUTO_TEST_CASE(test_both_checkpoints_invalid) {
    ECF_NAME_THIS_TEST();

    int line                                     = 0;
    const auto text                              = invalid_checkpoint("s", line);
    const auto [status, output, primary, backup] = start_expecting_exit(text, checkpoint("b", "admin:rwxlop,dave:r"));
    BOOST_CHECK_NE(status, 0);
    BOOST_CHECK_MESSAGE(output.find("no usable checkpoint") != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find(primary) != std::string::npos && output.find(backup) != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("line number " + std::to_string(line)) != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("/b names the user 'dave'") != std::string::npos, output);
}

///
/// @brief Without a server attribute, a checkpoint carrying a node attribute is unusable (D2.20): without a backup,
///        the server does not start, naming the file, the line and the node.
///
BOOST_AUTO_TEST_CASE(test_node_attribute_in_checkpoint_without_server_attribute) {
    ECF_NAME_THIS_TEST();

    const auto text                          = checkpoint("s", "alice:r");
    const auto line                          = line_of(text, R"("user":"alice")");
    const auto [status, output, primary, _b] = start_expecting_exit(text, "", "");
    BOOST_CHECK_NE(status, 0);
    BOOST_CHECK_MESSAGE(output.find(primary) != std::string::npos, output);
    BOOST_CHECK_MESSAGE(output.find("The Permissions attribute of /s requires a server Permissions attribute") !=
                            std::string::npos,
                        output);
    BOOST_CHECK_MESSAGE(output.find("(line number " + std::to_string(line) + ")") != std::string::npos,
                        "line " << line << " not named in: " << output);
}

///
/// @brief A7.6 with a valid backup: the server starts from the backup, and logs the event (R11.12) and the number of
///        nodes carrying an attribute.
///
BOOST_AUTO_TEST_CASE(test_a7_6_start_from_backup) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin,
                             {},
                             server_attribute,
                             std::chrono::seconds(30),
                             std::nullopt,
                             std::nullopt,
                             false,
                             {},
                             [](const fs::path& home, const std::string& port) {
                                 std::ofstream(home / ecf::Host{}.prefix_host_and_port(port, Ecf::CHECKPT()))
                                     << checkpoint("primary", "admin:rwxlop,dave:r");
                                 std::ofstream(home / ecf::Host{}.prefix_host_and_port(port, Ecf::BACKUP_CHECKPT()))
                                     << checkpoint("backup", "admin:rwxlop,alice:r");
                             });
    const auto home = server.home();
    auto client     = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    BOOST_CHECK(!client->defs()->findSuite("primary"));
    BOOST_CHECK(client->defs()->findSuite("backup"));

    std::ifstream log(home / "server.log");
    std::stringstream text;
    text << log.rdbuf();
    BOOST_CHECK_MESSAGE(text.str().find("WAR:") != std::string::npos &&
                            text.str().find("the backup may carry older Permissions attributes") != std::string::npos,
                        text.str());
    BOOST_CHECK_MESSAGE(text.str().find("Nodes with permissions = 1") != std::string::npos, text.str());
}

///
/// @brief --restore_from_checkpt at run time: a primary naming an unlisted user is rejected, the backup used, with the
///        warning (R11.12); the configured server attribute stays in force.
///
BOOST_AUTO_TEST_CASE(test_runtime_restore_from_backup) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {}, server_attribute);
    const auto home = server.home();
    const auto port = server.port();
    std::ofstream(home / ecf::Host{}.prefix_host_and_port(port, Ecf::CHECKPT()))
        << checkpoint("primary", "admin:rwxlop,dave:r");
    std::ofstream(home / ecf::Host{}.prefix_host_and_port(port, Ecf::BACKUP_CHECKPT()))
        << checkpoint("backup", "admin:rwxlop,alice:r");
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->restoreDefsFromCheckPt() == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    BOOST_CHECK(!client->defs()->findSuite("primary"));
    BOOST_CHECK(client->defs()->findSuite("backup"));
    BOOST_REQUIRE(client->defs()->server_state().permissions());
    BOOST_CHECK_EQUAL(client->defs()->server_state().permissions()->to_cli(), "admin:rwxlops,alice:r");

    std::ifstream log(home / "server.log");
    std::stringstream text;
    text << log.rdbuf();
    BOOST_CHECK_MESSAGE(text.str().find("Restored from the backup checkpoint") != std::string::npos, text.str());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

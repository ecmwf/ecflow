// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::Permissions;

namespace {

///
/// @brief Describes the start-up directory of a server, with its configuration file and an optional checkpoint.
///
class Startup {
public:
    ///
    /// @brief Creates the start-up directory.
    ///
    /// @param[in] permissions The server attribute of the configuration file, as JSON text; none when empty
    /// @param[in] variables Further members of the server variables, as JSON text, e.g. `"ECF_CHECK": "x"`
    ///
    explicit Startup(const std::string& permissions, const std::string& variables = "")
        : directory_{ecf::test::scaffold::MakeDirectory{}.create()},
          port_{ecf::test::foolproof::reserve_port()} {
        fs::create_directories(home());
        const auto file = directory_.path() / (host_name() + "." + std::to_string(port()) + ".ecf.cfg");
        std::ofstream(file) << R"({"version": 1, "ecflow": {"server": {)"
                            << (permissions.empty() ? "" : R"("permissions": )" + permissions + ", ")
                            << R"("variables": {"ECF_HOME": "home", "ECF_LOG": "server.log",)"
                            << R"( "ECF_CHECKMODE": "CHECK_NEVER")" << (variables.empty() ? "" : ", " + variables)
                            << "}}}}";
    }

    [[nodiscard]] std::uint32_t port() const { return port_.value(); }
    [[nodiscard]] fs::path directory() const { return directory_.path(); }
    [[nodiscard]] fs::path home() const { return directory_.path() / "home"; }

    ///
    /// @brief Writes the primary checkpoint, holding the given server attribute and a suite carrying an attribute.
    ///
    /// @param[in] server_permissions The server attribute, in the command-line form; none when empty
    ///
    void write_checkpoint(const std::string& server_permissions) const {
        Defs defs;
        if (!server_permissions.empty()) {
            defs.server_state().set_permissions(Permissions::parse_cli(server_permissions));
        }
        defs.add_suite("s")->add_permissions(Permissions::parse_cli("carol:rx"));
        defs.write_to_checkpt_file((home() / (host_name() + "." + std::to_string(port()) + ".ecf.check")).string());
    }

    ///
    /// @brief Returns the lines of the server log, without their time stamps.
    ///
    [[nodiscard]] std::vector<std::string> log_lines() const {
        std::vector<std::string> lines;
        std::istringstream log(ecf::test::foolproof::read_file_bytes(home() / "server.log"));
        for (std::string line; std::getline(log, line);) {
            const auto delimiter = line.find("] ");
            lines.push_back(delimiter == std::string::npos ? line : line.substr(delimiter + 2));
        }
        return lines;
    }

    ///
    /// @brief Returns the lines of the last block of the server log that starts with the given title.
    ///
    [[nodiscard]] std::vector<std::string> last_block(const std::string& title) const {
        const auto lines = log_lines();
        const auto start = std::find(lines.rbegin(), lines.rend(), title);
        std::vector<std::string> block;
        if (start == lines.rend()) {
            return block;
        }
        for (auto it = start.base(); it != lines.end() && it->rfind("  ", 0) == 0; ++it) {
            block.push_back(it->substr(2));
        }
        return block;
    }

    ///
    /// @brief Returns how many summaries of the attributes the server log holds.
    ///
    [[nodiscard]] std::size_t summaries() const {
        const auto lines = log_lines();
        return static_cast<std::size_t>(std::count(lines.begin(), lines.end(), "Permissions:"));
    }

    ///
    /// @brief Writes the whitelist of the server, granting write access to the user running the test.
    ///
    void write_whitelist() const {
        std::ofstream(home() / (host_name() + "." + std::to_string(port()) + ".ecf.lists"))
            << "4.5.0\n"
            << ecf::get_login_name() << "\n";
    }

    ///
    /// @brief Removes the whitelist of the server.
    ///
    void remove_whitelist() const { fs::remove(home() / (host_name() + "." + std::to_string(port()) + ".ecf.lists")); }

    static std::string host_name() { return ecf::Host{}.name(); }

private:
    ecf::test::scaffold::Directory directory_;
    ecf::test::scaffold::Port port_;
};

///
/// @brief Returns the server attribute received by a client, in the command-line form; empty when there is none.
///
std::string server_permissions_of(ClientInvoker& client) {
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());
    const auto* permissions = client.defs()->server_state().permissions();
    return permissions ? permissions->to_cli() : std::string{};
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerPermissions)

///
/// @brief Verifies that, without a checkpoint, the server attribute comes from the configuration file, and is
///        summarised in the log.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_from_configuration_file) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();
    Startup startup(R"([{"user": "alice", "permissions": "rwxlops"}, {"user": "bob", "permissions": "r"}])");

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), startup.port());
    BOOST_CHECK_EQUAL(server_permissions_of(client), "alice:rwxlops,bob:r");

    const std::vector<std::string> expected{
        "Source = configuration file", "Users = 2", "Sticky users = alice", "Nodes with permissions = 0"};
    const auto actual = startup.last_block("Permissions:");
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that the server attribute of the checkpoint overrides that of the configuration file.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_from_checkpoint) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();
    Startup startup(R"([{"user": "alice", "permissions": "rwxlops"}])");
    startup.write_checkpoint("carol:rwxlops,dave:rs,erin:rws");

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), startup.port());
    BOOST_CHECK_EQUAL(server_permissions_of(client), "carol:rwxlops,dave:rs,erin:rws");

    const std::vector<std::string> expected{
        "Source = checkpoint", "Users = 3", "Sticky users = carol, dave, erin", "Nodes with permissions = 1"};
    const auto actual = startup.last_block("Permissions:");
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that a checkpoint without a server attribute leaves that of the configuration file in force.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_checkpoint_without_attribute) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();
    Startup startup(R"([{"user": "alice", "permissions": "rwxlop"}])");
    startup.write_checkpoint("");

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), startup.port());
    BOOST_CHECK_EQUAL(server_permissions_of(client), "alice:rwxlop");

    const std::vector<std::string> expected{
        "Source = configuration file", "Users = 1", "Sticky users = none", "Nodes with permissions = 1"};
    const auto actual = startup.last_block("Permissions:");
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that the summary names at most ten sticky users, and is logged again after a reload of the
///        whitelist, but not after a failed one.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_summary_bound_and_reload) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();
    std::string entries;
    for (int i = 1; i <= 12; ++i) {
        entries += std::string(entries.empty() ? "" : ", ") + R"({"user": "u)" + (i < 10 ? "0" : "") +
                   std::to_string(i) + R"(", "permissions": "rs"})";
    }
    Startup startup("[" + entries + "]");
    startup.write_whitelist();

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    const std::vector<std::string> expected{
        "Source = configuration file",
        "Users = 12",
        "Sticky users = u01, u02, u03, u04, u05, u06, u07, u08, u09, u10, ... (2 more)",
        "Nodes with permissions = 0"};
    const auto actual = startup.last_block("Permissions:");
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(startup.summaries(), 1U);

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), startup.port());
    BOOST_REQUIRE_MESSAGE(client.reloadwsfile() == 0, client.errorMsg());
    BOOST_CHECK_EQUAL(startup.summaries(), 2U);

    startup.remove_whitelist();
    BOOST_CHECK(client.reloadwsfile() != 0);
    BOOST_CHECK_EQUAL(startup.summaries(), 2U);
}

///
/// @brief Verifies that a checkpoint carrying a server attribute stops the start-up of a server whose configuration
///        file has none.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_checkpoint_without_configuration) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    Startup startup("");
    startup.write_checkpoint("carol:rwxlops");

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    const auto exit = ecf::test::foolproof::wait_for_exit(process);

    BOOST_CHECK(exit.exited);
    BOOST_CHECK_NE(exit.status, 0);
    BOOST_CHECK_MESSAGE(exit.output.find("server Permissions attribute") != std::string::npos, exit.output);
    BOOST_CHECK(ecf::test::foolproof::connect_once(startup.port())); // nothing listens
}

///
/// @brief Verifies that --restore_from_checkpt applies the same order and checks as the start-up.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_restore_from_checkpoint) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();

    for (const bool configured : {true, false}) {
        BOOST_TEST_CONTEXT("server attribute in the configuration file: " << std::boolalpha << configured) {
            Startup startup(configured ? R"([{"user": "alice", "permissions": "rwxlops"}])" : "");

            Process process(ecf::test::foolproof::ecflow_server_executable(),
                            {"--port=" + std::to_string(startup.port())},
                            startup.directory());
            BOOST_SCOPE_EXIT_ALL(&) {
                BOOST_CHECK_EQUAL(process.terminate(), 0);
            };
            const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
            BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

            ClientInvoker client;
            ecf::test::foolproof::configure_client(client, host.value(), startup.port());
            BOOST_REQUIRE_MESSAGE(client.haltServer() == 0, client.errorMsg());
            startup.write_checkpoint("carol:rwxlops");

            if (configured) {
                BOOST_REQUIRE_MESSAGE(client.restoreDefsFromCheckPt() == 0, client.errorMsg());
                BOOST_CHECK_EQUAL(server_permissions_of(client), "carol:rwxlops");
                auto summary = startup.last_block("Permissions:");
                BOOST_CHECK(std::find(summary.begin(), summary.end(), "Source = checkpoint") != summary.end());

                // A later checkpoint without a server attribute falls back to the configuration file
                BOOST_REQUIRE_MESSAGE(client.delete_all(true) == 0, client.errorMsg());
                startup.write_checkpoint("");
                BOOST_REQUIRE_MESSAGE(client.restoreDefsFromCheckPt() == 0, client.errorMsg());
                BOOST_CHECK_EQUAL(server_permissions_of(client), "alice:rwxlops");
                summary = startup.last_block("Permissions:");
                BOOST_CHECK(std::find(summary.begin(), summary.end(), "Source = configuration file") != summary.end());
            }
            else {
                BOOST_CHECK(client.restoreDefsFromCheckPt() != 0);
                BOOST_CHECK_MESSAGE(client.errorMsg().find("server Permissions attribute") != std::string::npos,
                                    client.errorMsg());
                BOOST_CHECK_EQUAL(server_permissions_of(client), "");
                BOOST_CHECK(client.defs()->suites().empty());
            }
        }
    }
}

///
/// @brief Verifies that a restore failing with an error keeps the configured server attribute in force.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_restore_failure_keeps_configuration) {
    ECF_NAME_THIS_TEST();

    if (::geteuid() == 0) {
        BOOST_TEST_MESSAGE("skipped: the permissions of a directory do not restrict root");
        return;
    }

    using namespace ecf::test::scaffold;
    ecf::test::foolproof::ClearedServerEnvironment cleared;
    auto host = MakeHost{}.create();
    Startup startup(R"([{"user": "alice", "permissions": "rwxlops"}])", R"("ECF_CHECK": "locked/server.check")");
    const auto locked = startup.home() / "locked";
    fs::create_directories(locked);

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(startup.port())},
                    startup.directory());
    BOOST_SCOPE_EXIT_ALL(&) {
        fs::permissions(locked, fs::perms::owner_all);
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };
    const auto error = ecf::test::foolproof::wait_for_listener(startup.port());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), startup.port());
    BOOST_REQUIRE_MESSAGE(client.haltServer() == 0, client.errorMsg());

    // The checkpoint cannot even be inspected
    fs::permissions(locked, fs::perms::none);
    BOOST_CHECK(client.restoreDefsFromCheckPt() != 0);
    fs::permissions(locked, fs::perms::owner_all);

    BOOST_CHECK_EQUAL(server_permissions_of(client), "alice:rwxlops");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};

///
/// @brief Starts a server whose white list allows alice to read only, holding the suite "secret_suite".
///
void restrict_alice(const PermissionsServer& server) {
    std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists"))
        << "4.5.0\nadmin\n-alice\n";
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->reloadwsfile() == 0, client->errorMsg());
    auto defs = Defs::create();
    defs->add_suite("secret_suite");
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

std::string read(const fs::path& path) {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in), {}};
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsDenial)

///
/// @brief Verifies that a denial reaches the client as a denial: its own exception, failure kind and message.
///
BOOST_AUTO_TEST_CASE(test_denial_in_the_client) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    restrict_alice(server);

    auto client = server.client(alice);
    client->set_throw_on_error(true);
    try {
        (void)client->suspend("/secret_suite");
        BOOST_FAIL("the request was not denied");
    }
    catch (const ecf::PermissionDenied& e) {
        const std::string message = e.what();
        // The part of the message from the server names no node; the client prefixes the request as given
        const auto server_part = message.substr(message.find("Server reply: "));
        BOOST_CHECK_MESSAGE(server_part.find("Permission denied: --suspend") != std::string::npos, message);
        BOOST_CHECK_MESSAGE(server_part.find("secret_suite") == std::string::npos, message);
    }
    BOOST_CHECK(client->connection_diagnosis().failure == ecf::ConnectionFailure::PermissionDenied);
    BOOST_CHECK(client->server_reply().permission_denied());

    // The same client, after a request that succeeds, then one that fails without denial, holds no denial
    BOOST_CHECK_EQUAL(client->getDefs(), 0);
    BOOST_CHECK(!client->server_reply().permission_denied());
    BOOST_CHECK(client->connection_diagnosis().failure == ecf::ConnectionFailure::None);
    BOOST_CHECK_THROW((void)client->query("state", "/no/such/node", ""), std::runtime_error);
    BOOST_CHECK(!client->server_reply().permission_denied());
    BOOST_CHECK(client->connection_diagnosis().failure != ecf::ConnectionFailure::PermissionDenied);
    BOOST_CHECK_THROW((void)client->suspend("/secret_suite"), ecf::PermissionDenied);

    // Other errors are not denials
    auto admin_client = server.administrator();
    BOOST_CHECK(admin_client->suspend("/no/such/node") != 0);
    BOOST_CHECK(admin_client->connection_diagnosis().failure != ecf::ConnectionFailure::PermissionDenied);
}

///
/// @brief Verifies that the command-line client exits with its own status on a denial, and 1 on other errors.
///
BOOST_AUTO_TEST_CASE(test_denial_exit_status) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    PermissionsServer server(admin, {alice}, "");
    restrict_alice(server);

    // The command-line client takes its identity and its password from the environment
    const auto passwords = server.directory() / "alice.custom_passwd";
    std::ofstream(passwords) << "4.5.0\nalice localhost " << server.port() << " alice-secret\n";
    WithTestEnvironmentVariable host("ECF_HOST", "localhost");
    WithTestEnvironmentVariable port("ECF_PORT", server.port());
    WithTestEnvironmentVariable user("ECF_USER", "alice");
    WithTestEnvironmentVariable password_file("ECF_CUSTOM_PASSWD", passwords.string());

    const auto executable = fs::path(ecf::File::find_ecf_client_path());
    Process denied(executable, {"--suspend=/secret_suite"}, server.directory());
    BOOST_CHECK_EQUAL(denied.wait(), 3);
    BOOST_CHECK_MESSAGE(denied.read_stderr().find("Permission denied") != std::string::npos, denied.read_stderr());

    Process failed(executable, {"--no_such_option"}, server.directory());
    BOOST_CHECK_EQUAL(failed.wait(), 1);
}

///
/// @brief Verifies that denials are recorded in the server log in a parsable form, at a bounded rate.
///
BOOST_AUTO_TEST_CASE(test_denial_log_records) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    restrict_alice(server);

    auto client = server.client(alice);
    for (int i = 0; i < 20; ++i) {
        BOOST_CHECK(client->suspend("/secret_suite") != 0);
    }

    std::istringstream log(read(server.home() / "server.log"));
    std::vector<std::string> records;
    for (std::string line; std::getline(log, line);) {
        if (line.find(" DENIED ") != std::string::npos) {
            records.push_back(line);
        }
    }
    // Repeated denials are suppressed: how many records remain depends on the speed of the run (the rate itself is
    // verified with a controlled clock, by u_base T_Denial)
    BOOST_REQUIRE_GE(records.size(), 1U);
    const auto& first = records.front();
    for (const auto* field :
         {"user='alice'", "command='--suspend'", "request='--suspend /secret_suite'", "reason='", "suppressed=0"}) {
        BOOST_CHECK_MESSAGE(first.find(field) != std::string::npos, "missing " << field << " in " << first);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

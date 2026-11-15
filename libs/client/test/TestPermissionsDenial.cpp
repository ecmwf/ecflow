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
        // The message names the reason, the request and its requester (D5.29)
        BOOST_CHECK_MESSAGE(message.find("Command not accepted, due to: Authorisation (user) failed, due to: "
                                         "Insufficient permissions (write access required at /secret_suite) "
                                         "[--suspend /secret_suite :alice@") != std::string::npos,
                            message);
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
    // The output is taken once: a second read returns nothing
    const auto error = denied.read_stderr();
    BOOST_CHECK_MESSAGE(error.find("Insufficient permissions") != std::string::npos, error);

    Process failed(executable, {"--no_such_option"}, server.directory());
    BOOST_CHECK_EQUAL(failed.wait(), 1);
}

///
/// @brief Verifies that every denial is written to the server log, with the reason, the request and the requester
///        (D5.29).
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
        if (line.find("Command not accepted, due to: Authorisation") != std::string::npos) {
            records.push_back(line);
        }
    }
    BOOST_CHECK_EQUAL(records.size(), 20U);
    for (const auto& record : records) {
        BOOST_CHECK_MESSAGE(record.rfind("ERR:[", 0) == 0, record);
        BOOST_CHECK_MESSAGE(record.find("(write access required at /secret_suite) [--suspend /secret_suite :alice@") !=
                                std::string::npos,
                            record);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

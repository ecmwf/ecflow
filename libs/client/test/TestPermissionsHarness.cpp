// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "SCPort.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/EcfPortLock.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsHarness)

///
/// @brief Verifies that the harness configures the server attribute, and authenticates exactly the identities given,
///        the user running the test excluded.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_identities) {
    ECF_NAME_THIS_TEST();

    const Credentials admin{"admin", "admin-secret"};
    const Credentials alice{"alice", "alice-secret"};
    PermissionsServer server(
        admin, {alice}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}])");

    auto client = server.client(alice);
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    BOOST_REQUIRE(client->defs()->server_state().permissions() != nullptr);
    BOOST_CHECK_EQUAL(client->defs()->server_state().permissions()->to_cli(), "admin:rwxlops,alice:r");

    BOOST_CHECK(server.client(Credentials{"alice", "wrong"})->getDefs() != 0);
    BOOST_CHECK(server.client(Credentials{"mallory", "mallory-secret"})->getDefs() != 0);

    // The user running the test, without explicit credentials, is not authenticated
    ClientInvoker plain(std::string(ecf::string_constants::localhost), server.port());
    plain.set_throw_on_error(false);
    plain.set_connection_attempts(1);
    BOOST_CHECK(plain.getDefs() != 0);

    // Tear-down uses the administrator's identity, and leaves no server behind
    BOOST_CHECK(server.stop());
    BOOST_CHECK(server.administrator()->pingServer() != 0);
    BOOST_CHECK(!server.stop());
}

///
/// @brief Verifies that the harness also runs a server without a server attribute.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_without_server_attribute) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(Credentials{"admin", "admin-secret"}, {}, "");

    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    BOOST_CHECK(client->defs()->server_state().permissions() == nullptr);
}

///
/// @brief Verifies that the harness runs an HTTP server, and its clients use HTTP, whatever the JSON spacing.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_http) {
    ECF_NAME_THIS_TEST();

    for (const auto* settings : {R"("http": true)", R"("http":true)", "\"http\"\n:\ttrue"}) {
        BOOST_TEST_CONTEXT("settings: " << settings) {
            PermissionsServer server(Credentials{"admin", "admin-secret"},
                                     {},
                                     "",
                                     std::chrono::seconds(30),
                                     std::nullopt,
                                     std::nullopt,
                                     false,
                                     settings);
            auto client = server.administrator();
            BOOST_CHECK_EQUAL(client->pingServer(), 0);
            BOOST_CHECK(client->effective_protocol() == ecf::Protocol::Http);
        }
    }
}

///
/// @brief Verifies that a server refused at start-up is stopped, and its port released.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_start_failure_cleans_up) {
    ECF_NAME_THIS_TEST();

    std::string port;
    try {
        // An invalid server attribute stops the start-up of the server
        PermissionsServer server(Credentials{"admin", "admin-secret"}, {}, R"("not a list")", std::chrono::seconds(5));
        BOOST_FAIL("the harness accepted a server that did not start");
    }
    catch (const std::runtime_error& e) {
        const std::string message = e.what();
        port                      = message.substr(message.rfind(' ') + 1);
    }
    BOOST_REQUIRE(!port.empty());
    BOOST_CHECK(ecf::test::scaffold::EcfPortLock::is_free(std::stoi(port)));
}

///
/// @brief Verifies that a server refusing to terminate is killed, and its port released.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_stop_kills_when_refused) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(Credentials{"admin", "admin-secret"}, {}, "");
    const auto port = server.port();
    BOOST_CHECK(!ecf::test::scaffold::EcfPortLock::is_free(std::stoi(port)));

    BOOST_CHECK(!server.stop(Credentials{"mallory", "mallory-secret"}));
    BOOST_CHECK(!server.running());
    BOOST_CHECK(ecf::test::scaffold::EcfPortLock::is_free(std::stoi(port)));
}

///
/// @brief Verifies that a port inherited through ECF_HOST and ECF_PORT is ignored: the harness reserves its own.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_ignores_inherited_port) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // A port held by someone else, named by the environment
    const auto held = ecf::SCPort::next();
    WithTestEnvironmentVariable host("ECF_HOST", "localhost");
    WithTestEnvironmentVariable inherited("ECF_PORT", held);

    {
        PermissionsServer server(Credentials{"admin", "admin-secret"}, {}, "");
        BOOST_CHECK_NE(server.port(), held);
        BOOST_CHECK(server.running());
    }
    BOOST_CHECK(!EcfPortLock::is_free(std::stoi(held))); // still held by its owner
    EcfPortLock::remove(held);
}

///
/// @brief Verifies that a process that cannot be launched leaves no port reserved.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_launch_failure_cleans_up) {
    ECF_NAME_THIS_TEST();

    // Reservations are made in a directory of the test's own
    auto locks = ecf::test::scaffold::MakeDirectory{}.create();
    ecf::test::scaffold::WithTestEnvironmentVariable lock_directory("ECF_PORT_LOCK_DIR", locks.path().string());

    BOOST_CHECK_THROW(PermissionsServer(Credentials{"admin", "admin-secret"},
                                        {},
                                        "",
                                        std::chrono::seconds(5),
                                        std::nullopt,
                                        fs::current_path() / "no_such_executable"),
                      std::exception);
    BOOST_CHECK(fs::is_empty(locks.path()));
}

///
/// @brief Verifies that, when another server answers on the port, the harness neither stops it nor releases its port.
///
BOOST_AUTO_TEST_CASE(test_permissions_harness_ownership_mismatch_spares_foreign_server) {
    ECF_NAME_THIS_TEST();

    const Credentials admin{"admin", "admin-secret"};
    PermissionsServer foreign(admin, {}, "");

    // A second server on the same port cannot bind, and the foreign one answers, with the same credentials
    BOOST_CHECK_THROW(PermissionsServer(admin, {}, "", std::chrono::seconds(10), foreign.port()), std::runtime_error);

    BOOST_CHECK(foreign.running());
    BOOST_CHECK_EQUAL(foreign.administrator()->pingServer(), 0);
    BOOST_CHECK(!ecf::test::scaffold::EcfPortLock::is_free(std::stoi(foreign.port())));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

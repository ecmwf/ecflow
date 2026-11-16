// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "RestService.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/LockFile.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;
using ecf::test::RestService;

namespace {

// The REST services of these tests, one range per backend variant of the test, disjoint from every other test range
#if defined(ECF_TEST_HTTP_BACKEND)
constexpr int service_port_min = 13500;
constexpr int service_port_max = 13999;
#else
constexpr int service_port_min = 13000;
constexpr int service_port_max = 13499;
#endif

const std::string refusal = "The REST API does not support Permissions";

// The protocol between the service and the server, one per variant of the test
#if defined(ECF_TEST_HTTP_BACKEND)
constexpr bool http_backend = true;
#else
constexpr bool http_backend = false;
#endif

///
/// @brief Starts a server open to the login user, as the service requires, using the protocol of the variant.
///
/// @param[in] permissions The server Permissions attribute, as its JSON form; none when empty
/// @param[in] port A port held by the test, used instead of reserving one; none when empty
/// @return The server.
///
std::unique_ptr<PermissionsServer> start_server(const std::string& permissions, const std::string& port = {}) {
    return std::make_unique<PermissionsServer>(Credentials{"admin", "admin-secret"},
                                               std::vector<Credentials>{},
                                               permissions,
                                               std::chrono::seconds(30),
                                               port.empty() ? std::nullopt : std::optional<std::string>(port),
                                               std::nullopt,
                                               true,
                                               http_backend ? R"("http": true)" : "");
}

///
/// @brief Returns a server attribute that lists the login user, the account of the service.
///
/// @return The attribute, as its JSON form.
///
std::string listing_login_user() {
    return R"([{"user": "admin", "permissions": "rwxlops"}, {"user": ")" + ecf::get_login_name() +
           R"(", "permissions": "rwxlops"}])";
}

///
/// @brief Returns whether a response is the refusal of a service whose server uses Permissions (D6.6).
///
/// @param[in] response The response
/// @return True for a response with status 403 whose message states that the REST API does not support Permissions.
///
bool is_refusal(const httplib::Result& response) {
    return response && response->status == 403 && response->body.find(refusal) != std::string::npos;
}

///
/// @brief Checks that a response is the refusal of a service whose server uses Permissions (D6.6).
///
/// @param[in] response The response
/// @param[in] what The request, named in the messages of failed checks
///
void check_refused(const httplib::Result& response, const std::string& what) {
    BOOST_REQUIRE_MESSAGE(response, what << ": no response");
    BOOST_CHECK_MESSAGE(is_refusal(response), what << ": status " << response->status << ", " << response->body);
}

///
/// @brief Checks that every kind of request is refused: reads served from the copy of the tree, requests forwarded
///        to the server, and requests the service answers itself.
///
/// @param[in] service The service
///
void check_every_request_refused(const RestService& service) {
    for (const auto* path : {"/v1/suites",
                             "/v1/suites/s/tree",
                             "/v1/suites/s/definition",
                             "/v1/suites/s/status",
                             "/v1/suites/s/attributes",
                             "/v1/suites/s/t/script",
                             "/v1/server/ping",
                             "/v1/server/status",
                             "/v1/server/attributes",
                             "/v1/statistics",
                             "/v1/shutdown",
                             "/v1/unknown"}) {
        check_refused(service.get(path), std::string("GET ") + path);
    }
    check_refused(service.send("POST", "/v1/suites", R"({"definition": "suite x\nendsuite\n"})"), "POST /v1/suites");
    check_refused(service.send("PUT", "/v1/suites/s/status", R"({"action": "suspend"})"), "PUT /v1/suites/s/status");
    check_refused(service.send("PUT", "/v1/server/status", R"({"action": "reload_whitelist_file"})"),
                  "PUT /v1/server/status");
    check_refused(service.send("DELETE", "/v1/suites/s/definition", ""), "DELETE /v1/suites/s/definition");
    BOOST_CHECK(service.is_running());
}

///
/// @brief Checks that the service serves the tree of a server without Permissions, with suite s.
///
/// @param[in] service The service
///
void check_served(const RestService& service) {
    for (const auto* path : {"/v1/suites", "/v1/suites/s/tree", "/v1/server/ping", "/v1/statistics"}) {
        auto response = service.get(path);
        BOOST_REQUIRE_MESSAGE(response, path);
        BOOST_CHECK_MESSAGE(response->status == 200, path << ": status " << response->status);
        BOOST_CHECK_MESSAGE(response->body.find(refusal) == std::string::npos, path << ": " << response->body);
    }
    auto suites = service.get("/v1/suites");
    BOOST_REQUIRE(suites);
    BOOST_CHECK_MESSAGE(suites->body.find("\"s\"") != std::string::npos, suites->body);
}

///
/// @brief Waits, within a bounded time, until the service refuses, or serves, its suites.
///
/// @param[in] service The service
/// @param[in] refused True to wait for the refusal, false to wait for the suites to be served
/// @return True when the expected answer came within 60 s.
///
bool eventually(const RestService& service, bool refused) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (std::chrono::steady_clock::now() < deadline) {
        auto response = service.get("/v1/suites");
        if (refused ? is_refusal(response)
                    : (response && response->status == 200 && response->body.find("\"s\"") != std::string::npos)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return false;
}

///
/// @brief Checks, for some refresh periods of the service, that it keeps refusing, or serving, its suites.
///
/// @param[in] service The service
/// @param[in] refused True when every answer must be the refusal, false when every answer must serve the suites
/// @return True when every answer for 8 s was the expected one; the service refreshes every second, and waits 5 s
///         after a failure to reach the server.
///
bool stays(const RestService& service, bool refused) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < end) {
        auto response = service.get("/v1/suites");
        if (refused != is_refusal(response)) {
            BOOST_TEST_MESSAGE("unexpected answer: " << (response ? response->body : std::string("none")));
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return true;
}

///
/// @brief Loads suite s, with task t.
///
/// @param[in] server The server
///
void load(const PermissionsServer& server) {
    auto defs = Defs::create();
    defs->add_suite("s")->add_task("t");
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Http)

BOOST_AUTO_TEST_SUITE(T_RestService)

///
/// @brief Verifies that the REST service runs against a server of the harness open to the login user.
///
BOOST_AUTO_TEST_CASE(test_rest_service_against_configured_server) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(
        Credentials{"admin", "admin-secret"}, {}, "", std::chrono::seconds(30), std::nullopt, std::nullopt, true);

    std::string port;
    {
        RestService service(server, service_port_min, service_port_max);
        port = service.port();

        auto response = service.get("/v1/suites");
        BOOST_REQUIRE(response);
        BOOST_CHECK_EQUAL(response->status, 200);
    }
    // The reservation of the port is released (a bind probe would be refused for a while on Linux, the connections
    // closed by the service waiting in TIME_WAIT)
    BOOST_CHECK(!fs::exists(ecf::test::scaffold::LockFile::port_lock_path(port)));
}

///
/// @brief Verifies that a service that cannot refresh its copy of the tree is reported, killed, and its port released.
///
BOOST_AUTO_TEST_CASE(test_rest_service_start_failure_cleans_up) {
    ECF_NAME_THIS_TEST();

    // A server that requires a password from the login user: the service cannot authenticate, and stops. Its port is
    // reserved among those of the other tests, before the reservations of the service move to a directory of the
    // test's own
    PermissionsServer server(Credentials{"admin", "admin-secret"}, {}, "");

    auto locks = ecf::test::scaffold::MakeDirectory{}.create();
    ecf::test::scaffold::WithTestEnvironmentVariable lock_directory("ECF_PORT_LOCK_DIR", locks.path().string());

    BOOST_CHECK_THROW(RestService(server, service_port_min, service_port_max), std::runtime_error);

    for (const auto& entry : fs::directory_iterator(locks.path())) {
        const auto port = std::stoi(entry.path().stem().string());
        BOOST_CHECK_MESSAGE(port < service_port_min || port > service_port_max,
                            "the port of the service is still locked: " << port);
    }
}

///
/// @brief Verifies that a range without a free port is reported, without starting a service.
///
BOOST_AUTO_TEST_CASE(test_rest_service_range_exhausted) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    PermissionsServer server(
        Credentials{"admin", "admin-secret"}, {}, "", std::chrono::seconds(30), std::nullopt, std::nullopt, true);

    // The only port of the range is held
    auto held       = MakePort{}.with(AutomaticPortValue{static_cast<Port::port_t>(service_port_min)}).create_owned();
    const auto port = static_cast<int>(held->value());

    BOOST_CHECK_EXCEPTION(RestService(server, port, port), std::runtime_error, [](const std::runtime_error& e) {
        return std::string(e.what()).find("no free port") != std::string::npos;
    });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_RestPermissions)

///
/// @brief Verifies that a service whose refresh is refused, the server using Permissions and not listing the account
///        of the service, starts, and refuses every request (D6.6), from the start.
///
BOOST_AUTO_TEST_CASE(test_refresh_refused) {
    ECF_NAME_THIS_TEST();

    auto server = start_server(R"([{"user": "admin", "permissions": "rwxlops"}])");
    load(*server);

    RestService service(*server, service_port_min, service_port_max, http_backend);
    check_every_request_refused(service);
}

///
/// @brief Verifies that a service whose account is listed, and whose copy of the tree therefore carries the server
///        Permissions attribute, refuses every request (D6.6).
///
BOOST_AUTO_TEST_CASE(test_server_attribute_in_the_tree) {
    ECF_NAME_THIS_TEST();

    auto server = start_server(listing_login_user());
    load(*server);

    RestService service(*server, service_port_min, service_port_max, http_backend);
    check_every_request_refused(service);
}

///
/// @brief Verifies that against a server without Permissions, the service works as before (A7.7).
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server) {
    ECF_NAME_THIS_TEST();

    auto server = start_server("");
    load(*server);

    RestService service(*server, service_port_min, service_port_max, http_backend);
    check_served(service);
}

///
/// @brief Verifies the transitions of a running service, as the server behind it changes: a usable copy becoming
///        refused when the server restarts with Permissions, the refusal kept through a change of the server attribute
///        and while the server is unreachable, and the service usable again once a refresh succeeds against a server
///        without Permissions.
///
BOOST_AUTO_TEST_CASE(test_transitions) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // The servers succeed one another on the same port, held by the test
    auto held       = MakePort{}.with(AutomaticPortValue{3141}).create_owned();
    const auto port = std::to_string(held->value());

    auto server = start_server("", port);
    load(*server);
    RestService service(*server, service_port_min, service_port_max, http_backend);
    check_served(service);

    // The server restarts with Permissions, not listing the account of the service: the copy is dropped
    server->stop();
    server.reset();
    server = start_server(R"([{"user": "admin", "permissions": "rwxlops"}])", port);
    load(*server);
    BOOST_REQUIRE(eventually(service, true));
    check_every_request_refused(service);

    // The account of the service is listed: the refresh succeeds, the tree carries the server attribute
    auto admin = server->administrator();
    BOOST_REQUIRE_MESSAGE(admin->permissions("update", "/", listing_login_user(), false) == 0, admin->errorMsg());
    BOOST_CHECK(stays(service, true));

    // The account is no longer listed: its news are denied, and so is its refresh
    BOOST_REQUIRE_MESSAGE(
        admin->permissions("update", "/", R"([{"user": "admin", "permissions": "rwxlops"}])", false) == 0,
        admin->errorMsg());
    BOOST_CHECK(stays(service, true));

    // The server is unreachable: the refusal holds
    server->stop();
    server.reset();
    BOOST_CHECK(stays(service, true));

    // The server restarts without Permissions: the service is usable again
    server = start_server("", port);
    load(*server);
    BOOST_REQUIRE(eventually(service, false));
    check_served(service);

    // The server restarts with Permissions, the account of the service keeping r, with the same suites (and change
    // numbers close to those of the copy): the service refuses, without having been denied anything
    server->stop();
    server.reset();
    server = start_server(listing_login_user(), port);
    load(*server);
    BOOST_REQUIRE(eventually(service, true));
    check_every_request_refused(service);
    BOOST_CHECK(stays(service, true));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

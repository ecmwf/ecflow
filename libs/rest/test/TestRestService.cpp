// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "RestService.hpp"
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

BOOST_AUTO_TEST_SUITE_END()

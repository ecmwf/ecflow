// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <future>
#include <string>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

///
/// \brief Tests that the client retries a request whose connection to the server fails
///
/// The unit tests in TestClientRetry.cpp establish which requests are retried. These tests establish
/// that a retry actually takes place, waiting for the configured period between attempts.
///

using namespace ecf;

namespace {

using clock_type = std::chrono::steady_clock;

const auto RETRY_PERIOD = std::chrono::milliseconds{500};

///
/// @brief The outcome of a request, and the time taken to obtain it.
///
struct Outcome
{
    int result;
    std::string error;
    clock_type::duration elapsed;
};

///
/// @brief Requests the definitions from the server, retrying a failed connection as configured.
///
/// @param[in] port The port of the server
/// @param[in] attempts The number of connection attempts
/// @param[in] started Notified just before the request is sent
/// @return The outcome of the request.
///
Outcome request_definitions(const std::string& port, unsigned int attempts, std::promise<void>* started = nullptr) {
    ClientInvoker theClient(ecf::string_constants::localhost, port);
    theClient.set_throw_on_error(false);
    theClient.set_connection_attempts(attempts);
    theClient.set_retry_connection_period(RETRY_PERIOD);

    if (started) {
        started->set_value();
    }
    const auto start  = clock_type::now();
    const auto result = theClient.getDefs();
    return Outcome{result, theClient.errorMsg(), clock_type::now() - start};
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientRetryAgainstServer)

BOOST_AUTO_TEST_CASE(test_a_request_succeeds_when_the_server_starts_during_the_retries) {
    ECF_NAME_THIS_TEST();

    const auto port = SCPort::next();

    // The request is sent while no server is listening, and the server is then started
    std::promise<void> started;
    auto request =
        std::async(std::launch::async, [&port, &started] { return request_definitions(port, 20, &started); });
    started.get_future().wait();

    InvokeServer invokeServer("Client:: ...test_a_request_succeeds_when_the_server_starts_during_the_retries:", port);
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    const auto outcome = request.get();
    BOOST_CHECK_MESSAGE(outcome.result == 0, "expected the request to succeed\n" << outcome.error);
    // A request answered at the first attempt takes a few milliseconds; at least one retry took place
    BOOST_CHECK_MESSAGE(outcome.elapsed >= RETRY_PERIOD,
                        "expected at least one retry, but the request took "
                            << std::chrono::duration_cast<std::chrono::milliseconds>(outcome.elapsed).count() << "ms");
}

BOOST_AUTO_TEST_CASE(test_a_request_fails_after_the_configured_attempts_when_no_server_listens) {
    ECF_NAME_THIS_TEST();

    // The port is reserved (and released) by MakePort, so that no other test starts a server on it meanwhile.
    const auto reserved = ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{}).create();
    const auto port     = std::to_string(reserved.value());

    const unsigned int attempts = 3;
    const auto outcome          = request_definitions(port, attempts);

    BOOST_CHECK_MESSAGE(outcome.result == 1, "expected the request to fail");
    BOOST_CHECK_MESSAGE(outcome.error.find("Failed to connect to ") != std::string::npos, outcome.error);
    // Each attempt but the last is followed by a wait of the retry period
    BOOST_CHECK_MESSAGE(outcome.elapsed >= (attempts - 1) * RETRY_PERIOD,
                        "expected " << attempts << " attempts, but the request took "
                                    << std::chrono::duration_cast<std::chrono::milliseconds>(outcome.elapsed).count()
                                    << "ms");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

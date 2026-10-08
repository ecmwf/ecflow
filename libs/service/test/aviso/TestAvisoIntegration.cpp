// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "AvisoTestSupport.hpp"
#include "LocalAvisoServer.hpp"
#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/aviso/AvisoBackend.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::drain_until;
using ecf::test::LocalAvisoServer;
using ecf::test::select;

namespace {

using namespace std::chrono_literals;
using namespace ecf::service::aviso;

///
/// @brief Creates a credentials file with the given content, removed when the returned file is destroyed.
///
/// @param[in] content The content of the credentials file (JSON).
/// @return The credentials file, at a unique location.
///
ecf::test::scaffold::File make_auth_file(const std::string& content) {
    return ecf::test::scaffold::MakeTestFile{}
        .with(ecf::test::scaffold::AutomaticFileLocation{std::string{"aviso_auth"}})
        .with(content)
        .create();
}

///
/// @brief Starts a local Aviso server, on a port reserved from the base given by the test environment.
///
struct WithLocalAvisoServer
{
    WithLocalAvisoServer()
        : port{ecf::test::scaffold::MakePort{}
                   .with(ecf::test::scaffold::AutomaticPortValue{base_port()})
                   .create_owned()},
          server{static_cast<int>(port->value())} {}

    static ecf::test::scaffold::Port::port_t base_port() {
        const char* base = std::getenv("ECF_TEST_AVISO_PORT_BASE");
        if (base == nullptr) {
            throw std::runtime_error(
                "ECF_TEST_AVISO_PORT_BASE is not defined (it is set by the CMake test definition)");
        }
        return static_cast<ecf::test::scaffold::Port::port_t>(std::stoul(base));
    }

    std::unique_ptr<ecf::test::scaffold::Port> port;
    LocalAvisoServer server;
};

auto has_notifications(std::size_t count) {
    return [count](const std::vector<AvisoResponse>& r) { return select<AvisoNotification>(r).size() >= count; };
}

auto has_error() {
    return [](const std::vector<AvisoResponse>& r) { return !select<AvisoError>(r).empty(); };
}

auto has_watch_started(std::size_t count) {
    return [count](const std::vector<AvisoResponse>& r) { return select<AvisoWatchStarted>(r).size() >= count; };
}

const std::string listener = R"({ "event": "test_event", "request": { "date": "20261006" } })";

} // namespace

BOOST_AUTO_TEST_SUITE(U_AvisoBackend)

BOOST_FIXTURE_TEST_SUITE(T_AvisoIntegration, WithLocalAvisoServer)

BOOST_AUTO_TEST_CASE(delivers_live_notification) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "username": "user", "password": "pass" })");
    server.require_basic_auth("user", "pass");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));

    server.publish("test_event", R"({ "date": "20261007", "time": "1200" })"); // does not match the filter
    server.publish("test_event", R"({ "date": "20261006", "time": "1200" })", R"({ "location": "file:///x" })");

    auto responses     = drain_until(backend, has_notifications(1));
    auto notifications = select<AvisoNotification>(responses);
    BOOST_REQUIRE_EQUAL(notifications.size(), 1u);
    BOOST_CHECK_EQUAL(notifications[0].event_type(), "test_event");
    BOOST_CHECK_EQUAL(notifications[0].sequence(), 2u);
    BOOST_CHECK(notifications[0].payload_json().find("file:///x") != std::string::npos);
    BOOST_CHECK(select<AvisoError>(responses).empty());

    // The watch request carries the event type and the filter (the server refuses wrong credentials)
    auto requests = server.requests();
    BOOST_CHECK(requests[0].body.find(R"("event_type":"test_event")") != std::string::npos);
    BOOST_CHECK(requests[0].body.find(R"("date":"20261006")") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(resumes_after_revision) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");

    for (int i = 0; i < 3; ++i) {
        server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
    }

    // The attribute has already consumed notification 1, so only 2 and 3 are delivered
    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 1, auth.path().string()});

    auto responses     = drain_until(backend, has_notifications(2));
    auto notifications = select<AvisoNotification>(responses);
    BOOST_REQUIRE_EQUAL(notifications.size(), 2u);
    BOOST_CHECK_EQUAL(notifications[0].sequence(), 2u);
    BOOST_CHECK_EQUAL(notifications[1].sequence(), 3u);
}

BOOST_AUTO_TEST_CASE(accepts_matching_bearer_credentials) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.require_bearer_auth("abc");
    server.publish("test_event", R"({ "date": "20261006" })");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1));
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);
    BOOST_CHECK(select<AvisoError>(responses).empty());
}

BOOST_AUTO_TEST_CASE(reports_refused_credentials) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "wrong" })");
    server.require_bearer_auth("abc");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    // The client reports a refused credential as an authentication error
    BOOST_CHECK_MESSAGE(errors[0].reason().find("Aviso error (auth)") != std::string::npos, errors[0].reason());
}

BOOST_AUTO_TEST_CASE(reports_rejected_filter) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.reject_field("date");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    const auto& reason = errors[0].reason();
    BOOST_CHECK_MESSAGE(reason.find("HTTP 400") != std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find('\'') == std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find(std::string{unsupported_v1}) != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(reports_aviso_v1_pointer_for_server_without_aviso_v2) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");

    // An address that does not serve the Aviso v2 protocol answers with HTTP 404, as an Aviso v1 server does
    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url() + "/v1", 0, auth.path().string()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    const auto& reason = errors[0].reason();
    BOOST_CHECK_MESSAGE(reason.find("HTTP 404") != std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find(std::string{unsupported_v1}) != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(routine_close_is_handled_by_the_client_library) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.close_streams_after(300ms);

    // With the default retry delay (60 s), a notification delivered within seconds proves that the watch was not
    // re-created by the backend
    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});

    // The stream is closed by the server, and the client library reconnects at once
    BOOST_REQUIRE(server.wait_for_requests(2, 5s));
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1), 5s);
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);
    BOOST_CHECK(select<AvisoError>(responses).empty());
    BOOST_CHECK_EQUAL(select<AvisoWatchStarted>(responses).size(), 1u);
}

BOOST_AUTO_TEST_CASE(recreates_watch_after_stream_error) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.fail_streams_after(300ms);

    AvisoBackend backend{100ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});

    // The stream ends with an error, and the watch is re-created after the retry delay
    BOOST_REQUIRE(server.wait_for_requests(2, 5s));
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1));
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);
    BOOST_CHECK_GE(select<AvisoError>(responses).size(), 1u);
    BOOST_CHECK_GE(select<AvisoWatchStarted>(responses).size(), 2u);
}

BOOST_AUTO_TEST_CASE(recreated_watch_resumes_after_the_last_notification_received) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.fail_streams_after(300ms);

    AvisoBackend backend{100ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));

    // A notification is received on the first watch, which then ends with an error
    server.publish("test_event", R"({ "date": "20261006" })");
    auto first = drain_until(backend, has_notifications(1));
    BOOST_REQUIRE_EQUAL(select<AvisoNotification>(first).size(), 1u);
    BOOST_REQUIRE(server.wait_for_ended_streams(1, 5s));

    // The re-created watch resumes after that notification, which is not delivered again
    BOOST_REQUIRE(server.wait_for_requests(2, 5s));
    auto requests = server.requests();
    BOOST_CHECK_MESSAGE(requests[1].body.find("from_id") != std::string::npos, requests[1].body);
    BOOST_CHECK_MESSAGE(requests[1].body.find("from_date") == std::string::npos, requests[1].body);

    server.publish("test_event", R"({ "date": "20261006" })");
    auto second        = drain_until(backend, has_notifications(1));
    auto notifications = select<AvisoNotification>(second);
    BOOST_REQUIRE_EQUAL(notifications.size(), 1u);
    BOOST_CHECK_EQUAL(notifications[0].sequence(), 2u);
}

BOOST_AUTO_TEST_CASE(can_be_destroyed_while_notifications_are_streamed) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");

    {
        AvisoBackend backend;
        backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
        BOOST_REQUIRE(server.wait_for_requests(1, 5s));

        // Notifications are being streamed, and not drained, when the backend is destroyed
        for (int i = 0; i < 3; ++i) {
            server.publish("test_event", R"({ "date": "20261006" })");
        }
        BOOST_REQUIRE(server.wait_for_streamed(3, 5s));
    }

    // The server keeps serving, and no watch is created again by the destroyed backend
    BOOST_CHECK_EQUAL(server.publish("test_event", R"({ "date": "20261006" })"), 4u);
    BOOST_CHECK(!server.wait_for_requests(2, 500ms));
}

BOOST_AUTO_TEST_CASE(subscribing_again_replaces_the_watch) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));
    BOOST_REQUIRE_EQUAL(select<AvisoWatchStarted>(drain_until(backend, has_watch_started(1))).size(), 1u);

    // The second subscription, with another revision, replaces the first watch
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 5, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(2, 5s));
    auto requests = server.requests();
    BOOST_CHECK_MESSAGE(requests[1].body.find("from_id") != std::string::npos, requests[1].body);

    // Only the notifications after the new revision are delivered, once
    for (int i = 0; i < 6; ++i) {
        server.publish("test_event", R"({ "date": "20261006" })");
    }
    auto responses     = drain_until(backend, has_notifications(1));
    auto notifications = select<AvisoNotification>(responses);
    BOOST_REQUIRE_EQUAL(notifications.size(), 1u);
    BOOST_CHECK_EQUAL(notifications[0].sequence(), 6u);
    BOOST_CHECK_EQUAL(select<AvisoWatchStarted>(responses).size(), 1u);
    BOOST_CHECK(select<AvisoError>(responses).empty());
}

BOOST_AUTO_TEST_CASE(delivers_notification_published_while_watch_is_down) {
    ECF_NAME_THIS_TEST();

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");
    server.fail_streams_after(200ms);

    AvisoBackend backend{1500ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.path().string()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));

    // The stream ends with an error after 200 ms, and is only re-created after 1500 ms; publish in between
    BOOST_REQUIRE(server.wait_for_ended_streams(1, 5s));
    BOOST_REQUIRE_EQUAL(server.requests().size(), 1u);
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1));
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);

    // The re-created watch resumed from the time the first watch was opened
    auto requests = server.requests();
    BOOST_REQUIRE_GE(requests.size(), 2u);
    BOOST_CHECK_MESSAGE(requests[1].body.find("from_date") != std::string::npos, requests[1].body);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

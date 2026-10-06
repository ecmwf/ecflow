// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "LocalAvisoServer.hpp"
#include "TestContentProvider.hpp"
#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/aviso/v2/AvisoV2Backend.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::LocalAvisoServer;
using ecf::test::TestContentProvider;

namespace {

using namespace std::chrono_literals;
using namespace ecf::service::aviso;

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

template <typename Predicate>
std::vector<AvisoResponse>
drain_until(AvisoBackend& backend, Predicate predicate, std::chrono::milliseconds timeout = 10s) {
    std::vector<AvisoResponse> collected;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate(collected) && std::chrono::steady_clock::now() < deadline) {
        auto drained = backend.drain();
        collected.insert(collected.end(), drained.begin(), drained.end());
        std::this_thread::sleep_for(10ms);
    }
    return collected;
}

template <typename T>
std::vector<T> select(const std::vector<AvisoResponse>& responses) {
    std::vector<T> selected;
    for (const auto& response : responses) {
        if (std::holds_alternative<T>(response)) {
            selected.push_back(std::get<T>(response));
        }
    }
    return selected;
}

auto has_notifications(std::size_t count) {
    return [count](const std::vector<AvisoResponse>& r) { return select<AvisoNotification>(r).size() >= count; };
}

auto has_error() {
    return [](const std::vector<AvisoResponse>& r) { return !select<AvisoError>(r).empty(); };
}

const std::string listener = R"({ "event": "test_event", "request": { "date": "20261006" } })";

} // namespace

BOOST_AUTO_TEST_SUITE(U_AvisoV2)

BOOST_FIXTURE_TEST_SUITE(T_AvisoV2Integration, WithLocalAvisoServer)

BOOST_AUTO_TEST_CASE(delivers_live_notification) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "username": "user", "password": "pass" })"};

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});
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

    // The watch request carries the event type, the filter and the credentials
    auto requests = server.requests();
    BOOST_CHECK(requests[0].body.find(R"("event_type":"test_event")") != std::string::npos);
    BOOST_CHECK(requests[0].body.find(R"("date":"20261006")") != std::string::npos);
    BOOST_CHECK(requests[0].authorization.rfind("Basic ", 0) == 0);
}

BOOST_AUTO_TEST_CASE(resumes_after_revision) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};

    for (int i = 0; i < 3; ++i) {
        server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
    }

    // The attribute has already consumed notification 1, so only 2 and 3 are delivered
    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 1, auth.file()});

    auto responses     = drain_until(backend, has_notifications(2));
    auto notifications = select<AvisoNotification>(responses);
    BOOST_REQUIRE_EQUAL(notifications.size(), 2u);
    BOOST_CHECK_EQUAL(notifications[0].sequence(), 2u);
    BOOST_CHECK_EQUAL(notifications[1].sequence(), 3u);
}

BOOST_AUTO_TEST_CASE(accepts_matching_bearer_credentials) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};
    server.require_bearer_auth("abc");
    server.publish("test_event", R"({ "date": "20261006" })");

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1));
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);
    BOOST_CHECK(select<AvisoError>(responses).empty());
}

BOOST_AUTO_TEST_CASE(reports_refused_credentials) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "wrong" })"};
    server.require_bearer_auth("abc");

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    // The client reports a refused credential as an authentication error
    BOOST_CHECK_MESSAGE(errors[0].reason().find("Aviso error (auth)") != std::string::npos, errors[0].reason());
}

BOOST_AUTO_TEST_CASE(reports_rejected_filter) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};
    server.reject_field("date");

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    const auto& reason = errors[0].reason();
    BOOST_CHECK_MESSAGE(reason.find("HTTP 400") != std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find('\'') == std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find(std::string{unsupported_v1}) != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(reports_aviso_v1_pointer_for_server_without_aviso_v2) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};

    // An address that does not serve the Aviso v2 protocol answers with HTTP 404, as an Aviso v1 server does
    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url() + "/v1", 0, auth.file()});

    auto errors = select<AvisoError>(drain_until(backend, has_error()));
    BOOST_REQUIRE(!errors.empty());
    const auto& reason = errors[0].reason();
    BOOST_CHECK_MESSAGE(reason.find("HTTP 404") != std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find("ecFlow 5.19.x is the last release that supports Aviso v1") != std::string::npos,
                        reason);
}

BOOST_AUTO_TEST_CASE(recreates_watch_after_server_closes_stream) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};
    server.close_streams_after(300ms);

    v2::AvisoV2Backend backend{100ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});

    // The stream is closed by the server, and the watch is re-created after the retry delay
    BOOST_REQUIRE(server.wait_for_requests(2, 5s));
    server.publish("test_event", R"({ "date": "20261006" })");

    auto responses = drain_until(backend, has_notifications(1));
    BOOST_CHECK_EQUAL(select<AvisoNotification>(responses).size(), 1u);
    BOOST_CHECK_GE(select<AvisoWatchStarted>(responses).size(), 2u);
}

BOOST_AUTO_TEST_CASE(delivers_notification_published_while_watch_is_down) {
    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};
    server.close_streams_after(200ms);

    v2::AvisoV2Backend backend{1500ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", listener, server.url(), 0, auth.file()});
    BOOST_REQUIRE(server.wait_for_requests(1, 5s));

    // The stream is closed after 200 ms, and only re-created after 1500 ms; publish in between
    std::this_thread::sleep_for(600ms);
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

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "TestContentProvider.hpp"
#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/aviso/v2/AvisoV2Backend.hpp"

using ecf::test::TestContentProvider;

namespace {

using namespace std::chrono_literals;

///
/// @brief Drains the backend repeatedly, until the predicate holds for the collected responses or the timeout expires.
///
template <typename Predicate>
std::vector<ecf::service::aviso::AvisoResponse>
drain_until(ecf::service::aviso::AvisoBackend& backend, Predicate predicate, std::chrono::milliseconds timeout = 5s) {
    std::vector<ecf::service::aviso::AvisoResponse> collected;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate(collected) && std::chrono::steady_clock::now() < deadline) {
        auto drained = backend.drain();
        collected.insert(collected.end(), drained.begin(), drained.end());
        std::this_thread::sleep_for(10ms);
    }
    return collected;
}

std::size_t count_errors(const std::vector<ecf::service::aviso::AvisoResponse>& responses) {
    std::size_t count = 0;
    for (const auto& response : responses) {
        count += std::holds_alternative<ecf::service::aviso::AvisoError>(response) ? 1 : 0;
    }
    return count;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_AvisoV2)

BOOST_AUTO_TEST_SUITE(T_AvisoV2Backend)

BOOST_AUTO_TEST_CASE(reports_error_when_no_credentials_are_given) {
    using namespace ecf::service::aviso;

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});

    auto responses = backend.drain();
    BOOST_REQUIRE_EQUAL(responses.size(), 2u);
    BOOST_CHECK(std::holds_alternative<AvisoWatchStarted>(responses[0]));
    BOOST_REQUIRE(std::holds_alternative<AvisoError>(responses[1]));

    const auto& reason = std::get<AvisoError>(responses[1]).reason();
    BOOST_CHECK_MESSAGE(reason.find("no Aviso credentials file given") != std::string::npos, reason);
    BOOST_CHECK_MESSAGE(reason.find(std::string{unsupported_v1}) != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(reports_error_when_listener_is_invalid) {
    using namespace ecf::service::aviso;

    TestContentProvider auth{"aviso_v2_auth", R"({ "email": "user@host.int", "key": "abc" })"};

    v2::AvisoV2Backend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "request": {} })", "http://127.0.0.1:1", 0, auth.file()});

    auto responses = backend.drain();
    BOOST_REQUIRE_EQUAL(responses.size(), 2u);
    BOOST_REQUIRE(std::holds_alternative<AvisoError>(responses[1]));

    const auto& reason = std::get<AvisoError>(responses[1]).reason();
    BOOST_CHECK_MESSAGE(reason.find("must define the event") != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(retries_after_error) {
    using namespace ecf::service::aviso;

    v2::AvisoV2Backend backend{50ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});

    // The first attempt fails at once; the retry timer attempts again (and fails again) after the delay
    auto responses = drain_until(backend, [](const auto& collected) { return count_errors(collected) >= 3; });
    BOOST_CHECK_GE(count_errors(responses), 3u);
}

BOOST_AUTO_TEST_CASE(can_be_destroyed_while_retrying) {
    using namespace ecf::service::aviso;

    {
        v2::AvisoV2Backend backend{10ms};
        backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});
        std::this_thread::sleep_for(100ms);
    }
    // Pending retries of a destroyed backend are discarded
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

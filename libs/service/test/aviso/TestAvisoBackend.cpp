// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "AvisoTestSupport.hpp"
#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/aviso/AvisoBackend.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::drain_until;

namespace {

using namespace std::chrono_literals;

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

std::size_t count_errors(const std::vector<ecf::service::aviso::AvisoResponse>& responses) {
    return ecf::test::select<ecf::service::aviso::AvisoError>(responses).size();
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_AvisoBackend)

BOOST_AUTO_TEST_SUITE(T_AvisoBackend)

BOOST_AUTO_TEST_CASE(reports_error_when_no_credentials_are_given) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::service::aviso;

    AvisoBackend backend;
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
    ECF_NAME_THIS_TEST();

    using namespace ecf::service::aviso;

    auto auth = make_auth_file(R"({ "email": "user@host.int", "key": "abc" })");

    AvisoBackend backend;
    backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "request": {} })", "http://127.0.0.1:1", 0, auth.path().string()});

    auto responses = backend.drain();
    BOOST_REQUIRE_EQUAL(responses.size(), 2u);
    BOOST_REQUIRE(std::holds_alternative<AvisoError>(responses[1]));

    const auto& reason = std::get<AvisoError>(responses[1]).reason();
    BOOST_CHECK_MESSAGE(reason.find("must define the event") != std::string::npos, reason);
}

BOOST_AUTO_TEST_CASE(retries_after_error) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::service::aviso;

    AvisoBackend backend{50ms};
    backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});

    // The first attempt fails at once; the retry timer attempts again (and fails again) after the delay
    auto responses = drain_until(backend, [](const auto& collected) { return count_errors(collected) >= 3; });
    BOOST_CHECK_GE(count_errors(responses), 3u);
}

BOOST_AUTO_TEST_CASE(can_be_destroyed_while_retrying) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::service::aviso;

    {
        AvisoBackend backend{10ms};
        backend.subscribe(AvisoSubscribe{"/s/t:a", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});

        // The backend is retrying (a retry has already failed) when it is destroyed
        auto responses = drain_until(backend, [](const auto& collected) { return count_errors(collected) >= 2; });
        BOOST_REQUIRE_GE(count_errors(responses), 2u);
    }

    // The retries of a backend created later are scheduled after the pending retry of the destroyed one; once they
    // run, that retry has been discarded without a crash or a hang
    AvisoBackend later{10ms};
    later.subscribe(AvisoSubscribe{"/s/t:b", R"({ "event": "mars" })", "http://127.0.0.1:1", 0, ""});
    auto responses = drain_until(later, [](const auto& collected) { return count_errors(collected) >= 2; });
    BOOST_CHECK_GE(count_errors(responses), 2u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

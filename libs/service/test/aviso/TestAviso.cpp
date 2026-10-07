// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "TestContentProvider.hpp"
#include "ecflow/service/aviso/Aviso.hpp"

using ecf::test::TestContentProvider;

namespace {

template <typename T>
std::string to_string(const T& value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Aviso)

BOOST_AUTO_TEST_SUITE(T_AvisoSubscribe)

BOOST_AUTO_TEST_CASE(can_create_subscribe_request) {
    using namespace ecf::service::aviso;

    AvisoSubscribe request{"/s/f/t:a", R"({"event": "mars"})", "http://aviso:8000", 42, "/path/to/auth"};

    BOOST_CHECK_EQUAL(request.path(), "/s/f/t:a");
    BOOST_CHECK_EQUAL(request.listener(), R"({"event": "mars"})");
    BOOST_CHECK_EQUAL(request.url(), "http://aviso:8000");
    BOOST_CHECK_EQUAL(request.revision(), 42u);
    BOOST_CHECK_EQUAL(request.auth(), "/path/to/auth");
}

BOOST_AUTO_TEST_CASE(can_print_subscribe_request) {
    using namespace ecf::service::aviso;

    AvisoSubscribe request{"/s/f/t:a", R"({"event": "mars"})", "http://aviso:8000", 42, "/path/to/auth"};

    BOOST_CHECK_EQUAL(to_string(request),
                      R"(AvisoSubscribe{path: /s/f/t:a, listener: {"event": "mars"}, url: http://aviso:8000, )"
                      R"(revision: 42, auth: /path/to/auth})");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoResponse)

BOOST_AUTO_TEST_CASE(can_create_notification) {
    using namespace ecf::service::aviso;

    AvisoNotification notification{"mars", 7, R"({"step": "6"})", R"({"location": "file:///x"})"};

    BOOST_CHECK_EQUAL(notification.event_type(), "mars");
    BOOST_CHECK_EQUAL(notification.sequence(), 7u);
    BOOST_CHECK_EQUAL(notification.identifier_json(), R"({"step": "6"})");
    BOOST_CHECK_EQUAL(notification.payload_json(), R"({"location": "file:///x"})");
}

BOOST_AUTO_TEST_CASE(can_print_responses) {
    using namespace ecf::service::aviso;

    AvisoResponse notification = AvisoNotification{"mars", 7, R"({"step": "6"})", "null"};
    AvisoResponse error        = AvisoError{"connection refused"};

    BOOST_CHECK_EQUAL(to_string(notification),
                      R"(AvisoNotification{event_type: mars, sequence: 7, identifier: {"step": "6"}, payload: null})");
    BOOST_CHECK_EQUAL(to_string(error), "AvisoError{reason: connection refused}");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoListener)

BOOST_AUTO_TEST_CASE(can_parse_listener_with_event_only) {
    using namespace ecf::service::aviso;

    auto listener = parse_listener(R"({ "event": "mars" })");

    BOOST_CHECK_EQUAL(listener.event, "mars");
    BOOST_CHECK_EQUAL(listener.filter_json, "");
}

BOOST_AUTO_TEST_CASE(can_translate_listener_request_into_filter) {
    using namespace ecf::service::aviso;

    auto listener = parse_listener(
        R"({ "event": "mars", "request": { "class": "od", "expver": "0001", "step": [0, 6, 12], "time": 0 } })");

    BOOST_CHECK_EQUAL(listener.event, "mars");
    BOOST_CHECK_EQUAL(listener.filter_json, R"({"class":"od","expver":"0001","step":{"in":[0,6,12]},"time":0})");
}

BOOST_AUTO_TEST_CASE(cannot_parse_invalid_listener) {
    using namespace ecf::service::aviso;

    BOOST_CHECK_THROW(parse_listener("not json"), std::runtime_error);
    BOOST_CHECK_THROW(parse_listener(R"([ "mars" ])"), std::runtime_error);
    BOOST_CHECK_THROW(parse_listener(R"({ "request": { "class": "od" } })"), std::runtime_error);
    BOOST_CHECK_THROW(parse_listener(R"({ "event": "" })"), std::runtime_error);
    BOOST_CHECK_THROW(parse_listener(R"({ "event": 1 })"), std::runtime_error);
    BOOST_CHECK_THROW(parse_listener(R"({ "event": "mars", "request": [ 1 ] })"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoAuth)

BOOST_AUTO_TEST_CASE(can_load_basic_credentials) {
    using namespace ecf::service::aviso;

    TestContentProvider file{"aviso_auth", R"({ "username": "user", "password": "pass" })"};

    auto auth = load_auth(file.file());

    BOOST_REQUIRE(std::holds_alternative<BasicAuth>(auth));
    BOOST_CHECK_EQUAL(std::get<BasicAuth>(auth).username, "user");
    BOOST_CHECK_EQUAL(std::get<BasicAuth>(auth).password, "pass");
}

BOOST_AUTO_TEST_CASE(can_load_ecmwf_api_credentials_as_bearer_token) {
    using namespace ecf::service::aviso;

    // The content of an ECMWF API credentials file ($HOME/.ecmwfapirc): the key is the token, the email is ignored
    TestContentProvider file{"aviso_auth",
                             R"({ "url": "https://api.ecmwf.int/v1", "key": "0123456789abcdef", "email": "a@b.c" })"};

    auto auth = load_auth(file.file());

    BOOST_REQUIRE(std::holds_alternative<BearerAuth>(auth));
    BOOST_CHECK_EQUAL(std::get<BearerAuth>(auth).token, "0123456789abcdef");
}

BOOST_AUTO_TEST_CASE(prefers_key_over_basic_credentials) {
    using namespace ecf::service::aviso;

    TestContentProvider file{"aviso_auth",
                             R"({ "username": "user", "password": "pass", "email": "a@b.c", "key": "abc" })"};

    auto auth = load_auth(file.file());

    BOOST_REQUIRE(std::holds_alternative<BearerAuth>(auth));
    BOOST_CHECK_EQUAL(std::get<BearerAuth>(auth).token, "abc");
}

BOOST_AUTO_TEST_CASE(cannot_load_missing_or_unusable_credentials) {
    using namespace ecf::service::aviso;

    // No credentials file given
    BOOST_CHECK_THROW(load_auth(""), std::runtime_error);

    // Credentials file does not exist
    BOOST_CHECK_THROW(load_auth("/this/file/does/not/exist.json"), std::runtime_error);

    // Neither email and key, nor username and password
    TestContentProvider token{"aviso_auth", R"({ "token": "abc" })"};
    try {
        load_auth(token.file());
        BOOST_FAIL("Expected credentials without email and key, or username and password, to be rejected");
    }
    catch (const std::runtime_error& e) {
        BOOST_CHECK(std::string{e.what()}.find("neither user nor key credentials") != std::string::npos);
    }

    // A key without email is not a valid credentials file
    TestContentProvider key_only{"aviso_auth", R"({ "key": "abc" })"};
    BOOST_CHECK_THROW(load_auth(key_only.file()), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoError)

BOOST_AUTO_TEST_CASE(can_describe_error_naming_aviso_v1) {
    using namespace ecf::service::aviso;

    auto description = describe_error("http", 404, "Not Found", std::string{"abc-123"});

    BOOST_CHECK_EQUAL(description,
                      "Aviso error (http, HTTP 404): Not Found [request abc-123]; " +
                          std::string{ecf::service::aviso::unsupported_v1});
}

BOOST_AUTO_TEST_CASE(can_describe_error_without_quotes_nor_line_breaks) {
    using namespace ecf::service::aviso;

    auto description = describe_error("http", 400, "Field 'step' is invalid\nsee details");

    BOOST_CHECK(description.find('\'') == std::string::npos);
    BOOST_CHECK(description.find('\n') == std::string::npos);
    BOOST_CHECK(description.find("Field `step` is invalid see details") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

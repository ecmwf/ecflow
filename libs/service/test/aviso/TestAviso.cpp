// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <sstream>

#include <boost/test/unit_test.hpp>

#include "ecflow/service/aviso/Aviso.hpp"

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

BOOST_AUTO_TEST_SUITE_END()

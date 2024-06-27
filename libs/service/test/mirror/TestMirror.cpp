/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/service/mirror/Mirror.hpp"

BOOST_AUTO_TEST_SUITE(U_Mirror)

BOOST_AUTO_TEST_SUITE(T_MirrorRequest)

BOOST_AUTO_TEST_CASE(can_create_parameterised_mirror_request) {
    using namespace ecf::service::mirror;

    MirrorRequest request{"path", "host", "1234", 60, true, "auth"};

    BOOST_CHECK_EQUAL(request.path, "path");
    BOOST_CHECK_EQUAL(request.host, "host");
    BOOST_CHECK_EQUAL(request.port, "1234");
    BOOST_CHECK_EQUAL(request.polling, static_cast<uint32_t>(60));
    BOOST_CHECK_EQUAL(request.ssl, true);
    BOOST_CHECK_EQUAL(request.auth, "auth");
}

BOOST_AUTO_TEST_CASE(can_print_mirror_request) {
    using namespace ecf::service::mirror;

    MirrorRequest request{"path", "host", "1234", 60, true, "auth"};

    std::ostringstream oss;
    oss << request;
    BOOST_CHECK(oss.str().find("MirrorRequest") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_MirrorNotification)

BOOST_AUTO_TEST_CASE(can_create_parameterised_mirror_notification) {
    using namespace ecf::service::mirror;

    MirrorNotification notification{"path", MirrorData{1}};

    BOOST_CHECK_EQUAL(notification.path(), "path");
    BOOST_CHECK_EQUAL(notification.data().state, 1);
}

BOOST_AUTO_TEST_CASE(can_print_mirror_notification) {
    using namespace ecf::service::mirror;

    MirrorNotification notification{"path", MirrorData{1}};

    std::ostringstream oss;
    oss << notification;
    BOOST_CHECK(oss.str().find("MirrorNotification") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_MirrorError)

BOOST_AUTO_TEST_CASE(can_create_parameterised_mirror_error) {
    using namespace ecf::service::mirror;

    MirrorError error{"path", "reason"};

    BOOST_CHECK_EQUAL(error.path(), "path");
    BOOST_CHECK_EQUAL(error.reason(), "'reason'");
}

BOOST_AUTO_TEST_CASE(can_print_mirror_error) {
    using namespace ecf::service::mirror;

    MirrorError error{"path", "reason"};

    std::ostringstream oss;
    oss << error;
    BOOST_CHECK(oss.str().find("MirrorError") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_MirrorError)

BOOST_AUTO_TEST_CASE(can_create_mirror_response_with_notification) {
    using namespace ecf::service::mirror;

    MirrorNotification notification{"path", MirrorData{1}};
    MirrorResponse response = notification;

    BOOST_CHECK(std::holds_alternative<MirrorNotification>(response));
    BOOST_CHECK_EQUAL(std::get<MirrorNotification>(response).path(), "path");
    BOOST_CHECK_EQUAL(std::get<MirrorNotification>(response).data().state, 1);
}

BOOST_AUTO_TEST_CASE(can_create_mirror_response_with_error) {
    using namespace ecf::service::mirror;

    MirrorError error{"path", "reason"};
    MirrorResponse response = error;

    BOOST_CHECK(std::holds_alternative<MirrorError>(response));
    BOOST_CHECK_EQUAL(std::get<MirrorError>(response).path(), "path");
    BOOST_CHECK_EQUAL(std::get<MirrorError>(response).reason(), "'reason'");
}

BOOST_AUTO_TEST_CASE(can_print_mirror_response_with_notification) {
    using namespace ecf::service::mirror;

    MirrorNotification notification{"path", MirrorData{1}};
    MirrorResponse response = notification;

    std::ostringstream oss;
    oss << response;
    BOOST_CHECK(oss.str().find("MirrorNotification") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_print_mirror_response_with_error) {
    using namespace ecf::service::mirror;

    MirrorError error{"path", "reason"};
    MirrorResponse response = error;

    std::ostringstream oss;
    oss << response;
    BOOST_CHECK(oss.str().find("MirrorError") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

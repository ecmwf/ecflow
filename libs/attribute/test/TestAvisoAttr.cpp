/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/AvisoAttr.hpp"

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_AvisoAttr)

BOOST_AUTO_TEST_CASE(can_create_aviso_attribute_with_all_parameters) {

    using namespace ecf;

    auto aviso = AvisoAttr{nullptr,
                           "A",
                           R"({ "event": "mars", "request": { "class": "od"} })",
                           "http://host:port",
                           "/path/to/schema",
                           "60",
                           0,
                           "/path/to/auth",
                           "'this is a reason'"};

    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "od"} }')");
    BOOST_CHECK_EQUAL(aviso.url(), "http://host:port");
    BOOST_CHECK_EQUAL(aviso.schema(), "/path/to/schema");
    BOOST_CHECK_EQUAL(aviso.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(aviso.polling(), "60");
    BOOST_CHECK_EQUAL(aviso.reason(), "'this is a reason'");
}

BOOST_AUTO_TEST_CASE(can_create_aviso_attribute_ensuring_single_quotes_when_absent) {

    using namespace ecf;

    // Notice the absence of single quotes, on both the listener and reason
    auto aviso = AvisoAttr{nullptr,
                           "A",
                           R"({ "event": "mars", "request": { "class": "od"} })",
                           "http://host:port",
                           "/path/to/schema",
                           "60",
                           0,
                           "/path/to/auth",
                           "this is a reason"};

    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "od"} }')");
    BOOST_CHECK_EQUAL(aviso.url(), "http://host:port");
    BOOST_CHECK_EQUAL(aviso.schema(), "/path/to/schema");
    BOOST_CHECK_EQUAL(aviso.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(aviso.polling(), "60");
    BOOST_CHECK_EQUAL(aviso.reason(), "'this is a reason'");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

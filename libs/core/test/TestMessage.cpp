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

#include "ecflow/core/Message.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Message)

BOOST_AUTO_TEST_CASE(test_message_can_create_from_various_arguments) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace std::string_literals;

    BOOST_CHECK_EQUAL(Message().str(), ""s);
    BOOST_CHECK_EQUAL(Message("a", ' ', "1").str(), "a 1"s);
    BOOST_CHECK_EQUAL(Message("a", ' ', "1.01").str(), "a 1.01"s);
    BOOST_CHECK_EQUAL(Message("a", ' ', 1).str(), "a 1"s);
    BOOST_CHECK_EQUAL(Message("a", ' ', 1.01).str(), "a 1.01"s);
    BOOST_CHECK_EQUAL(Message("a", ' ', true).str(), "a 1"s);
    BOOST_CHECK_EQUAL(Message("a", Message("b", 'c')).str(), "abc"s);
}

BOOST_AUTO_TEST_CASE(test_message_can_convert_to_string) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace std::string_literals;

    std::string s = Message("a", Message("b", 'c'));
    BOOST_CHECK_EQUAL(s, "abc"s);
}

BOOST_AUTO_TEST_CASE(test_message_can_stream) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace std::string_literals;

    std::ostringstream oss;
    oss << Message("a", Message("b", 'c'));

    BOOST_CHECK_EQUAL(oss.str(), "abc"s);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

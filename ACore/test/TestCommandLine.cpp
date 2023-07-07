//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description
//============================================================================

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

#include "CommandLine.hpp"

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE(CoreTestSuite)

static void doCheck(const std::vector<std::string>& theArgs) {
    CommandLine cl(theArgs);

    BOOST_CHECK_MESSAGE(cl.size() == theArgs.size(), " argc incorrect");

    for (size_t i = 0; i < cl.size(); i++) {
        const auto& arg = cl.tokens()[i];
        BOOST_CHECK_MESSAGE(string(arg) == theArgs[i],
                            "Mismatch in args expected " << theArgs[i] << " but found " << arg);
    }
}

BOOST_AUTO_TEST_CASE(test_command_line_with_0_args) {
    std::vector<std::string> theArgs;
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_1_arg) {
    std::vector theArgs = {"arg1"s};
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_2_args) {
    std::vector theArgs = {"arg1"s, "arg2"s};
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_10_args) {
    std::vector<std::string> theArgs(10);
    for (int i = 0; i < 10; i++) {
        theArgs.push_back("arg"s + lexical_cast<string>(i));
    }
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_even_quotes) {
    CommandLine cl{R"(ecflow_client --alter=change variable VARIABLE "some long value string" /path/to/task)"};

    BOOST_REQUIRE_EQUAL(cl.size(), 6ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "ecflow_client");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "some long value string");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "/path/to/task");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_uneven_quotes) {
    CommandLine cl{R"(ecflow_client --alter=change variable VARIABLE 'some long value string" "/path/to/task')"};

    BOOST_REQUIRE_EQUAL(cl.tokens().size(), 6ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "ecflow_client");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "some long value string");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "/path/to/task");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_single_option) {
    CommandLine cl{R"(executable --option)"};

    BOOST_REQUIRE_EQUAL(cl.tokens().size(), 2ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "executable");
    BOOST_REQUIRE_EQUAL(cl.tokens()[1], "--option");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_empty_value_parameters) {
    CommandLine cl{R"(   executable --option parameter  type name    ""   "   "   " value " "\"" /some/path  )"};

    BOOST_REQUIRE_EQUAL(cl.tokens().size(), 10ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "executable");
    BOOST_REQUIRE_EQUAL(cl.tokens()[1], "--option");
    BOOST_REQUIRE_EQUAL(cl.tokens()[2], "parameter");
    BOOST_REQUIRE_EQUAL(cl.tokens()[3], "type");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "name");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "");
    BOOST_REQUIRE_EQUAL(cl.tokens()[6], "   ");
    BOOST_REQUIRE_EQUAL(cl.tokens()[7], " value ");
    BOOST_REQUIRE_EQUAL(cl.tokens()[8], "\"");
    BOOST_REQUIRE_EQUAL(cl.tokens()[9], "/some/path");
}

BOOST_AUTO_TEST_SUITE_END()

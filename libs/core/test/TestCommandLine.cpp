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

#include "TestNaming.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Converter.hpp"

using namespace boost;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CommandLine)

static void doCheck(const std::vector<std::string>& theArgs) {
    CommandLine cl(theArgs);

    BOOST_CHECK_MESSAGE(cl.size() == theArgs.size(), " argc incorrect");

    for (size_t i = 0; i < cl.size(); i++) {
        const auto& arg = cl.tokens()[i];
        BOOST_CHECK_MESSAGE(std::string(arg) == theArgs[i],
                            "Mismatch in args expected " << theArgs[i] << " but found " << arg);
    }
}

BOOST_AUTO_TEST_CASE(test_command_line_with_0_args) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> theArgs;
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_1_arg) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;

    std::vector theArgs = {"arg1"s};
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_2_args) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;

    std::vector theArgs = {"arg1"s, "arg2"s};
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_with_10_args) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;

    std::vector<std::string> theArgs(10);
    for (int i = 0; i < 10; i++) {
        theArgs.push_back("arg"s + ecf::convert_to<std::string>(i));
    }
    doCheck(theArgs);
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_even_quotes) {
    ECF_NAME_THIS_TEST();

    CommandLine cl{R"(ecflow_client --alter=change variable VARIABLE "some long value string" /path/to/task)"};

    BOOST_REQUIRE_EQUAL(cl.size(), 6ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "ecflow_client");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "some long value string");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "/path/to/task");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_incorrect_quotes) {
    ECF_NAME_THIS_TEST();

    BOOST_REQUIRE_THROW(
        CommandLine{R"(ecflow_client --alter=change variable name "some incorrectly ' quoted value" "/some/path)"},
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_unmatched_quotes) {
    ECF_NAME_THIS_TEST();

    BOOST_REQUIRE_THROW(CommandLine{R"(ecflow_client --alter=change variable name "some unclosed value /some/path)"},
                        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_command_line_with_matched_quotes) {
    ECF_NAME_THIS_TEST();

    CommandLine cl{R"(ecflow_client --alter=change variable name "some correctly 'inner' quotes value" '/some/path')"};

    BOOST_REQUIRE_EQUAL(cl.size(), 6ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "ecflow_client");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "some correctly 'inner' quotes value");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "/some/path");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_single_option) {
    ECF_NAME_THIS_TEST();

    CommandLine cl{R"(executable --option)"};

    BOOST_CHECK_EQUAL(cl.tokens().size(), 2ul);
    BOOST_CHECK_EQUAL(cl.tokens()[0], "executable");
    BOOST_CHECK_EQUAL(cl.tokens()[1], "--option");
}

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_empty_value_parameters) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_command_line_is_able_to_handle_single_quotes_in_double_quotes) {
    ECF_NAME_THIS_TEST();

    CommandLine cl{R"(ecflow_client --alter change variable name "'value'" /some/path  )"};

    BOOST_REQUIRE_EQUAL(cl.tokens().size(), 7ul);
    BOOST_REQUIRE_EQUAL(cl.tokens()[0], "ecflow_client");
    BOOST_REQUIRE_EQUAL(cl.tokens()[1], "--alter");
    BOOST_REQUIRE_EQUAL(cl.tokens()[2], "change");
    BOOST_REQUIRE_EQUAL(cl.tokens()[3], "variable");
    BOOST_REQUIRE_EQUAL(cl.tokens()[4], "name");
    BOOST_REQUIRE_EQUAL(cl.tokens()[5], "'value'");
    BOOST_REQUIRE_EQUAL(cl.tokens()[6], "/some/path");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

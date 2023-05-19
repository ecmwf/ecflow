//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : Tests the capabilities of ClientOptions
//============================================================================
#include <boost/test/unit_test.hpp>

#include "ClientEnvironment.hpp"
#include "ClientInvoker.hpp"
#include "ClientOptions.hpp"
#include "PasswordEncryption.hpp"

BOOST_AUTO_TEST_SUITE(ClientTestSuite)

BOOST_AUTO_TEST_CASE(test_is_able_to_process_username_and_password) {
    const char* expected_username = "username";
    const char* plain_password    = "password";
    std::string expected_password = PasswordEncryption::encrypt(plain_password, expected_username);

    const char* argv[]{"ecflow_client", "--user", expected_username, "--password", plain_password, "--ping"};
    int argc = boost::size(argv);

    ClientOptions options;
    ClientEnvironment environment(false);
    options.parse(argc, const_cast<char**>(argv), &environment);

    std::string actual_username = environment.get_user_name();
    BOOST_REQUIRE(expected_username == actual_username);

    std::string actual_password = environment.get_user_password(expected_username);
    BOOST_REQUIRE(expected_password == actual_password);
}

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_command_line_with_even_quotes) {
    CommandLine cl{R"(ecflow_client --alter=change variable VARIABLE "some long value string" /path/to/task)"};

    BOOST_REQUIRE(cl.tokens().size() == 6);
    BOOST_REQUIRE(cl.tokens()[0] == "ecflow_client");
    BOOST_REQUIRE(cl.tokens()[4] == "some long value string");
    BOOST_REQUIRE(cl.tokens()[5] == "/path/to/task");
}

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_command_line_with_uneven_quotes) {
    CommandLine cl{R"(ecflow_client --alter=change variable VARIABLE 'some long value string" "/path/to/task')"};

    BOOST_REQUIRE_EQUAL(cl.tokens().size(), 6);
    BOOST_REQUIRE(cl.tokens()[0] == "ecflow_client");
    BOOST_REQUIRE(cl.tokens()[4] == "some long value string");
    BOOST_REQUIRE(cl.tokens()[5] == "/path/to/task");
}

BOOST_AUTO_TEST_SUITE_END()

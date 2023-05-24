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
#include "ClientToServerCmd.hpp"
#include "CommandLine.hpp"
#include "PasswordEncryption.hpp"

BOOST_AUTO_TEST_SUITE(ClientTestSuite)

BOOST_AUTO_TEST_CASE(test_is_able_to_process_username_and_password) {
    const char* expected_username = "username";
    const char* plain_password    = "password";
    std::string expected_password = PasswordEncryption::encrypt(plain_password, expected_username);

    // Make command line
    auto cl = CommandLine::make_command_line(
        "ecflow_client", "--user", expected_username, "--password", plain_password, "--ping");

    ClientOptions options;
    ClientEnvironment environment(false);
    options.parse(cl, &environment);

    std::string actual_username = environment.get_user_name();
    BOOST_REQUIRE(expected_username == actual_username);

    std::string actual_password = environment.get_user_password(expected_username);
    BOOST_REQUIRE(expected_password == actual_password);
}

template <AlterCmd::Change_attr_type EXPECTED_TYPE>
void test_handle_alter_change(const std::string& type,
                              const std::string& name,
                              const std::string& value,
                              const std::string& path) {
    auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "change", type, name, value, path);

    ClientOptions options;
    ClientEnvironment environment(false);
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<AlterCmd*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        BOOST_REQUIRE(derived_command->value() == value);
        BOOST_REQUIRE(derived_command->name() == name);
        BOOST_REQUIRE(derived_command->change_attr_type() == EXPECTED_TYPE);
        BOOST_REQUIRE(derived_command->paths().size() == 1);
        BOOST_REQUIRE(derived_command->paths()[0] == path);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_FAIL(std::string("Unexpected exception caught: ") + e.what());
    }
}

template <AlterCmd::Add_attr_type EXPECTED_TYPE>
void test_handle_alter_add(const std::string& type,
                           const std::string& name,
                           const std::string& value,
                           const std::string& path) {
    auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "add", type, name, value, path);

    ClientOptions options;
    ClientEnvironment environment(false);
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<AlterCmd*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        BOOST_REQUIRE(derived_command->value() == value);
        BOOST_REQUIRE(derived_command->name() == name);
        BOOST_REQUIRE(derived_command->add_attr_type() == EXPECTED_TYPE);
        BOOST_REQUIRE(derived_command->paths().size() == 1);
        BOOST_REQUIRE(derived_command->paths()[0] == path);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_FAIL(std::string("Unexpected exception caught: ") + e.what());
    }
}

template <AlterCmd::Delete_attr_type EXPECTED_TYPE>
void test_handle_alter_delete(const std::string& type, const std::string& name, const std::string& path) {
    auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", type, name, path);

    ClientOptions options;
    ClientEnvironment environment(false);
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<AlterCmd*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        BOOST_REQUIRE(derived_command->value() == "");
        BOOST_REQUIRE(derived_command->name() == name);
        BOOST_REQUIRE(derived_command->delete_attr_type() == EXPECTED_TYPE);
        BOOST_REQUIRE(derived_command->paths().size() == 1);
        BOOST_REQUIRE(derived_command->paths()[0] == path);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_FAIL(std::string("Unexpected exception caught: ") + e.what());
    }
}

BOOST_AUTO_TEST_CASE(test_is_able_handle_alter) {
    std::vector<std::string> values = {"--dashes at beginning of value",
                                       "a value with --dashes inside",
                                       "    value starting with spaces",
                                       "value ending with spaces      "};

    for (const auto& value : values) {
        using Expected = AlterCmd::Change_attr_type;
        test_handle_alter_change<Expected::VARIABLE>("variable", "name", value, "/path/to/node");
        test_handle_alter_change<Expected::LABEL>("label", "name", value, "/path/to/node");
        test_handle_alter_change<Expected::EVENT>("event", "name", value, "/path/to/node");
    }

    for (const auto& value : values) {
        using Expected = AlterCmd::Add_attr_type;
        test_handle_alter_add<Expected::ADD_VARIABLE>("variable", "name", value, "/path/to/node");
        test_handle_alter_add<Expected::ADD_LABEL>("label", "name", value, "/path/to/node");
    }

    {
        using Expected = AlterCmd::Delete_attr_type;
        test_handle_alter_delete<Expected::DEL_VARIABLE>("variable", "name", "/path/to/node");
        test_handle_alter_delete<Expected::DEL_LABEL>("label", "name", "/path/to/node");
        test_handle_alter_delete<Expected::DEL_EVENT>("event", "name", "/path/to/node");
    }
}

BOOST_AUTO_TEST_SUITE_END()

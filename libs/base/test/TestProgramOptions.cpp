/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <array>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ProgramOptions)

BOOST_AUTO_TEST_CASE(test_program_options_implicit_value) {
    ECF_NAME_THIS_TEST();

    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "arg1", po::value<std::string>()->implicit_value(std::string{}), "optional arg1 description");

    {
        std::array argv = {"test_program_options_implicit_value", "--help", "--arg1"};

        po::variables_map vm;
        po::store(po::parse_command_line(argv.size(), argv.data(), desc), vm); // populate variable map
        po::notify(vm);                                                        // raise any errors

        BOOST_CHECK_MESSAGE(vm.count("help"), "Expected help");
        BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");
        BOOST_CHECK_MESSAGE(vm["arg1"].as<std::string>() == "", "Expected arg1 to be empty");
    }
    {
        std::array argv = {"test_program_options_implicit_value", "--arg1=11"};

        po::variables_map vm;
        po::store(po::parse_command_line(argv.size(), argv.data(), desc), vm);
        po::notify(vm);

        BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");
        BOOST_CHECK_MESSAGE(vm["arg1"].as<std::string>() == "11",
                            "Expected arg1 with value of 11 but found " << vm["arg1"].as<std::string>());
    }
}

BOOST_AUTO_TEST_CASE(test_program_options_multitoken) {
    ECF_NAME_THIS_TEST();

    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "arg1", po::value<std::vector<std::string>>()->multitoken(), "arg1 description");

    std::array argv = {"test_program_options_multitoken", "--help", "--arg1", "a", "b"};

    po::variables_map vm;
    po::store(po::parse_command_line(argv.size(), argv.data(), desc), vm);
    po::notify(vm);

    BOOST_CHECK_MESSAGE(vm.count("help"), "Expected help");
    BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");

    std::vector<std::string> expected;
    expected.emplace_back("a");
    expected.emplace_back("b");
    BOOST_CHECK_MESSAGE(vm["arg1"].as<std::vector<std::string>>() == expected, "multi-token not as expected");
}

BOOST_AUTO_TEST_CASE(test_program_options_multitoken_with_negative_values) {
    ECF_NAME_THIS_TEST();

    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "arg1", po::value<std::vector<std::string>>()->multitoken(), "arg1 description");

    std::array argv{"test_program_options_multitoken_1", "--help", "--arg1", "-1", "-w"};

    //  --alter delete cron -w 0,1 10:00 /s1     # -w treated as option
    //  --alter=/s1 change meter name -1         # -1 treated as option
    // Note: negative numbers get treated as options: i.e trying to change meter value to a negative number
    //  To avoid negative numbers from being treated as option use, we need to change command line style:
    //       po::command_line_style::unix_style ^ po::command_line_style::allow_short

    po::variables_map vm;
    po::store(
        po::parse_command_line(
            argv.size(), argv.data(), desc, po::command_line_style::unix_style ^ po::command_line_style::allow_short),
        vm);
    po::notify(vm);

    BOOST_CHECK_MESSAGE(vm.count("help"), "Expected help");
    BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");

    std::vector<std::string> expected;
    expected.emplace_back("-1");
    expected.emplace_back("-w");
    BOOST_CHECK_MESSAGE(vm["arg1"].as<std::vector<std::string>>() == expected, "multi-token not as expected");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

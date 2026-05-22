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
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/base/ClientOptionsParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ClientOptionsParser)

namespace {

std::vector<std::string> parse_alter(std::vector<std::string> args) {
    ecf::ClientOptionsParser parser;
    auto options = parser(args);
    BOOST_REQUIRE_EQUAL(options.size(), 1u);
    BOOST_REQUIRE_EQUAL(options[0].string_key, "alter");
    BOOST_REQUIRE_MESSAGE(args.empty(), "expected all args to be consumed, but " << args.size() << " remain");
    return options[0].value;
}

} // namespace

BOOST_AUTO_TEST_SUITE(AddInLimit)

BOOST_AUTO_TEST_CASE(add_inlimit_with_path_prefixed_limit_collects_all_tokens) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "/s/limits:cpu", "5", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "5", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_with_path_prefixed_limit_and_flag_value) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "/s/limits:cpu", "1 -s", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "1 -s", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_with_path_prefixed_limit_and_node_flag_value) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "/s/limits:cpu", "-n 5", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "-n 5", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_without_path_prefix_collects_all_tokens) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "cpu", "-s 5", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "cpu", "-s 5", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_with_default_token_count) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "/s/limits:cpu", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_with_multiple_target_nodes) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "inlimit", "/s/limits:cpu", "2", "/s/f/t1", "/s/f/t2"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "2", "/s/f/t1", "/s/f/t2"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(add_inlimit_using_equals_form) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter=add", "inlimit", "/s/limits:cpu", "5", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "inlimit", "/s/limits:cpu", "5", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END() // AddInLimit

BOOST_AUTO_TEST_SUITE(DeleteInLimit)

BOOST_AUTO_TEST_CASE(delete_inlimit_unnamed_collects_all_tokens) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "delete", "inlimit", "/s/f/t"});

    const std::vector<std::string> expected = {"delete", "inlimit", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(delete_inlimit_by_plain_name_collects_all_tokens) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "delete", "inlimit", "cpu", "/s/f/t"});

    const std::vector<std::string> expected = {"delete", "inlimit", "cpu", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(delete_inlimit_by_path_prefixed_name_collects_all_tokens) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "delete", "inlimit", "/s/limits:cpu", "/s/f/t"});

    const std::vector<std::string> expected = {"delete", "inlimit", "/s/limits:cpu", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(delete_inlimit_unnamed_multiple_nodes) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "delete", "inlimit", "/s/f/t1", "/s/f/t2"});

    const std::vector<std::string> expected = {"delete", "inlimit", "/s/f/t1", "/s/f/t2"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(AddLabel)

BOOST_AUTO_TEST_CASE(add_label_selector_does_not_interfere) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "add", "label", "my_label", "my_value", "/s/f/t"});

    const std::vector<std::string> expected = {"add", "label", "my_label", "my_value", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(DeleteLabel)

BOOST_AUTO_TEST_CASE(delete_label_selector_does_not_interfere) {
    ECF_NAME_THIS_TEST();

    const auto values = parse_alter({"--alter", "delete", "label", "my_label", "/s/f/t"});

    const std::vector<std::string> expected = {"delete", "label", "my_label", "/s/f/t"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END() // T_ClientOptionsParser

BOOST_AUTO_TEST_SUITE_END() // U_Base

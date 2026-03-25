/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <memory>

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/base/cts/user/AlterCmd.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_InLimitParsing)

auto is_invalid_inlimit_options = [](const std::runtime_error& e) {
    return ecf::algorithm::contains(e.what(), "an inlimit cannot be limited for both submission and node");
};

auto is_invalid_inlimit_value = [](const std::runtime_error& e) {
    return ecf::algorithm::contains(e.what(), "the inlimit value must be > 0, but value was: ");
};

auto is_invalid_inlimit_format = [](const std::runtime_error& e) {
    return ecf::algorithm::contains(e.what(), "the inlimit value") &&
           ecf::algorithm::contains(e.what(), "cannot be converted to an integer");
};

BOOST_AUTO_TEST_CASE(test_inlimit_parse_just_tokens_value) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;

    {
        auto value = "42";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 42);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " -13 ";

        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_value);
    }

    {
        auto value = "  0 ";

        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_value);
    }

    {
        auto value = "1 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 1);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " 2 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 2);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " 3";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 3);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, false);
    }
}

BOOST_AUTO_TEST_CASE(test_inlimit_parse_with_submission_limiting_option) {
    ECF_NAME_THIS_TEST();

    {
        auto value = " -s ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 1);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " 10 -s ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " -s 10 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " -s -s 10 -s -s ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " 10-s0 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 100);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = " 10-s-s0 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 100);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = "-s10 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, true);
        BOOST_CHECK_EQUAL(limited_node, false);
    }

    {
        auto value = "-s10 0 ";
        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_format);
    }
}

BOOST_AUTO_TEST_CASE(test_inlimit_parse_with_node_limiting_option) {
    ECF_NAME_THIS_TEST();

    // Check if limited node is handled correctly

    {
        auto value = " -n ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 1);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = " 10 -n ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = " -n 10 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = " -n -n 10 -n -n ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = " 10-n0 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 100);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = " 10-n-n0 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 100);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = "-n10 ";

        auto [tokens, limited_submission, limited_node] = ecf::parse_inlimit_value(value);

        BOOST_CHECK_EQUAL(tokens, 10);
        BOOST_CHECK_EQUAL(limited_submission, false);
        BOOST_CHECK_EQUAL(limited_node, true);
    }

    {
        auto value = "-n10 0 ";
        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_format);
    }
}

BOOST_AUTO_TEST_CASE(test_inlimit_parse_with_both_node_and_submission_limiting_options) {
    ECF_NAME_THIS_TEST();

    // Check if limited node + limited submission is handled correctly

    {
        auto value = "-s -n";

        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_options);
    }

    {
        auto value = " -s -s -s -n -n ";

        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_options);
    }

    {
        auto value = " 10 -s -n";

        BOOST_CHECK_EXCEPTION(ecf::parse_inlimit_value(value), std::runtime_error, is_invalid_inlimit_options);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

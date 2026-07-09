/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <deque>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Collections.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Collections)

BOOST_AUTO_TEST_CASE(can_reverse_iterate_a_vector) {
    ECF_NAME_THIS_TEST();

    std::vector<int> values{1, 2, 3, 4, 5};

    std::vector<int> collected;
    for (auto& v : ecf::collections::reversed(values)) {
        collected.push_back(v);
    }

    std::vector<int> expected{5, 4, 3, 2, 1};
    BOOST_CHECK_EQUAL(collected.size(), 5u);
    BOOST_CHECK_EQUAL_COLLECTIONS(collected.begin(), collected.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(can_reverse_iterate_an_empty_vector) {
    ECF_NAME_THIS_TEST();

    std::vector<int> values;

    int count = 0;
    for ([[maybe_unused]] auto& v : ecf::collections::reversed(values)) {
        ++count;
    }

    BOOST_CHECK_EQUAL(count, 0);
}

BOOST_AUTO_TEST_CASE(can_reverse_iterate_a_const_vector) {
    ECF_NAME_THIS_TEST();

    const std::vector<int> values{10, 20, 30};

    std::vector<int> collected;
    for (const auto& v : ecf::collections::reversed(values)) {
        collected.push_back(v);
    }

    std::vector<int> expected{30, 20, 10};
    BOOST_CHECK_EQUAL_COLLECTIONS(collected.begin(), collected.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(can_modify_a_vector_through_reversed_adapter) {
    ECF_NAME_THIS_TEST();

    std::vector<int> values{1, 2, 3};

    int next = 100;
    for (auto& v : ecf::collections::reversed(values)) {
        v = next++;
    }

    // The adapter visits elements back-to-front, so the assignment order is reflected in reverse.
    std::vector<int> expected{102, 101, 100};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(can_reverse_iterate_a_deque_of_pairs) {
    ECF_NAME_THIS_TEST();

    using entry_t = std::pair<std::string, int>;
    std::deque<entry_t> values{{"a", 1}, {"b", 2}, {"c", 3}};

    std::deque<entry_t> collected;
    for (auto& v : ecf::collections::reversed(values)) {
        collected.push_back(v);
    }

    std::deque<entry_t> expected{{"c", 3}, {"b", 2}, {"a", 1}};
    BOOST_REQUIRE_EQUAL(collected.size(), expected.size());
    for (size_t i = 0; i < collected.size(); ++i) {
        BOOST_CHECK(collected[i] == expected[i]);
    }
}

BOOST_AUTO_TEST_CASE(can_reverse_iterate_an_empty_deque) {
    ECF_NAME_THIS_TEST();

    using entry_t = std::pair<std::string, int>;
    std::deque<entry_t> values;

    int count = 0;
    for ([[maybe_unused]] auto& v : ecf::collections::reversed(values)) {
        ++count;
    }

    BOOST_CHECK_EQUAL(count, 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

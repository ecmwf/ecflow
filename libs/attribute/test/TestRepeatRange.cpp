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

#include "ecflow/attribute/RepeatRange.hpp"

#define CHECK_EQUAL(A, E) BOOST_CHECK_EQUAL(A, static_cast<typename decltype(rng)::size_type>(E))

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_RepeatRange)

/*
 * Test Suite: ::test_repeat_datelist
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_datelist)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    {
        RepeatDateList repeat("R", {20190929});

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1);
        CHECK_EQUAL(rng.size(), 1);
    }
    {
        RepeatDateList repeat("R", {20190929, 20190131});

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 2);
        CHECK_EQUAL(rng.size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    {
        RepeatDateList repeat("R", {20190929, 20190131});

        Range rng(repeat);
        std::vector expected = {20190929, 20190131};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    {
        RepeatDateList repeat("R", {20190929, 20190131, 20190929});

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.value(), 20190929);
        BOOST_CHECK_EQUAL(rng.current_value(), 20190929);
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 20190131);
        BOOST_CHECK_EQUAL(rng.current_value(), 20190131);
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 20190929);
        BOOST_CHECK_EQUAL(rng.current_value(), 20190929);
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_datelist

/*
 * Test Suite: ::test_repeat_date
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_date)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    { // range of a single value
        RepeatDate repeat("R", 20241129, 20241129, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1);
        CHECK_EQUAL(rng.size(), 1);
    }
    { // ranges of sequencial values, with step 1
        RepeatDate repeat("R", 20241129, 20241203, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 5);
        CHECK_EQUAL(rng.size(), 5);
    }
    { // ranges of values, with step 2; first/last value included in given values
        RepeatDate repeat("R", 20241129, 20241203, 2);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    { // ranges of values, with step 2; last value not included in given values
        RepeatDate repeat("R", 20241129, 20241204, 2);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    { // ranges of values, with step 2; first/last value included in given values
        RepeatDate repeat("R", 20241129, 20241205, 2);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 4);
        CHECK_EQUAL(rng.size(), 4);
    }
    { // ranges of values, with step 3; first/last value included in given values
        RepeatDate repeat("R", 20241129, 20241205, 3);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    { // ranges of values, with step 3; last value not included in given values
        RepeatDate repeat("R", 20241129, 20241206, 3);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    { // ranges of values, with step 2; last value not included in given values
        RepeatDate repeat("R", 20241129, 20241207, 3);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    { // ranges of values, with step 2; first/last value included in given values
        RepeatDate repeat("R", 20241129, 20241208, 3);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 4);
        CHECK_EQUAL(rng.size(), 4);
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    {
        RepeatDate repeat("R", 20241129, 20241203, 1);

        Range rng(repeat);
        std::vector expected = {20241129, 20241130, 20241201, 20241202, 20241203};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
    {
        RepeatDate repeat("R", 20241129, 20241207, 3);

        Range rng(repeat);
        std::vector expected = {20241129, 20241202, 20241205};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
    {
        RepeatDate repeat("R", 20241129, 20241208, 3);

        Range rng(repeat);
        std::vector expected = {20241129, 20241202, 20241205, 20241208};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    {
        RepeatDate repeat("R", 20241129, 20241207, 3);

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.value(), 20241129);
        BOOST_CHECK_EQUAL(rng.current_value(), 20241129);
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 20241202);
        BOOST_CHECK_EQUAL(rng.current_value(), 20241202);
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 20241205);
        BOOST_CHECK_EQUAL(rng.current_value(), 20241205);
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_date

/*
 * Test Suite: ::test_repeat_datetime
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_datetime)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700101T000001", "24:00:00");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1);
        CHECK_EQUAL(rng.size(), 1);
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000000", "24:00:00");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1);
        CHECK_EQUAL(rng.size(), 1);
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000001", "24:00:00");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 2);
        CHECK_EQUAL(rng.size(), 2);
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000001", "0:01:00");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1441);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(1441));
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000000", "0:01:00");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 1440);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(1440));
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000001", "0:00:01");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 86401);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(86401));
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000000", "0:00:01");

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 86400);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(86400));
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000001", "06:00:00");

        Range rng(repeat);
        std::vector expected = {
            "19700101T000001", "19700101T060001", "19700101T120001", "19700101T180001", "19700102T000001"};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(Instant::format(rng.at(i)), expected[i]);
        }
    }
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000000", "06:00:00");

        Range rng(repeat);
        std::vector expected = {"19700101T000001", "19700101T060001", "19700101T120001", "19700101T180001"};
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(Instant::format(rng.at(i)), expected[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    {
        RepeatDateTime repeat("R", "19700101T000001", "19700102T000001", "12:00:00");

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.valueAsString(), "19700101T000001");
        BOOST_CHECK_EQUAL(Instant::format(rng.current_value()), "19700101T000001");
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), "19700101T120001");
        BOOST_CHECK_EQUAL(Instant::format(rng.current_value()), "19700101T120001");
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), "19700102T000001");
        BOOST_CHECK_EQUAL(Instant::format(rng.current_value()), "19700102T000001");
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_datetime

/*
 * Test Suite: ::test_repeat_enumerated
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_enumerated)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"1"s, "2"s, "3"s};
    {
        RepeatEnumerated repeat("R", strings);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"1"s, "2"s, "3"s};
    {
        RepeatEnumerated repeat("R", strings);

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), strings.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), strings[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"1"s, "2"s, "3"s};
    {
        RepeatEnumerated repeat("R", strings);

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[0]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[0]);
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[1]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[1]);
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[2]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[2]);
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_enumerated

/*
 * Test Suite: ::test_repeat_integer
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_integer)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;

    {
        RepeatInteger repeat("R", 0, 10, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 11);
        CHECK_EQUAL(rng.size(), 11);
    }
    {
        RepeatInteger repeat("R", 5, 10, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 6);
        CHECK_EQUAL(rng.size(), 6);
    }
    {
        RepeatInteger repeat("R", 1, 100, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 100);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(100));
    }
    {
        RepeatInteger repeat("R", 0, 100, 1);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 101);
        CHECK_EQUAL(rng.size(), static_cast<typename decltype(rng)::size_type>(101));
    }
    {
        RepeatInteger repeat("R", 0, 4, 2);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
    {
        RepeatInteger repeat("R", 0, 6, 3);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    {
        RepeatInteger repeat("R", 0, 6, 1);
        std::vector expected = {0, 1, 2, 3, 4, 5, 6};

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
    {
        RepeatInteger repeat("R", 0, 6, 2);
        std::vector expected = {0, 2, 4, 6};

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
    {
        RepeatInteger repeat("R", 1, 6, 1);
        std::vector expected = {1, 2, 3, 4, 5, 6};

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
    {
        RepeatInteger repeat("R", 1, 6, 2);
        std::vector expected = {1, 3, 5};

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), expected.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), expected[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    {
        RepeatInteger repeat("R", 1, 6, 2);

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.value(), 1);
        BOOST_CHECK_EQUAL(rng.current_value(), 1);
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 3);
        BOOST_CHECK_EQUAL(rng.current_value(), 3);
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.value(), 5);
        BOOST_CHECK_EQUAL(rng.current_value(), 5);
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_integer

/*
 * Test Suite: ::test_repeat_day
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_day)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    {
        RepeatDay repeat(2);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 0);
        CHECK_EQUAL(rng.size(), 0);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_day

/*
 * Test Suite: ::test_repeat_string
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(test_repeat_string)

BOOST_AUTO_TEST_CASE(can_construct) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"a"s, "b"s, "c"s};
    {
        RepeatString repeat("R", strings);

        Range rng(repeat);
        CHECK_EQUAL(rng.begin(), 0);
        CHECK_EQUAL(rng.end(), 3);
        CHECK_EQUAL(rng.size(), 3);
    }
}

BOOST_AUTO_TEST_CASE(can_access_iterating) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"a"s, "b"s, "c"s};
    {
        RepeatString repeat("R", strings);

        Range rng(repeat);
        CHECK_EQUAL(rng.size(), strings.size());
        for (auto i = std::begin(rng); i != std::end(rng); ++i) {
            BOOST_CHECK_EQUAL(rng.at(i), strings[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(can_access_current) {
    using namespace ecf;
    using namespace std::string_literals;
    const std::vector strings = {"a"s, "b"s, "c"s};
    {
        RepeatString repeat("R", strings);

        Range rng(repeat);
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[0]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[0]);
        CHECK_EQUAL(rng.current_index(), 0);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[1]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[1]);
        CHECK_EQUAL(rng.current_index(), 1);

        repeat.increment();
        BOOST_CHECK_EQUAL(repeat.valueAsString(), strings[2]);
        BOOST_CHECK_EQUAL(rng.current_value(), strings[2]);
        CHECK_EQUAL(rng.current_index(), 2);
    }
}

BOOST_AUTO_TEST_SUITE_END() // test_repeat_string

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

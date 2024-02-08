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

#include "ecflow/core/Chrono.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Chrono)

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_create_reference_instant) {
    Instant instant;
    BOOST_CHECK_EQUAL(Instant::format(instant), "19700101T000000");
}

BOOST_AUTO_TEST_CASE(test_chrono__is_able_to_create_instant_based_on_std_chrono_time_point) {
    Instant original{std::chrono::system_clock::now()};

    auto timestamp        = Instant::format(original);
    Instant reconstructed = Instant::parse(timestamp);
    auto timestamp2       = Instant::format(reconstructed);

    BOOST_CHECK_EQUAL(original, reconstructed);
}

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_parse_and_format_instant) {
    {
        std::string value = "19700101T000000";
        Instant instant   = Instant::parse(value);
        BOOST_CHECK_EQUAL(Instant::format(instant), value);
    }
    {
        std::string value = "20000101T235959";
        Instant instant   = Instant::parse(value);
        BOOST_CHECK_EQUAL(Instant::format(instant), value);
    }
}

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_compare_instants_for_equality) {
    Instant instant0 = Instant::parse("20000101T235959");

    Instant instant1 = instant0;
    Instant instant2 = Instant::parse("20000102T000000");
    Instant instant3 = Instant::parse("20000102T000001");

    BOOST_CHECK(instant0 == instant1);

    BOOST_CHECK(instant1 == instant1);
    BOOST_CHECK(instant1 != instant2);
    BOOST_CHECK(instant1 != instant3);

    BOOST_CHECK(instant2 != instant1);
    BOOST_CHECK(instant2 == instant2);
    BOOST_CHECK(instant2 != instant3);

    BOOST_CHECK(instant3 != instant1);
    BOOST_CHECK(instant3 != instant2);
    BOOST_CHECK(instant3 == instant3);
}

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_compare_instants_for_inequality) {
    Instant instant1 = Instant::parse("20000101T235959");
    Instant instant2 = Instant::parse("20000102T000000");
    Instant instant3 = Instant::parse("20000102T000001");

    BOOST_CHECK(instant1 < instant2);
    BOOST_CHECK(instant1 < instant3);

    BOOST_CHECK(instant2 > instant1);
    BOOST_CHECK(instant2 < instant3);

    BOOST_CHECK(instant3 > instant1);
    BOOST_CHECK(instant3 > instant2);
}

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_add_duration_to_instant) {
    {
        Instant instant = Instant::parse("20000101T235959");
        Instant next    = instant + Duration{std::chrono::seconds{1}};
        BOOST_CHECK_EQUAL(Instant::format(next), "20000102T000000");
    }
    {
        Instant instant = Instant::parse("20000101T235959");
        Instant next    = instant + Duration{-std::chrono::seconds{1}};
        BOOST_CHECK_EQUAL(Instant::format(next), "20000101T235958");
    }
}

BOOST_AUTO_TEST_CASE(test_chrono_is_able_to_subtract_duration_from_instant) {
    {
        Instant instant = Instant::parse("20000101T235959");
        Instant next    = instant - Duration{std::chrono::seconds{1}};
        BOOST_CHECK_EQUAL(Instant::format(next), "20000101T235958");
    }
    {
        Instant instant = Instant::parse("20000101T235959");
        Instant next    = instant - Duration{-std::chrono::seconds{1}};
        BOOST_CHECK_EQUAL(Instant::format(next), "20000102T000000");
    }
}

BOOST_AUTO_TEST_CASE(test_chrono_parsing_invalid_value_throws) {
    using expected = std::runtime_error;
    BOOST_CHECK_THROW(Instant::parse("20000101T235961"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000101T236059"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000101T240000"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000101T555555"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000132T000000"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000230T000000"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000431T000000"), expected);
    BOOST_CHECK_THROW(Instant::parse("20000631T000000"), expected);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

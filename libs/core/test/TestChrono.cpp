// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string> // also defines _LIBCPP_VERSION, when built with libc++

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Chrono.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Chrono)

BOOST_AUTO_TEST_CASE(is_able_to_create_reference_instant) {
    ECF_NAME_THIS_TEST();

    Instant instant;
    BOOST_CHECK_EQUAL(Instant::format(instant), "19700101T000000");
}

BOOST_AUTO_TEST_CASE(is_able_to_create_instant_based_on_std_chrono_time_point) {
    ECF_NAME_THIS_TEST();

    Instant original{std::chrono::system_clock::now()};

    auto timestamp        = Instant::format(original);
    Instant reconstructed = Instant::parse(timestamp);
    auto timestamp2       = Instant::format(reconstructed);

    BOOST_CHECK_EQUAL(original, reconstructed);
    BOOST_CHECK_EQUAL(timestamp, timestamp2);
}

BOOST_AUTO_TEST_CASE(is_able_to_parse_and_format_instant) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(is_able_to_compare_instants_for_equality) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(is_able_to_compare_instants_for_inequality) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(is_able_to_add_duration_to_instant) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(is_able_to_subtract_duration_from_instant) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(parsing_invalid_value_throws) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(is_able_to_cast_instant_to_duration) {
    ECF_NAME_THIS_TEST();

    {
        ecf::Instant instant;
        BOOST_CHECK_EQUAL(ecf::Instant::format(instant), "19700101T000000");
        BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into_seconds(instant), 0);
        BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into<std::chrono::nanoseconds>(instant), 0);
    }

    {
        ecf::Instant instant = ecf::Instant::parse("20230601T000000");
        BOOST_CHECK_EQUAL(ecf::Instant::format(instant), "20230601T000000");
        BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into_seconds(instant), 1685577600);
        BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into<std::chrono::nanoseconds>(instant), 1685577600000000000);
    }
}

BOOST_AUTO_TEST_CASE(is_able_to_parse_and_format_duration) {
    ECF_NAME_THIS_TEST();

    auto seconds = [](const std::string& value) { return Duration::parse(value).as_seconds().count(); };

    BOOST_CHECK_EQUAL(seconds("12"), 43200);
    BOOST_CHECK_EQUAL(seconds("12:00"), 43200);
    BOOST_CHECK_EQUAL(seconds("12:00:00"), 43200);
    BOOST_CHECK_EQUAL(seconds("01:02:03"), 3723);
    BOOST_CHECK_EQUAL(seconds("+01:00:00"), 3600);
    BOOST_CHECK_EQUAL(seconds("-06:00:00"), -21600);
    BOOST_CHECK_EQUAL(seconds("-01:30:00"), -5400);
    BOOST_CHECK_EQUAL(seconds("00:-30:00"), -1800);
    BOOST_CHECK_THROW(seconds("1.5"), std::runtime_error);
    BOOST_CHECK_THROW(seconds("abc"), std::runtime_error);

    BOOST_CHECK_EQUAL(Duration::format(Duration{std::chrono::seconds{43200}}), "12:00:00");
    BOOST_CHECK_EQUAL(Duration::format(Duration{std::chrono::seconds{-5400}}), "-1:30:00");
    BOOST_CHECK_EQUAL(Duration::format(Duration{std::chrono::seconds{-1800}}), "00:-30:00");

    // The formatted durations parse back to the same value
    for (long value : {43200L, 3723L, -5400L, -1800L, -30L}) {
        auto duration = Duration{std::chrono::seconds{value}};
        BOOST_CHECK_EQUAL(seconds(Duration::format(duration)), value);
    }
}

BOOST_AUTO_TEST_CASE(duration_parsing_applies_a_leading_sign_to_the_whole_duration) {
    ECF_NAME_THIS_TEST();

    auto seconds = [](const std::string& value) { return Duration::parse(value).as_seconds().count(); };

    BOOST_CHECK_EQUAL(seconds("-00:30:00"), -1800);
    BOOST_CHECK_EQUAL(seconds("-0:0:30"), -30);
    BOOST_CHECK_EQUAL(seconds("-00:30"), -1800);
    BOOST_CHECK_EQUAL(seconds("-1:30:00"), -5400);
    BOOST_CHECK_EQUAL(seconds("-12"), -43200);
    BOOST_CHECK_EQUAL(seconds("-0"), 0);
    BOOST_CHECK_EQUAL(seconds("+00:30:00"), 1800);
    BOOST_CHECK_EQUAL(seconds("+0:0:30"), 30);
}

BOOST_AUTO_TEST_CASE(duration_parsing_accepts_the_sign_written_by_format) {
    ECF_NAME_THIS_TEST();

    auto seconds = [](const std::string& value) { return Duration::parse(value).as_seconds().count(); };

    // A negative duration is written with the sign on its first non-zero field, the preceding fields being zero
    BOOST_CHECK_EQUAL(seconds("00:-30:00"), -1800);
    BOOST_CHECK_EQUAL(seconds("00:00:-30"), -30);
    BOOST_CHECK_EQUAL(seconds("00:-1:30"), -90);
    BOOST_CHECK_EQUAL(seconds("0:-30"), -1800);
    BOOST_CHECK_EQUAL(seconds("00:-00:30"), -30);

    // Every formatted duration parses back to the same value
    for (long value : {0L,  1L,   59L,  60L,  61L,    3599L,  3600L,  3601L,   86399L,  86400L,  259200L,
                       -1L, -59L, -60L, -61L, -3599L, -3600L, -3601L, -86399L, -86400L, -259200L}) {
        auto formatted = Duration::format(Duration{std::chrono::seconds{value}});
        BOOST_CHECK_MESSAGE(seconds(formatted) == value,
                            value << " is formatted as " << formatted << ", which parses as " << seconds(formatted));
    }
}

BOOST_AUTO_TEST_CASE(duration_parsing_refuses_misplaced_signs) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"01:-30:00",
                                    "-1:-30:00",
                                    "-00:-30:00",
                                    "00:-30:-10",
                                    "00:30:-10",
                                    "00:+30:00",
                                    "00:00:+30",
                                    "--1",
                                    "+-1",
                                    "-+1",
                                    "-",
                                    "+",
                                    "00:-",
                                    "1-",
                                    "1:3-0"}) {
        BOOST_CHECK_THROW(Duration::parse(value), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(duration_parsing_refuses_malformed_fields) {
    ECF_NAME_THIS_TEST();

    for (const std::string value :
         {"",   ":",     "::",    ":30", "1:",     "1::", "1::00", "01:02:03:04", "1:2:3:4", "00:00:00:00", " 1",
          "1 ", "1:00 ", "1: 00", "1.5", "1:30.5", "abc", "1h",    "0x10",        "1e2",     "12:3a"}) {
        BOOST_CHECK_THROW(Duration::parse(value), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(duration_parsing_limits_minutes_and_seconds_to_59) {
    ECF_NAME_THIS_TEST();

    auto seconds = [](const std::string& value) { return Duration::parse(value).as_seconds().count(); };

    BOOST_CHECK_EQUAL(seconds("00:59:59"), 3599);
    BOOST_CHECK_EQUAL(seconds("0:0:59"), 59);
    BOOST_CHECK_EQUAL(seconds("00:-59:59"), -3599);
    BOOST_CHECK_EQUAL(seconds("1:2:3"), 3723);
    BOOST_CHECK_EQUAL(seconds("007:05:09"), 25509);

    for (const std::string value : {"00:60", "00:60:00", "00:00:60", "00:90:00", "00:-60:00", "0:0:-60", "1:100:00"}) {
        BOOST_CHECK_THROW(Duration::parse(value), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(duration_parsing_refuses_hours_beyond_the_range_of_seconds) {
    ECF_NAME_THIS_TEST();

    auto seconds = [](const std::string& value) { return Duration::parse(value).as_seconds().count(); };

    // The largest number of seconds is 9223372036854775807, i.e. 2562047788015215 hours, 30 minutes and 7 seconds
    BOOST_CHECK_EQUAL(seconds("4800:00:00"), 17280000);
    BOOST_CHECK_EQUAL(seconds("2562047788015215:30:07"), 9223372036854775807L);
    BOOST_CHECK_EQUAL(seconds("-2562047788015215:30:07"), -9223372036854775807L);

    for (const std::string value :
         {"2562047788015215:30:08", "2562047788015216", "-2562047788015216", "99999999999999999999"}) {
        BOOST_CHECK_THROW(Duration::parse(value), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(instants_beyond_the_range_of_32_bit_seconds_round_trip) {
    ECF_NAME_THIS_TEST();

    // The range of 32-bit seconds since the epoch is [19011213T204552, 20380119T031407]. The values stay within the
    // range of std::chrono::system_clock with libstdc++ (nanoseconds, [16770922, 22620411]); libc++ uses microseconds
    for (const std::string value : {"16780101T000000",
                                    "18000101T000000",
                                    "19011213T204551",
                                    "19011213T204552",
                                    "19700101T000000",
                                    "20380119T031407",
                                    "20380119T031408",
                                    "21000101T000000",
                                    "22611231T235959"}) {
        BOOST_CHECK_EQUAL(Instant::format(Instant::parse(value)), value);
    }

    BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into_seconds(Instant::parse("20380119T031408")), 2147483648L);
    BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into_seconds(Instant::parse("19011213T204551")), -2147483649L);
    BOOST_CHECK_EQUAL(ecf::coerce_from_instant_into_seconds(Instant::parse("19700101T000000")), 0L);
}

#if defined(_LIBCPP_VERSION)
BOOST_AUTO_TEST_CASE(instant_parsing_is_lenient_with_libcxx) {
    ECF_NAME_THIS_TEST();

    // No check is made for the end of the input, and libc++ stops at the end of the input after any complete field
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse("20240101T000000Z")), "20240101T000000");
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse(" 20240101T000000")), "20240101T000000");
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse("20240101")), "20240101T000000");
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse("20240101T1")), "20240101T010000");
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse("20240101t000000")), "20240101T000000");
    BOOST_CHECK_EQUAL(Instant::format(Instant::parse("20240101T235960")), "20240102T000000");
    BOOST_CHECK_THROW(Instant::parse("20240101T"), std::runtime_error);
    BOOST_CHECK_THROW(Instant::parse("202401"), std::runtime_error);
}
#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

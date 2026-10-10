// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Pins the behaviour of changing the current value of each kind of Repeat
///        (Repeat::change and Repeat::changeValue), as used by `--alter change repeat`.
///

namespace {

///
/// @brief Enables the server-side change numbers for the lifetime of the object.
///
/// The change number of a Repeat is only incremented when running as a server.
///
struct ServerScope
{
    ServerScope() { Ecf::set_server(true); }
    ~ServerScope() { Ecf::set_server(false); }
    ServerScope(const ServerScope&)            = delete;
    ServerScope& operator=(const ServerScope&) = delete;
};

///
/// @brief Checks that changing the Repeat to the given value succeeds.
///
/// @param[in,out] rep the Repeat to change
/// @param[in] value the new value, as given to Repeat::change
/// @param[in] expected_index_or_value the expected index_or_value() after the change
/// @param[in] expected_as_string the expected valueAsString() after the change
///
void check_changed(Repeat& rep,
                   const std::string& value,
                   long expected_index_or_value,
                   const std::string& expected_as_string) {
    ServerScope server;
    auto before = rep.state_change_no();

    BOOST_REQUIRE_NO_THROW(rep.change(value));

    BOOST_CHECK_EQUAL(rep.index_or_value(), expected_index_or_value);
    BOOST_CHECK_EQUAL(rep.valueAsString(), expected_as_string);
    BOOST_CHECK_MESSAGE(rep.state_change_no() > before, "expected the change number to increase for " << value);
}

///
/// @brief Checks that changing the Repeat to the given value is refused, and leaves the Repeat unchanged.
///
/// @param[in,out] rep the Repeat to change
/// @param[in] value the new value, as given to Repeat::change
/// @param[in] fragment a fragment expected in the error message
///
void check_refused(Repeat& rep, const std::string& value, const std::string& fragment) {
    ServerScope server;
    auto before_change_no = rep.state_change_no();
    auto before_value     = rep.index_or_value();
    auto before_repeat    = rep.toString();

    std::string error;
    try {
        rep.change(value);
    }
    catch (const std::runtime_error& e) {
        error = e.what();
    }

    BOOST_CHECK_MESSAGE(!error.empty(), "expected change(" << value << ") to be refused for " << before_repeat);
    BOOST_CHECK_MESSAGE(error.find(fragment) != std::string::npos,
                        "expected '" << fragment << "' in the error for change(" << value << "), but found: " << error);
    BOOST_CHECK_EQUAL(rep.index_or_value(), before_value);
    BOOST_CHECK_EQUAL(rep.toString(), before_repeat);
    BOOST_CHECK_EQUAL(rep.state_change_no(), before_change_no);
}

///
/// @brief Retrieves the value of a generated variable of the Repeat.
///
std::string gen_value(const Repeat& rep, const std::string& name) {
    const Variable& var = rep.find_gen_variable(name);
    BOOST_CHECK_MESSAGE(!var.empty(), "expected generated variable " << name);
    return var.value();
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_RepeatChange)

/*
 * Test Suite: ::repeat_date
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_date)

BOOST_AUTO_TEST_CASE(change_within_range_and_on_grid_is_accepted) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    check_changed(rep, "20150730", 20150730, "20150730"); // end
    check_changed(rep, "20150514", 20150514, "20150514"); // start
    check_changed(rep, "20150604", 20150604, "20150604"); // over a month boundary, 3 steps from start

    Repeat descending(RepeatDate("YMD", 20150730, 20150514, -7));
    check_changed(descending, "20150514", 20150514, "20150514"); // end
    check_changed(descending, "20150730", 20150730, "20150730"); // start
    check_changed(descending, "20150709", 20150709, "20150709"); // 3 steps from start
}

BOOST_AUTO_TEST_CASE(change_with_start_equal_to_end) {
    ECF_NAME_THIS_TEST();

    for (int step : {1, -1}) {
        Repeat rep(RepeatDate("YMD", 20200101, 20200101, step));
        check_changed(rep, "20200101", 20200101, "20200101");
        check_refused(rep, "20200102", "should be in the range");
        check_refused(rep, "20191231", "should be in the range");

        rep.increment();
        BOOST_CHECK(!rep.valid());
    }
}

BOOST_AUTO_TEST_CASE(change_with_malformed_value_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    check_refused(rep, "", "expected 8 characters");
    check_refused(rep, "2015051", "expected 8 characters");
    check_refused(rep, "201505140", "expected 8 characters");
    check_refused(rep, "2015051x", "is not convertible to an long");
    check_refused(rep, "20150230", "is not valid"); // no 30th of February
    check_refused(rep, "20151301", "is not valid"); // no 13th month
}

BOOST_AUTO_TEST_CASE(change_outside_range_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    check_refused(rep, "20150507", "should be in the range"); // one step before start
    check_refused(rep, "20150806", "should be in the range"); // one step after end

    Repeat descending(RepeatDate("YMD", 20150730, 20150514, -7));
    check_refused(descending, "20150806", "should be in the range");
    check_refused(descending, "20150507", "should be in the range");
}

BOOST_AUTO_TEST_CASE(change_off_the_step_grid_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    check_refused(rep, "20150515", "is not in line with the delta/step");
    check_refused(rep, "20150729", "is not in line with the delta/step");

    Repeat descending(RepeatDate("YMD", 20150730, 20150514, -7));
    check_refused(descending, "20150729", "is not in line with the delta/step");
    check_refused(descending, "20150515", "is not in line with the delta/step");
}

BOOST_AUTO_TEST_CASE(set_value_bypasses_validation) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    rep.set_value(20150515); // off the grid
    BOOST_CHECK_EQUAL(rep.value(), 20150515);
    rep.set_value(20160101); // out of range
    BOOST_CHECK_EQUAL(rep.value(), 20160101);
    BOOST_CHECK(!rep.valid());
    BOOST_CHECK_EQUAL(rep.last_valid_value(), 20150730);
}

BOOST_AUTO_TEST_CASE(change_updates_generated_variables) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20150514, 20150730, 7));
    rep.update_repeat_genvar();
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD"), "20150514");

    rep.change("20150604");

    BOOST_CHECK_EQUAL(gen_value(rep, "YMD_YYYY"), "2015");
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD_MM"), "06");
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD_DD"), "04");
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD_DOW"), "4"); // Thursday
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD_JULIAN"),
                      std::to_string(ecf::CalendarDate(20150604).as_julian_day().value()));

    // The variable named after the Repeat is only refreshed by update_repeat_genvar()
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD"), "20150514");
    rep.update_repeat_genvar();
    BOOST_CHECK_EQUAL(gen_value(rep, "YMD"), "20150604");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_date

/*
 * Test Suite: ::repeat_datetime
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_datetime)

BOOST_AUTO_TEST_CASE(change_within_range_and_on_grid_is_accepted) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    auto seconds = [](const char* s) { return ecf::coerce_from_instant_into_seconds(Instant::parse(s)); };

    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
    check_changed(rep, "20240102T000000", seconds("20240102T000000"), "20240102T000000"); // end
    check_changed(rep, "20240101T000000", seconds("20240101T000000"), "20240101T000000"); // start
    check_changed(rep, "20240101T180000", seconds("20240101T180000"), "20240101T180000");

    Repeat descending(RepeatDateTime("DT", "20240102T000000", "20240101T000000", "-06:00:00"));
    check_changed(descending, "20240101T000000", seconds("20240101T000000"), "20240101T000000"); // end
    check_changed(descending, "20240102T000000", seconds("20240102T000000"), "20240102T000000"); // start
    check_changed(descending, "20240101T060000", seconds("20240101T060000"), "20240101T060000");
}

BOOST_AUTO_TEST_CASE(change_with_start_equal_to_end) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20240101T120000", "20240101T120000", "01:00:00"));
    check_changed(rep,
                  "20240101T120000",
                  ecf::coerce_from_instant_into_seconds(ecf::Instant::parse("20240101T120000")),
                  "20240101T120000");
    check_refused(rep, "20240101T130000", "should be in the range");
}

BOOST_AUTO_TEST_CASE(change_with_malformed_value_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
    check_refused(rep, "", "is not valid");
    check_refused(rep, "garbage", "is not valid");
}

BOOST_AUTO_TEST_CASE(change_outside_range_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
    check_refused(rep, "20231231T180000", "should be in the range");
    check_refused(rep, "20240102T060000", "should be in the range");

    Repeat descending(RepeatDateTime("DT", "20240102T000000", "20240101T000000", "-06:00:00"));
    check_refused(descending, "20240102T060000", "should be in the range");
    check_refused(descending, "20231231T180000", "should be in the range");
}

BOOST_AUTO_TEST_CASE(change_off_the_step_grid_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
    check_refused(rep, "20240101T030000", "is not in line with the delta/step");
    check_refused(rep, "20240101T000001", "is not in line with the delta/step");

    Repeat descending(RepeatDateTime("DT", "20240102T000000", "20240101T000000", "-06:00:00"));
    check_refused(descending, "20240101T210000", "is not in line with the delta/step");
}

BOOST_AUTO_TEST_CASE(change_updates_generated_variables) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
    rep.update_repeat_genvar();

    rep.change("20240101T180000");

    BOOST_CHECK_EQUAL(gen_value(rep, "DT_DATE"), "20240101");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_YYYY"), "2024");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_MM"), "01");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_DD"), "01");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_JULIAN"), std::to_string(ecf::CalendarDate(20240101).as_julian_day().value()));
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_TIME"), "180000");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_HOURS"), "18");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_MINUTES"), "00");
    BOOST_CHECK_EQUAL(gen_value(rep, "DT_SECONDS"), "00");

    // The variable named after the Repeat is only refreshed by update_repeat_genvar()
    BOOST_CHECK_EQUAL(gen_value(rep, "DT"), "20240101T000000");
    rep.update_repeat_genvar();
    BOOST_CHECK_EQUAL(gen_value(rep, "DT"), "20240101T180000");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_datetime

/*
 * Test Suite: ::repeat_datelist
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_datelist)

BOOST_AUTO_TEST_CASE(change_to_a_member_is_accepted) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateList("DL", {20240101, 20240215, 20240301}));
    check_changed(rep, "20240301", 2, "20240301"); // last
    check_changed(rep, "20240101", 0, "20240101"); // first
    check_changed(rep, "20240215", 1, "20240215");
}

BOOST_AUTO_TEST_CASE(change_to_a_non_member_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateList("DL", {20240101, 20240215, 20240301}));
    check_refused(rep, "", "must be convertible to integer");
    check_refused(rep, "abc", "must be convertible to integer");
    check_refused(rep, "20240102", "is not a valid member of the date list");
    check_refused(rep, "1", "is not a valid member of the date list"); // an index is not a member
}

BOOST_AUTO_TEST_CASE(change_value_by_index) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateList("DL", {20240101, 20240215, 20240301}));
    rep.changeValue(2);
    BOOST_CHECK_EQUAL(rep.index_or_value(), 2);
    BOOST_CHECK_THROW(rep.changeValue(3), std::runtime_error);
    BOOST_CHECK_THROW(rep.changeValue(-1), std::runtime_error);
    BOOST_CHECK_EQUAL(rep.index_or_value(), 2);
}

BOOST_AUTO_TEST_CASE(change_updates_generated_variables) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateList("DL", {20240101, 20240215, 20240301}));
    rep.update_repeat_genvar();

    rep.change("20240215");

    BOOST_CHECK_EQUAL(gen_value(rep, "DL_YYYY"), "2024");
    BOOST_CHECK_EQUAL(gen_value(rep, "DL_MM"), "02");
    BOOST_CHECK_EQUAL(gen_value(rep, "DL_DD"), "15");
    BOOST_CHECK_EQUAL(gen_value(rep, "DL_DOW"), "4"); // Thursday
    BOOST_CHECK_EQUAL(gen_value(rep, "DL_JULIAN"), std::to_string(ecf::CalendarDate(20240215).as_julian_day().value()));
}

BOOST_AUTO_TEST_SUITE_END() // repeat_datelist

/*
 * Test Suite: ::repeat_datetimelist
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_datetimelist)

BOOST_AUTO_TEST_CASE(change_to_a_member_is_accepted) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    Repeat rep(RepeatDateTimeList(
        "DTL",
        {Instant::parse("20240101T000000"), Instant::parse("20240102T120000"), Instant::parse("20240103T060000")}));
    check_changed(rep, "20240103T060000", 2, "20240103T060000"); // last
    check_changed(rep, "20240101T000000", 0, "20240101T000000"); // first
}

BOOST_AUTO_TEST_CASE(change_to_a_non_member_is_refused) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    Repeat rep(RepeatDateTimeList("DTL", {Instant::parse("20240101T000000"), Instant::parse("20240102T120000")}));
    check_refused(rep, "garbage", "is not a valid datetime");
    check_refused(rep, "1", "is not a valid datetime"); // an index is not a member
    check_refused(rep, "20240101T000001", "is not a valid member of the datetimelist");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_datetimelist

/*
 * Test Suite: ::repeat_integer
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_integer)

BOOST_AUTO_TEST_CASE(change_within_range_is_accepted) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    check_changed(rep, "10", 10, "10"); // end
    check_changed(rep, "0", 0, "0");    // start
    check_changed(rep, "4", 4, "4");

    Repeat descending(RepeatInteger("N", 10, 0, -2));
    check_changed(descending, "0", 0, "0");    // end
    check_changed(descending, "10", 10, "10"); // start
    check_changed(descending, "6", 6, "6");
}

BOOST_AUTO_TEST_CASE(change_off_the_step_grid_is_accepted) {
    ECF_NAME_THIS_TEST();

    // Unlike date and datetime, an integer Repeat does not check the step grid
    Repeat rep(RepeatInteger("N", 0, 10, 3));
    check_changed(rep, "4", 4, "4");

    Repeat descending(RepeatInteger("N", 10, 0, -2));
    check_changed(descending, "5", 5, "5");
}

BOOST_AUTO_TEST_CASE(change_with_start_equal_to_end) {
    ECF_NAME_THIS_TEST();

    for (int step : {1, -1}) {
        Repeat rep(RepeatInteger("N", 5, 5, step));
        check_changed(rep, "5", 5, "5");
        check_refused(rep, "6", "should be in the range");
        check_refused(rep, "4", "should be in the range");
    }
}

BOOST_AUTO_TEST_CASE(change_with_malformed_value_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    check_refused(rep, "", "is not convertible to an long");
    check_refused(rep, "abc", "is not convertible to an long");
    check_refused(rep, "1.5", "is not convertible to an long");
}

BOOST_AUTO_TEST_CASE(change_outside_range_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    check_refused(rep, "-2", "should be in the range");
    check_refused(rep, "12", "should be in the range");

    Repeat descending(RepeatInteger("N", 10, 0, -2));
    check_refused(descending, "12", "should be in the range");
    check_refused(descending, "-2", "should be in the range");
}

BOOST_AUTO_TEST_CASE(descending_iterates_to_completion) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 10, 1, -1));
    std::vector<long> visited;
    while (rep.valid()) {
        visited.push_back(rep.value());
        rep.increment();
    }

    const std::vector<long> expected{10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    BOOST_CHECK_EQUAL_COLLECTIONS(visited.begin(), visited.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(rep.value(), 0);
    BOOST_CHECK_EQUAL(rep.last_valid_value(), 1);

    rep.reset();
    BOOST_CHECK_EQUAL(rep.value(), 10);
}

BOOST_AUTO_TEST_CASE(inconsistent_ordering_is_accepted_at_construction) {
    ECF_NAME_THIS_TEST();

    // The sign of the step disagrees with the order of start and end:
    // the Repeat is complete from the start, and no value is accepted
    {
        Repeat rep(RepeatInteger("N", 0, 10, -1));
        BOOST_CHECK(!rep.valid());
        check_refused(rep, "0", "should be in the range");
        check_refused(rep, "5", "should be in the range");
    }
    {
        Repeat rep(RepeatInteger("N", 10, 0, 1));
        BOOST_CHECK(!rep.valid());
        check_refused(rep, "10", "should be in the range");
        check_refused(rep, "5", "should be in the range");
    }
}

BOOST_AUTO_TEST_CASE(zero_step_is_accepted_at_construction) {
    ECF_NAME_THIS_TEST();

    {
        // start < end: complete from the start
        Repeat rep(RepeatInteger("N", 0, 10, 0));
        BOOST_CHECK(!rep.valid());
    }
    {
        // start >= end: never completes
        Repeat rep(RepeatInteger("N", 10, 0, 0));
        for (int i = 0; i < 5; ++i) {
            BOOST_CHECK(rep.valid());
            rep.increment();
        }
        BOOST_CHECK_EQUAL(rep.value(), 10);
    }
}

BOOST_AUTO_TEST_CASE(set_value_bypasses_validation) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    rep.set_value(11);
    BOOST_CHECK_EQUAL(rep.value(), 11);
    BOOST_CHECK(!rep.valid());
    BOOST_CHECK_EQUAL(rep.last_valid_value(), 10);
}

BOOST_AUTO_TEST_CASE(change_does_not_refresh_the_generated_variable) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    rep.update_repeat_genvar();
    rep.change("4");

    BOOST_CHECK_EQUAL(gen_value(rep, "N"), "0");
    rep.update_repeat_genvar();
    BOOST_CHECK_EQUAL(gen_value(rep, "N"), "4");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_integer

/*
 * Test Suite: ::repeat_enumerated
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_enumerated)

BOOST_AUTO_TEST_CASE(change_by_name_or_index_is_accepted) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatEnumerated("E", {"a", "b", "c"}));
    check_changed(rep, "c", 2, "c"); // last, by name
    check_changed(rep, "a", 0, "a"); // first, by name
    check_changed(rep, "1", 1, "b"); // by index
    check_changed(rep, "2", 2, "c"); // last, by index
    check_changed(rep, "0", 0, "a"); // first, by index
}

BOOST_AUTO_TEST_CASE(change_prefers_a_member_over_an_index) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatEnumerated("E", {"10", "20", "1"}));
    check_changed(rep, "1", 2, "1"); // the member "1", not the index 1
    BOOST_CHECK_EQUAL(rep.value(), 1);
    check_changed(rep, "0", 0, "10"); // no member "0": the index 0
    BOOST_CHECK_EQUAL(rep.value(), 10);
}

BOOST_AUTO_TEST_CASE(change_to_unknown_name_or_index_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatEnumerated("E", {"a", "b", "c"}));
    check_refused(rep, "z", "is not a valid index or a member of the enumerated list");
    check_refused(rep, "", "is not a valid index or a member of the enumerated list");
    check_refused(rep, "3", "is not a valid index");
    check_refused(rep, "-1", "is not a valid index");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_enumerated

/*
 * Test Suite: ::repeat_string
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_string)

BOOST_AUTO_TEST_CASE(change_by_name_or_index_is_accepted) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatString("S", {"a", "b", "c"}));
    check_changed(rep, "c", 2, "c"); // last, by name
    check_changed(rep, "a", 0, "a"); // first, by name
    check_changed(rep, "1", 1, "b"); // by index
    check_changed(rep, "2", 2, "c"); // last, by index
    check_changed(rep, "0", 0, "a"); // first, by index
}

BOOST_AUTO_TEST_CASE(change_prefers_a_member_over_an_index) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatString("S", {"x", "y", "0"}));
    check_changed(rep, "0", 2, "0"); // the member "0", not the index 0
    check_changed(rep, "1", 1, "y"); // no member "1": the index 1
}

BOOST_AUTO_TEST_CASE(change_to_unknown_name_or_index_is_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatString("S", {"a", "b", "c"}));
    check_refused(rep, "z", "is not a valid index or member of the string list");
    check_refused(rep, "", "is not a valid index or member of the string list");
    check_refused(rep, "3", "is not a valid index");
    check_refused(rep, "-1", "is not a valid index");
}

BOOST_AUTO_TEST_SUITE_END() // repeat_string

/*
 * Test Suite: ::repeat_day
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_day)

BOOST_AUTO_TEST_CASE(change_is_ignored) {
    ECF_NAME_THIS_TEST();

    ServerScope server;
    Repeat rep(RepeatDay(2));
    auto before = rep.state_change_no();

    BOOST_CHECK_NO_THROW(rep.change("5"));
    BOOST_CHECK_NO_THROW(rep.change("garbage"));
    BOOST_CHECK_NO_THROW(rep.changeValue(5));

    BOOST_CHECK_EQUAL(rep.step(), 2);
    BOOST_CHECK_EQUAL(rep.value(), 2);
    BOOST_CHECK(rep.valid());
    BOOST_CHECK_EQUAL(rep.state_change_no(), before);
}

BOOST_AUTO_TEST_SUITE_END() // repeat_day

BOOST_AUTO_TEST_SUITE_END() // T_RepeatChange

BOOST_AUTO_TEST_SUITE_END() // U_Attributes

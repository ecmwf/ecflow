// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests the change of the begin, end, step and current value of a Repeat, given as options in the value of
///        `--alter change repeat` (e.g. "--begin 20260101 --end 20261231").
///

namespace {

///
/// @brief Enables the server-side change numbers for the lifetime of the object.
///
struct ServerScope
{
    ServerScope() { Ecf::set_server(true); }
    ~ServerScope() { Ecf::set_server(false); }
    ServerScope(const ServerScope&)            = delete;
    ServerScope& operator=(const ServerScope&) = delete;
};

///
/// @brief Describes the definition and the value of a Repeat, to compare a Repeat before and after a change.
///
std::string state_of(const Repeat& rep) {
    return rep.toString() + " value " + std::to_string(rep.index_or_value());
}

///
/// @brief Checks that the change is accepted, and gives the expected definition and value.
///
/// @param[in,out] rep the Repeat to change
/// @param[in] value the value given to the change
/// @param[in] definition the expected definition, as written in a definition file
/// @param[in] expected_value the expected index_or_value()
///
void check_changed(Repeat& rep, const std::string& value, const std::string& definition, long expected_value) {
    ServerScope server;
    auto before = rep.state_change_no();

    BOOST_REQUIRE_MESSAGE(
        [&]() {
            try {
                rep.change(value);
                return true;
            }
            catch (const std::exception& e) {
                BOOST_TEST_MESSAGE("change(" << value << ") refused: " << e.what());
                return false;
            }
        }(),
        "expected change(" << value << ") to be accepted");

    BOOST_CHECK_EQUAL(rep.toString(), "  repeat " + definition + "\n");
    BOOST_CHECK_EQUAL(rep.index_or_value(), expected_value);
    BOOST_CHECK_MESSAGE(rep.state_change_no() > before, "expected the change number to increase for " << value);
}

///
/// @brief Checks that the change is refused with the expected error, and leaves the Repeat unchanged.
///
void check_refused(Repeat& rep, const std::string& value, const std::string& fragment) {
    ServerScope server;
    auto before_change_no = rep.state_change_no();
    auto before           = state_of(rep);

    std::string error;
    try {
        rep.change(value);
    }
    catch (const std::runtime_error& e) {
        error = e.what();
    }

    BOOST_CHECK_MESSAGE(!error.empty(), "expected change(" << value << ") to be refused for " << before);
    BOOST_CHECK_MESSAGE(error.find(fragment) != std::string::npos,
                        "expected '" << fragment << "' in the error for change(" << value << "), but found: " << error);
    BOOST_CHECK_EQUAL(state_of(rep), before);
    BOOST_CHECK_EQUAL(rep.state_change_no(), before_change_no);
}

long seconds(const char* instant) {
    return ecf::coerce_from_instant_into_seconds(ecf::Instant::parse(instant));
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_RepeatAlter)

/*
 * Test Suite: ::options
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(options)

BOOST_AUTO_TEST_CASE(malformed_option_strings_are_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 1));
    check_refused(rep, "--", "unknown option '--'");
    check_refused(rep, "--foo 1", "unknown option '--foo'");
    check_refused(rep, "--begin 1 --foo 2", "unknown option '--foo'");
    check_refused(rep, "--begin", "the option --begin expects a value");
    check_refused(rep, "--begin 1 --end", "the option --end expects a value");
    // an option name is never the value of another option
    check_refused(rep, "--end --begin 5", "the option --end expects a value");
    check_refused(rep, "--begin --end 5", "the option --begin expects a value");
    check_refused(rep, "--begin --step 1", "the option --begin expects a value");
    check_refused(rep, "--begin --current 5", "the option --begin expects a value");
    check_refused(rep, "--begin 1 --begin 2", "the option --begin is given more than once");
    check_refused(rep, "--current 1 --current 2", "the option --current is given more than once");
    check_refused(rep, "--begin 1 2", "unknown option '2'"); // one value per option
}

BOOST_AUTO_TEST_CASE(options_may_be_given_in_any_order_with_any_spacing) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"--begin 2 --end 8 --current 4",
                                    "--current 4 --end 8 --begin 2",
                                    "--end   8  --begin 2\t--current 4  ",
                                    "--end 8 --current 4 --begin 2"}) {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_changed(rep, value, "integer N 2 8", 4);
    }
}

BOOST_AUTO_TEST_CASE(plain_value_and_current_option_are_equivalent) {
    ECF_NAME_THIS_TEST();

    Repeat plain(RepeatInteger("N", 0, 10, 2));
    Repeat option(RepeatInteger("N", 0, 10, 2));
    check_changed(plain, "6", "integer N 0 10 2", 6);
    check_changed(option, "--current 6", "integer N 0 10 2", 6);
    BOOST_CHECK(plain == option);

    // ... including the errors
    check_refused(plain, "5", "is not in line with the delta/step");
    check_refused(option, "--current 5", "is not in line with the delta/step");
    check_refused(plain, "12", "should be in the range");
    check_refused(option, "--current 12", "should be in the range");
}

BOOST_AUTO_TEST_CASE(negative_plain_values_are_not_options) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, -10, -1));
    check_changed(rep, "-5", "integer N 0 -10 -1", -5);
    check_changed(rep, "--begin -2 --current -3", "integer N -2 -10 -1", -3);
}

BOOST_AUTO_TEST_CASE(option_strings_at_the_edges_of_the_syntax) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 1));
    check_refused(rep, "--begin --end 5", "the option --begin expects a value");
    check_refused(rep, "--begin=5", "unknown option '--begin=5'");
    check_refused(rep, "--step 99999999999999999999", "is out of the range of integers");
    check_changed(rep, " --begin 2 --end 8", "integer N 2 8", 2); // leading white space
    check_changed(rep, "--begin 1\n--end 9", "integer N 1 9", 2); // separated by a new line
    check_changed(rep, "\t--current 3\n", "integer N 1 9", 3);
}

BOOST_AUTO_TEST_CASE(current_option_alone_behaves_as_a_plain_value) {
    ECF_NAME_THIS_TEST();

    // Even on a Repeat that could not be defined today (restored from a checkpoint), --current alone changes the
    // value exactly as the plain value does, without checking the rest of the definition
    {
        Repeat rep(RepeatInteger("N", 10, 0, 0)); // a zero step
        check_changed(rep, "--current 5", "integer N 10 0 0", 5);
        check_changed(rep, "6", "integer N 10 0 0", 6);
    }
    {
        Repeat rep(RepeatInteger("N", 0, std::numeric_limits<int>::max(), 1)); // a completion beyond an int
        check_changed(rep, "--current 5", "integer N 0 2147483647", 5);
    }
}

BOOST_AUTO_TEST_SUITE_END() // options

/*
 * Test Suite: ::repeat_integer
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_integer)

BOOST_AUTO_TEST_CASE(each_option_alone) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_changed(rep, "--begin 3", "integer N 3 10", 3); // the value 0 is clamped to the new begin
    }
    {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        rep.change("5");
        check_changed(rep, "--end 20", "integer N 0 20", 5); // the value stays
        check_changed(rep, "--end 4", "integer N 0 4", 4);   // the value 5 is clamped to the new end
    }
    {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        rep.change("7");
        check_changed(rep, "--step 3", "integer N 0 10 3", 6); // the value 7 moves towards the begin, onto the grid
    }
    {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_changed(rep, "--current 9", "integer N 0 10", 9);
    }
}

BOOST_AUTO_TEST_CASE(explicit_current_is_validated_against_the_new_range_and_never_clamped) {
    ECF_NAME_THIS_TEST();

    // The three examples of the task: begin 10 and current 12, with ends 15, 11 and 5
    {
        Repeat rep(RepeatInteger("N", 0, 15, 1));
        check_changed(rep, "--begin 10 --current 12", "integer N 10 15", 12);
    }
    {
        Repeat rep(RepeatInteger("N", 0, 11, 1));
        check_refused(rep, "--begin 10 --current 12", "should be in the range");
    }
    {
        Repeat rep(RepeatInteger("N", 0, 5, 1));
        check_refused(rep, "--begin 10 --current 12", "the end cannot be less than the start");
    }
    {
        // the explicit value must lie on the new grid
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_refused(rep, "--step 3 --current 7", "is not in line with the delta/step");
        check_changed(rep, "--step 3 --current 9", "integer N 0 10 3", 9);
    }
}

BOOST_AUTO_TEST_CASE(invalid_results_are_refused_and_leave_the_repeat_unchanged) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 0, 10, 2));
    rep.change("4");
    check_refused(rep, "--step 0", "the step cannot be zero");
    check_refused(rep, "--end -1", "the end cannot be less than the start, when the step is positive");
    check_refused(rep, "--step -1", "the start cannot be less than the end, when the step is negative");
    check_refused(rep, "--begin 20 --current 4", "the end cannot be less than the start");
    check_refused(rep, "--begin abc", "the value of --begin (abc) is not an integer");
    check_refused(rep, "--end 1.5", "the value of --end (1.5) is not an integer");
    check_refused(rep, "--step 2147483648", "the value of --step (2147483648) is out of the range of integers");
    check_refused(rep, "--begin -2147483649", "the value of --begin (-2147483649) is out of the range of integers");
    check_refused(rep,
                  "--begin -99999999999999999999",
                  "the value of --begin (-99999999999999999999) is out of the range of integers");
    check_refused(rep,
                  "--end +99999999999999999999",
                  "the value of --end (+99999999999999999999) is out of the range of integers");
    check_refused(rep, "--end 99999999999999999999x", "the value of --end (99999999999999999999x) is not an integer");
    check_refused(rep, "--current x", "the value of --current (x) is not an integer");
    // one valid option does not save the others
    check_refused(rep, "--end 20 --step 0", "the step cannot be zero");
}

BOOST_AUTO_TEST_CASE(completion_value_must_fit_an_int) {
    ECF_NAME_THIS_TEST();

    constexpr int max = std::numeric_limits<int>::max();
    constexpr int min = std::numeric_limits<int>::min();

    Repeat rep(RepeatInteger("N", 0, 10, 1));
    check_refused(rep, "--end " + std::to_string(max), "the value completing the Repeat (2147483648)");
    check_refused(
        rep, "--begin -5 --end " + std::to_string(min) + " --step -1", "the value completing the Repeat (-2147483649)");
    // with an end one short of the limit, a step of 1 completes on the limit
    check_changed(rep, "--end " + std::to_string(max - 1), "integer N 0 2147483646", 0);
    // the last value 0 (the only one), completed by a step to the limit
    check_changed(rep, "--step " + std::to_string(max), "integer N 0 2147483646 2147483647", 0);
}

BOOST_AUTO_TEST_CASE(descending_repeat) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatInteger("N", 10, 0, -2));
    check_changed(rep, "--begin 8", "integer N 8 0 -2", 8); // the value 10 is clamped to the new begin
    rep.change("4");
    check_changed(rep, "--end 6", "integer N 8 6 -2", 6);           // the value 4 is clamped to the new end
    check_changed(rep, "--begin 9 --end 0", "integer N 9 0 -2", 7); // 6 lies off the grid of 9: moved to 7
    check_changed(rep, "--step -3", "integer N 9 0 -3", 9); // 7 lies off the grid (9, 6, ...): moved towards the begin
}

BOOST_AUTO_TEST_CASE(completed_repeat) {
    ECF_NAME_THIS_TEST();

    {
        // Extending the end brings the value past the end back into the range
        Repeat rep(RepeatInteger("N", 0, 4, 2));
        while (rep.valid()) {
            rep.increment();
        }
        BOOST_REQUIRE_EQUAL(rep.value(), 6);
        check_changed(rep, "--end 10", "integer N 0 10 2", 6);
        BOOST_CHECK(rep.valid());
    }
    {
        // A change that does not continue the range keeps the Repeat finished, one step past the new last value
        Repeat rep(RepeatInteger("N", 0, 6, 1));
        while (rep.valid()) {
            rep.increment();
        }
        BOOST_REQUIRE_EQUAL(rep.value(), 7);
        check_changed(rep, "--begin 2", "integer N 2 6", 7); // 7 is still the completion value
        BOOST_CHECK(!rep.valid());
        check_changed(rep, "--step 2", "integer N 2 6 2", 8);
        BOOST_CHECK(!rep.valid());
        check_changed(rep, "--end 3", "integer N 2 3 2", 4);
        BOOST_CHECK(!rep.valid());
        check_changed(rep, "--end 20 --step 3", "integer N 2 20 3", 23); // 4 lies off the new grid
        BOOST_CHECK(!rep.valid());
        check_changed(rep, "--end 30", "integer N 2 30 3", 23); // 23 lies on the grid: the Repeat continues
        BOOST_CHECK(rep.valid());
    }
    {
        // An explicit current value restarts a finished Repeat
        Repeat rep(RepeatInteger("N", 0, 4, 2));
        while (rep.valid()) {
            rep.increment();
        }
        check_changed(rep, "--step 1 --current 3", "integer N 0 4", 3);
        BOOST_CHECK(rep.valid());
    }
    {
        // A descending Repeat stays finished below its new end
        Repeat rep(RepeatInteger("N", 10, 0, -2));
        while (rep.valid()) {
            rep.increment();
        }
        BOOST_REQUIRE_EQUAL(rep.value(), -2);
        check_changed(rep, "--end 3", "integer N 10 3 -2", 2);
        BOOST_CHECK(!rep.valid());
    }
}

BOOST_AUTO_TEST_CASE(limits_of_the_range) {
    ECF_NAME_THIS_TEST();

    {
        // a step larger than the range: only the begin runs
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        rep.change("7");
        check_changed(rep, "--step 20", "integer N 0 10 20", 0);
    }
    {
        // begin equal to end, with either sign of the step
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_changed(rep, "--begin 5 --end 5", "integer N 5 5", 5);
        check_changed(rep, "--step -1", "integer N 5 5 -1", 5);
    }
    {
        // the smallest step completes on the lower limit of the integers
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        check_changed(rep, "--end 0 --step -2147483648", "integer N 0 0 -2147483648", 0);
    }
}

BOOST_AUTO_TEST_SUITE_END() // repeat_integer

/*
 * Test Suite: ::repeat_date
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_date)

BOOST_AUTO_TEST_CASE(each_option_alone) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
        check_changed(rep, "--begin 20260201", "date YMD 20260201 20261231 1", 20260201);
    }
    {
        Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
        rep.change("20260315");
        check_changed(rep, "--end 20260630", "date YMD 20260101 20260630 1", 20260315);
        check_changed(rep, "--end 20260301", "date YMD 20260101 20260301 1", 20260301);
    }
    {
        // a change of step across a month: 20260105 is 4 days from the begin, moved back to 0 weeks
        Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
        rep.change("20260105");
        check_changed(rep, "--step 7", "date YMD 20260101 20261231 7", 20260101);
        rep.change("20260205");                                                     // 5 weeks from the begin
        check_changed(rep, "--step 14", "date YMD 20260101 20261231 14", 20260129); // 2 x 14 days
    }
    {
        Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
        check_changed(rep, "--current 20261012", "date YMD 20260101 20261231 1", 20261012);
    }
}

BOOST_AUTO_TEST_CASE(all_options_together) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20250101, 20251231, 1));
    check_changed(
        rep, "--begin 20260101 --end 20261231 --step 7 --current 20260115", "date YMD 20260101 20261231 7", 20260115);
    // a new begin after the old end is fine when the end moves too
    check_changed(rep, "--end 20271231 --begin 20270101", "date YMD 20270101 20271231 7", 20270101);
}

BOOST_AUTO_TEST_CASE(invalid_results_are_refused_and_leave_the_repeat_unchanged) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20260101, 20261231, 7));
    rep.change("20260115");
    check_refused(rep, "--begin 2026010", "the value of --begin (2026010) is not a valid date");
    check_refused(rep, "--begin 2026010x", "is not an integer");
    check_refused(rep, "--end 20260230", "the value of --end (20260230) is not a valid date");
    check_refused(rep, "--end 20251231", "The end must be greater than the start date");
    check_refused(rep, "--step 0", "the delta cannot be zero");
    check_refused(rep, "--step -1", "The start must be greater than the end date");
    check_refused(rep, "--step x", "the value of --step (x) is not an integer");
    check_refused(rep, "--current 20260116", "is not in line with the delta/step");
    check_refused(rep, "--current 20270101", "should be in the range");
    check_refused(rep, "--current 2026", "is not a valid date");
}

BOOST_AUTO_TEST_CASE(descending_repeat) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20260131, 20260101, -7)); // 31, 24, 17, 10, 3
    rep.change("20260117");
    check_changed(rep, "--begin 20260130", "date YMD 20260130 20260101 -7", 20260123); // 17 moves to 30 - 7
    // 23 is clamped to the end 25, which lies off the grid (30, 23, ...): moved towards the begin
    check_changed(rep, "--end 20260125", "date YMD 20260130 20260125 -7", 20260130);
}

BOOST_AUTO_TEST_CASE(generated_variables_follow_the_change) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
    rep.update_repeat_genvar();
    rep.change("--begin 20260315");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_MM").value(), "03");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_DD").value(), "15");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_DOW").value(), "0"); // Sunday
    BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_JULIAN").value(),
                      std::to_string(ecf::CalendarDate(20260315).as_julian_day().value()));
}

BOOST_AUTO_TEST_CASE(limits_of_the_range) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
    rep.change("20260704");
    check_changed(rep, "--step 400", "date YMD 20260101 20261231 400", 20260101); // a step larger than the range
}

BOOST_AUTO_TEST_CASE(completed_repeat) {
    ECF_NAME_THIS_TEST();

    {
        // extended: the value past the end lies on the grid of the new range, and the Repeat continues
        Repeat rep(RepeatDate("YMD", 20260101, 20260110, 7)); // 1, 8
        rep.update_repeat_genvar();
        while (rep.valid()) {
            rep.increment();
        }
        BOOST_REQUIRE_EQUAL(rep.value(), 20260115);
        check_changed(rep, "--end 20260131", "date YMD 20260101 20260131 7", 20260115);
        BOOST_CHECK(rep.valid());
        BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_DD").value(), "15");
        BOOST_CHECK_EQUAL(rep.find_gen_variable("YMD_DOW").value(), "4"); // Thursday
    }
    {
        // any other change keeps it finished, one step past the new last value
        Repeat rep(RepeatDate("YMD", 20260101, 20260110, 7));
        while (rep.valid()) {
            rep.increment();
        }
        check_changed(rep, "--step 2", "date YMD 20260101 20260110 2", 20260111); // 1, 3, 5, 7, 9
        BOOST_CHECK(!rep.valid());
        check_changed(rep, "--end 20260105", "date YMD 20260101 20260105 2", 20260107);
        BOOST_CHECK(!rep.valid());
    }
}

BOOST_AUTO_TEST_SUITE_END() // repeat_date

/*
 * Test Suite: ::repeat_datetime
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_datetime)

BOOST_AUTO_TEST_CASE(each_option_alone) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
        check_changed(rep,
                      "--begin 20260101T060000",
                      "datetime DT 20260101T060000 20260102T000000 06:00:00",
                      seconds("20260101T060000"));
    }
    {
        Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
        rep.change("20260101T180000");
        check_changed(rep,
                      "--end 20260101T120000",
                      "datetime DT 20260101T000000 20260101T120000 06:00:00",
                      seconds("20260101T120000"));
    }
    {
        Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
        rep.change("20260101T180000");
        check_changed(
            rep, "--step 04:00:00", "datetime DT 20260101T000000 20260102T000000 04:00:00", seconds("20260101T160000"));
        check_changed(rep,
                      "--step 1",
                      "datetime DT 20260101T000000 20260102T000000 01:00:00",
                      seconds("20260101T160000")); // a duration in hours
    }
    {
        Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
        check_changed(rep,
                      "--current 20260101T120000",
                      "datetime DT 20260101T000000 20260102T000000 06:00:00",
                      seconds("20260101T120000"));
    }
}

BOOST_AUTO_TEST_CASE(invalid_results_are_refused_and_leave_the_repeat_unchanged) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
    check_refused(rep, "--begin garbage", "the value of --begin (garbage) is not a valid date and time");
    check_refused(rep, "--end 20260132T000000", "the value of --end (20260132T000000) is not a valid date and time");
    check_refused(rep, "--step abc", "the value of --step (abc) is not a valid duration");
    check_refused(rep, "--step 00:00:00", "the delta cannot be zero");
    check_refused(rep, "--step -06:00:00", "The start must be greater than the end date+time");
    check_refused(rep, "--end 20251231T000000", "The end must be greater than the start date+time");
    check_refused(rep, "--current 20260101T030000", "is not in line with the delta/step");
    check_refused(rep, "--current 20260103T000000", "should be in the range");
}

BOOST_AUTO_TEST_CASE(descending_repeat) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20260102T000000", "20260101T000000", "-06:00:00"));
    rep.change("20260101T120000");
    check_changed(rep,
                  "--begin 20260101T200000",
                  "datetime DT 20260101T200000 20260101T000000 -6:00:00",
                  seconds("20260101T140000")); // 12:00 lies off the grid of 20:00: moved towards the begin
}

BOOST_AUTO_TEST_CASE(completed_repeat) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260101T120000", "06:00:00"));
    while (rep.valid()) {
        rep.increment();
    }
    BOOST_REQUIRE_EQUAL(rep.value(), seconds("20260101T180000"));
    check_changed(rep,
                  "--step 04:00:00",
                  "datetime DT 20260101T000000 20260101T120000 04:00:00",
                  seconds("20260101T160000")); // stays finished: one step past 12:00
    BOOST_CHECK(!rep.valid());
    check_changed(rep,
                  "--end 20260102T000000",
                  "datetime DT 20260101T000000 20260102T000000 04:00:00",
                  seconds("20260101T160000")); // continues: 16:00 lies on the new grid
    BOOST_CHECK(rep.valid());
}

BOOST_AUTO_TEST_CASE(generated_variables_follow_the_change) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
    rep.update_repeat_genvar();
    rep.change("--begin 20260101T013000 --end 20260103T000000");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("DT_DATE").value(), "20260101");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("DT_HOURS").value(), "01");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("DT_MINUTES").value(), "30");
    BOOST_CHECK_EQUAL(rep.find_gen_variable("DT_JULIAN").value(),
                      std::to_string(ecf::CalendarDate(20260101).as_julian_day().value()));
}

BOOST_AUTO_TEST_CASE(sign_of_the_step_applies_to_the_whole_duration) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
        check_refused(rep, "--step -0:30", "The start must be greater than the end date+time");
    }
    {
        Repeat rep(RepeatDateTime("DT", "20260102T000000", "20260101T000000", "-06:00:00"));
        rep.change("20260101T120000");
        check_changed(
            rep, "--step -0:30", "datetime DT 20260102T000000 20260101T000000 00:-30:00", seconds("20260101T120000"));
        check_changed(rep,
                      "--step 00:-45:00",
                      "datetime DT 20260102T000000 20260101T000000 00:-45:00",
                      seconds("20260101T120000"));
    }
}

BOOST_AUTO_TEST_CASE(malformed_steps_are_refused) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00"));
    for (const std::string step : {"00:90:00", "01:02:03:04", "01:-30:00", "1:"}) {
        check_refused(
            rep, "--step " + step, "the value of --step (" + step + ") is not a valid duration, expected H[:MM[:SS]]");
    }
}

BOOST_AUTO_TEST_SUITE_END() // repeat_datetime

/*
 * Test Suite: ::repeat_day
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(repeat_day)

BOOST_AUTO_TEST_CASE(only_the_step_applies) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDay(1));
    check_changed(rep, "--step 2", "day 2", 2);
    check_refused(rep, "--begin 1", "the option --begin does not apply to repeat day");
    check_refused(rep, "--end 1", "the option --end does not apply to repeat day");
    check_refused(rep, "--current 1", "the option --current does not apply to repeat day");
    check_refused(rep, "--step 3 --current 1", "the option --current does not apply to repeat day");
    check_refused(rep, "--step x", "the value of --step (x) is not an integer");

    check_refused(rep, "--step 3 --step 4", "the option --step is given more than once");

    // any step is accepted, as in a definition file
    check_changed(rep, "--step 0", "day 0", 0);
    check_changed(rep, "--step -1", "day -1", -1);
    check_changed(rep, "--step 2", "day 2", 2);

    // a plain value, empty included, is still ignored
    BOOST_CHECK_NO_THROW(rep.change(""));
    BOOST_CHECK_NO_THROW(rep.change("5"));
    BOOST_CHECK_EQUAL(rep.step(), 2);
}

BOOST_AUTO_TEST_SUITE_END() // repeat_day

/*
 * Test Suite: ::list_based_repeats
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(list_based_repeats)

BOOST_AUTO_TEST_CASE(only_current_is_an_option) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatString("S", {"a", "b", "--begin", "x y", "--currently"}));
    check_changed(rep, "--current b", R"(string S "a" "b" "--begin" "x y" "--currently")", 1);
    check_changed(rep, "--current x y", R"(string S "a" "b" "--begin" "x y" "--currently")", 3); // with a space
    check_changed(rep, "--begin", R"(string S "a" "b" "--begin" "x y" "--currently")", 2);       // a member
    check_changed(rep, "--current a", R"(string S "a" "b" "--begin" "x y" "--currently")", 0);
    check_changed(rep, "--current --begin", R"(string S "a" "b" "--begin" "x y" "--currently")", 2);
    check_changed(rep, "--currently", R"(string S "a" "b" "--begin" "x y" "--currently")", 4);  // not the option
    check_changed(rep, "--current 1", R"(string S "a" "b" "--begin" "x y" "--currently")", 1);  // an index
    check_changed(rep, "--current\ta", R"(string S "a" "b" "--begin" "x y" "--currently")", 0); // after a tab
}

BOOST_AUTO_TEST_CASE(range_options_that_match_no_member_are_refused_clearly) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatString("S", {"a", "b"}));
        check_refused(rep, "--begin 2026", "the option --begin does not apply to repeat string");
        check_refused(rep, "--end", "the option --end does not apply to repeat string");
        check_refused(rep, "--step 1", "the option --step does not apply to repeat string");
        check_refused(rep, "--step\t1", "the option --step does not apply to repeat string");
        check_refused(rep, "--current", "the option --current expects a value");
        check_refused(rep, "--current c", "is not a valid index or member of the string list");
        // a value merely starting with the name of an option is a plain value
        check_refused(rep, "--beginning", "is not a valid index or member of the string list");
        check_refused(rep, "--currently", "is not a valid index or member of the string list");
    }
    {
        Repeat rep(RepeatEnumerated("E", {"a", "b"}));
        check_changed(rep, "--current b", R"(enumerated E "a" "b")", 1);
        check_refused(rep, "--begin 2026", "the option --begin does not apply to repeat enumerated");
        check_refused(rep, "--current c", "is not a valid index or a member of the enumerated list");
    }
    {
        Repeat rep(RepeatDateList("DL", {20260101, 20260102}));
        check_changed(rep, "--current 20260102", R"(datelist DL "20260101" "20260102")", 1);
        check_refused(rep, "--begin 20260101", "the option --begin does not apply to repeat datelist");
        check_refused(rep, "--current 20260103", "is not a valid member of the date list");
        check_refused(rep, "--current", "the option --current expects a value");
    }
    {
        using ecf::Instant;
        Repeat rep(RepeatDateTimeList("DTL", {Instant::parse("20260101T000000"), Instant::parse("20260102T000000")}));
        check_changed(rep, "--current 20260102T000000", R"(datetimelist DTL "20260101T000000" "20260102T000000")", 1);
        check_refused(rep, "--step 1", "the option --step does not apply to repeat datetimelist");
        check_refused(rep, "--current 20260103T000000", "is not a valid member of the datetimelist");
    }
}

BOOST_AUTO_TEST_CASE(white_space_around_the_current_value) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatString("S", {"a", "b", "--current"}));
    check_changed(rep, "--current b ", R"(string S "a" "b" "--current")", 1); // trailing white space
    check_changed(rep, "--current\na", R"(string S "a" "b" "--current")", 0); // a new line
    check_changed(rep, "--current", R"(string S "a" "b" "--current")", 2);    // a member named --current
    check_changed(rep, "--current --current", R"(string S "a" "b" "--current")", 2);

    Repeat other(RepeatString("S", {"a", "b"}));
    check_refused(other, "--current ", "the option --current expects a value");
}

BOOST_AUTO_TEST_CASE(members_win_over_indexes) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatEnumerated("E", {"0", "1", "5"}));
    check_changed(rep, "--current 5", R"(enumerated E "0" "1" "5")", 2); // the member 5, not the index
    check_changed(rep, "--current 1", R"(enumerated E "0" "1" "5")", 1);
}

BOOST_AUTO_TEST_CASE(date_lists_refuse_other_values) {
    ECF_NAME_THIS_TEST();

    Repeat rep(RepeatDateList("DL", {20260101, 20260102}));
    check_refused(rep, "--current 2026", "is not a valid member of the date list");
    check_refused(rep, "--foo", "must be convertible to integer");
}

BOOST_AUTO_TEST_SUITE_END() // list_based_repeats

BOOST_AUTO_TEST_SUITE_END() // T_RepeatAlter

BOOST_AUTO_TEST_SUITE_END() // U_Attributes

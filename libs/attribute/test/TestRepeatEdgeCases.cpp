// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Pins the behaviour of Repeats at the edges: construction limits, serialisation, the value reported
///        at expiry, and the Repeat wrapper.
///

namespace {

///
/// @brief Saves the Repeat with cereal, and restores it into a new object.
///
Repeat cereal_round_trip(const Repeat& rep) {
    std::string text;
    ecf::save_as_string(text, rep);
    Repeat restored;
    ecf::restore_from_string(text, restored);
    return restored;
}

///
/// @brief Retrieves the message of the exception thrown by the given function, or an empty string.
///
template <typename F>
std::string error_of(F&& f) {
    try {
        f();
    }
    catch (const std::exception& e) {
        return e.what();
    }
    return std::string{};
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_RepeatEdgeCases)

/*
 * Test Suite: ::serialisation
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(serialisation)

BOOST_AUTO_TEST_CASE(round_trip_keeps_the_value_of_every_kind) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    std::vector<Repeat> repeats;

    {
        Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));
        rep.set_value(rep.end() + 6 * 3600); // past the end
        repeats.push_back(rep);
    }
    {
        Repeat rep(RepeatDateTime("DT", "20240102T000000", "20240101T000000", "00:-30:00"));
        rep.change("20240101T120000");
        repeats.push_back(rep);
    }
    {
        Repeat rep(RepeatDateTimeList("DTL", {Instant::parse("20240101T000000"), Instant::parse("20240102T000000")}));
        rep.set_value(-1); // before the first
        repeats.push_back(rep);
    }
    {
        Repeat rep(RepeatDateTimeList("DTL", {Instant::parse("20240101T000000"), Instant::parse("20240102T000000")}));
        rep.set_value(2); // past the last
        repeats.push_back(rep);
    }
    {
        Repeat rep(RepeatInteger("N", 10, 0, -2));
        rep.set_value(-2); // past the end of a descending Repeat
        repeats.push_back(rep);
    }
    repeats.emplace_back(RepeatDay(3));

    for (const auto& rep : repeats) {
        Repeat restored = cereal_round_trip(rep);
        BOOST_CHECK_MESSAGE(restored == rep,
                            "expected a round trip of " << rep.dump() << " but found " << restored.dump());
        BOOST_CHECK_EQUAL(restored.index_or_value(), rep.index_or_value());
    }
}

BOOST_AUTO_TEST_CASE(round_trip_of_datetime_loses_the_names_of_the_generated_variables) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;

    // The generated variables of a datetime Repeat are named when constructed, from the name of the Repeat, which
    // is still empty in the default constructor used by the deserialisation; a copy names them again
    for (const Repeat& rep : {Repeat(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00")),
                              Repeat(RepeatDateTimeList("DT", {Instant::parse("20240101T000000")}))}) {
        Repeat restored = cereal_round_trip(rep);
        restored.update_repeat_genvar();
        BOOST_CHECK(restored.find_gen_variable("DT_DATE").empty());
        BOOST_CHECK_EQUAL(restored.find_gen_variable("DT").value(), "20240101T000000");

        Repeat copy(restored);
        copy.update_repeat_genvar();
        BOOST_CHECK_EQUAL(copy.find_gen_variable("DT_DATE").value(), "20240101");
    }
}

BOOST_AUTO_TEST_SUITE_END() // serialisation

/*
 * Test Suite: ::expiry
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(expiry)

BOOST_AUTO_TEST_CASE(last_valid_value_is_the_end_even_when_off_the_step_grid) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatInteger("N", 1, 6, 2));
        std::vector<long> visited;
        while (rep.valid()) {
            visited.push_back(rep.value());
            rep.increment();
        }
        const std::vector<long> expected{1, 3, 5};
        BOOST_CHECK_EQUAL_COLLECTIONS(visited.begin(), visited.end(), expected.begin(), expected.end());
        BOOST_CHECK_EQUAL(rep.value(), 7);
        // the last valid value is 6, a value that never ran
        BOOST_CHECK_EQUAL(rep.last_valid_value(), 6);
        BOOST_CHECK_EQUAL(rep.valueAsString(), "6");
    }
    {
        Repeat rep(RepeatDate("YMD", 20260101, 20260110, 4));
        std::vector<long> visited;
        while (rep.valid()) {
            visited.push_back(rep.value());
            rep.increment();
        }
        const std::vector<long> expected{20260101, 20260105, 20260109};
        BOOST_CHECK_EQUAL_COLLECTIONS(visited.begin(), visited.end(), expected.begin(), expected.end());
        BOOST_CHECK_EQUAL(rep.value(), 20260113);
        BOOST_CHECK_EQUAL(rep.last_valid_value(), 20260110);
    }
}

BOOST_AUTO_TEST_CASE(set_to_last_value_is_the_end_even_when_off_the_step_grid) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatInteger("N", 0, 10, 3));
        rep.setToLastValue();
        BOOST_CHECK_EQUAL(rep.value(), 10);
        BOOST_CHECK(rep.valid());
        BOOST_CHECK_EQUAL(rep.current_index(), 3);
    }
    {
        Repeat rep(RepeatDate("YMD", 20200101, 20200110, 7));
        rep.setToLastValue();
        BOOST_CHECK_EQUAL(rep.value(), 20200110);
        BOOST_CHECK(rep.valid());
        BOOST_CHECK_EQUAL(rep.current_index(), 1);
    }
}

BOOST_AUTO_TEST_SUITE_END() // expiry

/*
 * Test Suite: ::construction
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(construction)

BOOST_AUTO_TEST_CASE(datetime_construction_errors) {
    ECF_NAME_THIS_TEST();

    const std::string start = "20240101T000000";
    const std::string end   = "20240102T000000";

    BOOST_CHECK_THROW(RepeatDateTime("DT", start, end, "00:00:00"), std::runtime_error); // zero step
    BOOST_CHECK_THROW(RepeatDateTime("DT", start, end, ""), std::runtime_error);         // parses as a zero step
    BOOST_CHECK_THROW(RepeatDateTime("DT", start, end, "abc"), std::runtime_error);
    BOOST_CHECK_THROW(RepeatDateTime("", start, end, "01:00:00"), std::runtime_error); // no name
    BOOST_CHECK_THROW(RepeatDateTime("DT", "garbage", end, "01:00:00"), std::runtime_error);

    auto ordering = error_of([&]() { RepeatDateTime("DT", end, start, "01:00:00"); });
    BOOST_CHECK_MESSAGE(ordering.find("The end must be greater than the start") != std::string::npos, ordering);
    ordering = error_of([&]() { RepeatDateTime("DT", start, end, "-01:00:00"); });
    BOOST_CHECK_MESSAGE(ordering.find("The start must be greater than the end") != std::string::npos, ordering);
}

BOOST_AUTO_TEST_CASE(datetime_construction_limits) {
    ECF_NAME_THIS_TEST();

    // start equal to end, with either sign of the step
    for (const std::string step : {"01:00:00", "-01:00:00"}) {
        Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240101T000000", step));
        BOOST_CHECK(rep.valid());
        rep.increment();
        BOOST_CHECK(!rep.valid());
    }

    // a step larger than the range runs once
    Repeat rep(RepeatDateTime("DT", "20240101T000000", "20240101T060000", "24:00:00"));
    BOOST_CHECK(rep.valid());
    rep.increment();
    BOOST_CHECK(!rep.valid());
    BOOST_CHECK_EQUAL(rep.valueAsString(), "20240101T060000");
}

BOOST_AUTO_TEST_CASE(date_calendar_limits) {
    ECF_NAME_THIS_TEST();

    // leap days
    BOOST_CHECK_NO_THROW(RepeatDate("D", 20240229, 20240301, 1));
    BOOST_CHECK_NO_THROW(RepeatDate("D", 20000229, 20000301, 1));
    BOOST_CHECK_THROW(RepeatDate("D", 20230229, 20230301, 1), std::runtime_error);
    BOOST_CHECK_THROW(RepeatDate("D", 21000229, 21000301, 1), std::runtime_error);

    // the range of years of the calendar
    BOOST_CHECK_THROW(RepeatDate("D", 13991231, 14000101, 1), std::runtime_error);
    BOOST_CHECK_NO_THROW(RepeatDate("D", 14000101, 14000102, 1));
    BOOST_CHECK_NO_THROW(RepeatDate("D", 99991230, 99991231, 1));

    // the same rules apply to the members of a date list
    BOOST_CHECK_NO_THROW(RepeatDateList("D", {20240229}));
    BOOST_CHECK_THROW(RepeatDateList("D", {20230229}), std::runtime_error);
    BOOST_CHECK_THROW(RepeatDateList("D", {13991231}), std::runtime_error);

    // ... and to a change of the value
    Repeat rep(RepeatDate("D", 20230101, 20231231, 1));
    BOOST_CHECK_THROW(rep.change("20230229"), std::runtime_error);
    Repeat leap(RepeatDate("D", 20240101, 20241231, 1));
    BOOST_CHECK_NO_THROW(leap.change("20240229"));
}

BOOST_AUTO_TEST_CASE(integer_extremes) {
    ECF_NAME_THIS_TEST();

    constexpr int max = std::numeric_limits<int>::max();
    constexpr int min = std::numeric_limits<int>::min();

    {
        Repeat rep(RepeatInteger("N", 0, max, 1));
        rep.setToLastValue();
        BOOST_CHECK_EQUAL(rep.value(), max);
        rep.increment();
        // the value is held as a long, so the increment past the end does not wrap
        BOOST_CHECK_EQUAL(rep.value(), static_cast<long>(max) + 1);
        BOOST_CHECK(!rep.valid());
        BOOST_CHECK_EQUAL(rep.last_valid_value(), max);
    }
    {
        Repeat rep(RepeatInteger("N", min, max, max));
        std::vector<long> visited;
        while (rep.valid()) {
            visited.push_back(rep.value());
            rep.increment();
        }
        const std::vector<long> expected{min, -1, max - 1};
        BOOST_CHECK_EQUAL_COLLECTIONS(visited.begin(), visited.end(), expected.begin(), expected.end());
        BOOST_CHECK_EQUAL(rep.current_index(), 3);
        BOOST_CHECK_EQUAL(rep.next_value_as_string(), std::to_string(max));
    }
}

BOOST_AUTO_TEST_CASE(duplicate_and_unsorted_members_are_accepted) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;

    {
        Repeat rep(RepeatDateList("D", {20200101, 20200102, 20200101}));
        rep.set_value(1);
        rep.change("20200101");
        BOOST_CHECK_EQUAL(rep.index_or_value(), 0); // the first match
    }
    {
        Repeat rep(RepeatEnumerated("E", {"a", "b", "a"}));
        rep.set_value(1);
        rep.change("a");
        BOOST_CHECK_EQUAL(rep.index_or_value(), 0);
    }
    {
        Repeat rep(RepeatString("S", {"a", "b", "a"}));
        rep.set_value(1);
        rep.change("a");
        BOOST_CHECK_EQUAL(rep.index_or_value(), 0);
    }
    {
        auto later   = Instant::parse("20240102T000000");
        auto earlier = Instant::parse("20240101T000000");
        Repeat rep(RepeatDateTimeList("DTL", {later, earlier}));
        // start and end are the first and the last members, in the order given
        BOOST_CHECK_EQUAL(rep.start(), ecf::coerce_from_instant_into_seconds(later));
        BOOST_CHECK_EQUAL(rep.end(), ecf::coerce_from_instant_into_seconds(earlier));
    }
}

BOOST_AUTO_TEST_CASE(repeat_wrapper_copies_and_compares) {
    ECF_NAME_THIS_TEST();

    {
        Repeat rep(RepeatInteger("N", 0, 10, 1));
        Repeat& alias = rep;
        rep           = alias; // self-assignment
        BOOST_CHECK(rep == Repeat(RepeatInteger("N", 0, 10, 1)));
    }
    {
        // the same name and values, but another kind
        BOOST_CHECK(!(Repeat(RepeatInteger("X", 0, 1, 1)) == Repeat(RepeatEnumerated("X", {"0", "1"}))));
        BOOST_CHECK(!(Repeat(RepeatEnumerated("X", {"0", "1"})) == Repeat(RepeatString("X", {"0", "1"}))));
    }
    {
        Ecf::set_server(true);
        Repeat rep(RepeatDate("YMD", 20260101, 20261231, 1));
        rep.update_repeat_genvar();
        rep.change("20260102");
        Ecf::set_server(false);
        BOOST_REQUIRE(rep.state_change_no() > 0);
        BOOST_REQUIRE(!rep.find_gen_variable("YMD_YYYY").empty());

        // a copy holds the same Repeat, but neither its change number nor its generated variables
        Repeat copy(rep);
        BOOST_CHECK(copy == rep);
        BOOST_CHECK_EQUAL(copy.state_change_no(), 0u);
        BOOST_CHECK(copy.find_gen_variable("YMD_YYYY").empty());
        copy.update_repeat_genvar();
        BOOST_CHECK_EQUAL(copy.find_gen_variable("YMD_YYYY").value(), "2026");
    }
    {
        // a repeat day accepts any step
        BOOST_CHECK_EQUAL(Repeat(RepeatDay(0)).step(), 0);
        BOOST_CHECK_EQUAL(Repeat(RepeatDay(-1)).step(), -1);
    }
}

BOOST_AUTO_TEST_SUITE_END() // construction

BOOST_AUTO_TEST_SUITE_END() // T_RepeatEdgeCases

BOOST_AUTO_TEST_SUITE_END() // U_Attributes

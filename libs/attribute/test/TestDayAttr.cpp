/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Str.hpp"
#include "scaffold/ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_DayAttr)

BOOST_AUTO_TEST_CASE(test_day_attr) {
    ECF_NAME_THIS_TEST();

    // See TimeAttr.hpp for rules concerning isFree() and checkForReque()
    // test time attr isFree(), and checkForRequeue
    Calendar calendar;
    calendar.init(ptime(date(2013, 7, 9), minutes(0)), Calendar::REAL); // tuesday

    // Represent a day within a week (range 0==Sun to 6==Sat)
    BOOST_CHECK_MESSAGE(calendar.day_of_week() == 2, " Expected tuesday(2) but found " << calendar.day_of_week());

    DayAttr day(DayAttr::WEDNESDAY);
    day.reset(calendar);

    // int day_changed = 0; // after midnight make sure we keep day_changed
    // day_changed = 0;  tuesday
    // day_changed = 1;  wednesday
    // day_changed = 3;  thursday
    for (int m = 1; m < 96; m++) {
        calendar.update(time_duration(hours(1)));
        // if (calendar.dayChanged())
        //     day_changed++;

        day.calendarChanged(calendar);

        if (day.date() < calendar.date()) {
            BOOST_CHECK_MESSAGE(!day.isFree(calendar),
                                day.toString() << " is free should fail at day " << calendar.day_of_week());
            BOOST_CHECK_MESSAGE(!day.checkForRequeue(calendar),
                                day.toString() << " checkForRequeue should return false for a single day "
                                               << calendar.day_of_week());
        }
        else if (day.date() == calendar.date()) {
            BOOST_CHECK_MESSAGE(day.isFree(calendar),
                                day.toString() << " is free should pass at day " << calendar.day_of_week());
            BOOST_CHECK_MESSAGE(!day.checkForRequeue(calendar),
                                day.toString() << " checkForRequeue should fail at " << calendar.day_of_week());
        }
        else {
            BOOST_CHECK_MESSAGE(day.date() > calendar.date(), "");
            BOOST_CHECK_MESSAGE(!day.isFree(calendar),
                                day.toString() << " is free should pass at day " << calendar.day_of_week());
            BOOST_CHECK_MESSAGE(day.checkForRequeue(calendar),
                                day.toString() << " checkForRequeue should fail at " << calendar.day_of_week());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_day_attr_constructor) {
    ECF_NAME_THIS_TEST();

    {
        DayAttr day;
        BOOST_CHECK_MESSAGE(day.day() == DayAttr::SUNDAY, "");
        BOOST_CHECK_MESSAGE(day.state_change_no() == 0, "");
        BOOST_CHECK_MESSAGE(day.isSetFree() == false, "");
    }
    {
        DayAttr day(DayAttr::WEDNESDAY);
        BOOST_CHECK_MESSAGE(day.day() == DayAttr::WEDNESDAY, "");
        BOOST_CHECK_MESSAGE(day.state_change_no() == 0, "");
        BOOST_CHECK_MESSAGE(day.isSetFree() == false, "");
    }
    {
        DayAttr day("monday");
        BOOST_CHECK_MESSAGE(day.day() == DayAttr::MONDAY, "");
        BOOST_CHECK_MESSAGE(day.state_change_no() == 0, "");
        BOOST_CHECK_MESSAGE(day.isSetFree() == false, "");
    }
}

static DayAttr print_and_parse_attr(DayAttr& day) {

    PrintStyle style(PrintStyle::MIGRATE);
    std::string output;
    day.print(output);
    output.erase(output.begin() + output.size() - 1); // remove trailing newline

    std::vector<std::string> tokens;
    Str::split_orig(output, tokens);

    return DayAttr::create(tokens, true /*read state*/);
}

BOOST_AUTO_TEST_CASE(test_day_parsing) {
    ECF_NAME_THIS_TEST();

    {
        DayAttr day(DayAttr::WEDNESDAY);
        day.setFree();
        DayAttr parsed_day = print_and_parse_attr(day);

        BOOST_CHECK_MESSAGE(day == parsed_day,
                            "Parse failed expected " << day.dump() << " but found " << parsed_day.dump());
    }
    {
        DayAttr day(DayAttr::WEDNESDAY);
        day.setFree();
        DayAttr parsed_day = print_and_parse_attr(day);

        BOOST_CHECK_MESSAGE(day == parsed_day,
                            "Parse failed expected " << day.dump() << " but found " << parsed_day.dump());
    }
    {
        DayAttr day(DayAttr::WEDNESDAY);
        DayAttr parsed_day = print_and_parse_attr(day);

        BOOST_CHECK_MESSAGE(day == parsed_day,
                            "Parse failed expected " << day.dump() << " but found " << parsed_day.dump());
    }
    {
        Calendar calendar;
        calendar.init(ptime(date(2020, 6, 26), minutes(0)), Calendar::REAL); // friday

        DayAttr day(calendar.date());

        DayAttr parsed_day = print_and_parse_attr(day);
        BOOST_CHECK_MESSAGE(day == parsed_day,
                            "Parse failed expected " << day.dump() << " but found " << parsed_day.dump());
    }
    {
        Calendar c;
        c.init(ptime(date(2020, 6, 26), minutes(0)), Calendar::REAL); // friday

        boost::gregorian::date_duration one_day(1);
        boost::gregorian::date next_day = c.date(); // todays date

        for (int i = 0; i < 7; i++) {
            next_day += one_day;

            DayAttr day(next_day);
            DayAttr parsed_day = print_and_parse_attr(day);
            BOOST_CHECK_MESSAGE(day == parsed_day,
                                "Parse failed expected " << day.dump() << " but found " << parsed_day.dump());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

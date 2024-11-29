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

#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/core/Cal.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Calendar)

BOOST_AUTO_TEST_CASE(test_calendar_default_ptime) {
    ECF_NAME_THIS_TEST();

    ptime time;
    BOOST_CHECK_MESSAGE(time.is_special(), "Default ptime is special");
    BOOST_CHECK_MESSAGE(time.is_not_a_date_time(), "Default ptime is_not_a_date_time");
    BOOST_CHECK_MESSAGE(!time.is_infinity(), "Expected Default ptime not is_infinity");
    BOOST_CHECK_MESSAGE(!time.is_pos_infinity(), "Expected default ptime not is_pos_infinity");
    BOOST_CHECK_MESSAGE(!time.is_neg_infinity(), "Expected default ptime not is_neg_infinity");
}

BOOST_AUTO_TEST_CASE(test_calendar_state_parsing) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "Default calendar type should be real");

    // init the calendar to 2009, Feb, 10th, then write out the state
    boost::gregorian::date theDate(2009, 2, 10);
    ptime time(theDate, hours(23) + minutes(59));
    calendar.init(time, Calendar::REAL);
    std::string calendar_state;
    calendar.write_state(calendar_state);

    // read the state, into a different calendar & compare
    std::vector<std::string> lineTokens;
    Str::split(calendar_state, lineTokens);
    Calendar calendar2;
    calendar2.read_state(calendar_state, lineTokens);
    BOOST_CHECK_MESSAGE(calendar == calendar2,
                        "Calendar should be the same\n"
                            << calendar.toString() << "\n"
                            << calendar2.toString());

    // Update calendar.
    calendar.update(minutes(2));
    BOOST_CHECK_MESSAGE(!(calendar == calendar2), "Calendar should be different");

    // re-compare after reloading state
    lineTokens.clear();
    calendar_state.clear();
    calendar.write_state(calendar_state);

    Str::split(calendar_state, lineTokens);
    calendar2.read_state(calendar_state, lineTokens);

    BOOST_CHECK_MESSAGE(calendar == calendar2, "Calendar should be the same");
}

BOOST_AUTO_TEST_CASE(test_calendar_1) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "Default calendar type should be real");

    // init the calendar to 2009, Feb, 10th,  15 minutes past midnight
    boost::gregorian::date theDate(2009, 2, 10);
    ptime time(theDate, hours(23) + minutes(59));
    calendar.init(time, Calendar::HYBRID);
    BOOST_CHECK_MESSAGE(calendar.hybrid(), "init failed to reset calendar type");

    calendar.update(minutes(2));
}

BOOST_AUTO_TEST_CASE(test_calendar) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "Default calendar type should be real");

    // init the calendar to 2009, Feb, 10th,  15 minutes past midnight
    boost::gregorian::date theDate(2009, 2, 10);
    ptime time(theDate, minutes(15));
    calendar.init(time, Calendar::REAL);
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "Calendar should now be REAL");

    std::string expectedTime = "2009-Feb-10 00:15:00";
    std::string actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);

    time_duration td = hours(1) + minutes(10);
    calendar.update(td);
    expectedTime = "2009-Feb-10 01:25:00";
    actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);

    // Increment by 24 hours
    calendar.update(hours(24));
    expectedTime = "2009-Feb-11 01:25:00";
    actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);
}

BOOST_AUTO_TEST_CASE(test_calendar_time_series_relative) {
    ECF_NAME_THIS_TEST();

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::HYBRID);

    std::string expectedTime = "2010-Feb-10 00:00:00";
    std::string actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);

    // Create a test when we can match a time series. Need to sync hour with suite time
    // at hour 1, suite time should also be 01:00, for test to work
    //
    // Create the time series: start  10:00
    //                         finish 20:00
    //                         incr    1:00
    TimeSeries timeSeries(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(1, 0), true /*relative*/);

    for (int hour = 1; hour < 24; hour++) {
        // Update calendar every hour, then see we can match time series, *RELATIVE* to suite start
        calendar.update(time_duration(hours(1)));
        timeSeries.calendarChanged(calendar);

        if (hour >= timeSeries.start().hour() && hour <= timeSeries.finish().hour()) {
            BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                "Calendar should match relative time series at hour " << hour);
        }
        else {
            BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                "Calendar should NOT match relative time series at hour " << hour);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_calendar_time_series_relative_complex) {
    ECF_NAME_THIS_TEST();

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::HYBRID);

    // Create a test when we can match a time series
    // Create the time series: start  10:00
    //                         finish 20:00
    //                         incr   00:15
    TimeSeries timeSeries(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(0, 15), true /*relative*/);

    for (int hour = 0; hour < 24; hour++) {
        for (int minute = 0; minute < 60; minute++) {

            // Update calendar every hour, then see we can match time series, *RELATIVE* to suite start
            calendar.update(minutes(1));
            timeSeries.calendarChanged(calendar);

            tm suiteTm = to_tm(calendar.suiteTime());

            bool matches = timeSeries.isFree(calendar);

            bool intersects =
                (suiteTm.tm_hour >= timeSeries.start().hour() && suiteTm.tm_hour <= timeSeries.finish().hour() &&
                 (suiteTm.tm_min == 0 || suiteTm.tm_min % timeSeries.incr().minute() == 0));
            // Ovoid overshooting past end of series
            bool boundaryOk = true;
            if (suiteTm.tm_hour == timeSeries.finish().hour()) {
                boundaryOk = (suiteTm.tm_min <= timeSeries.finish().minute());
            }

            if (intersects && boundaryOk) {
                BOOST_CHECK_MESSAGE(matches,
                                    "Calendar should match relative time series at "
                                        << suiteTm.tm_hour << Str::COLON() << suiteTm.tm_min
                                        << " suite time = " << to_simple_string(calendar.suiteTime()));
                if (!matches) {
                    ECF_TEST_ERR(<< "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
                                 << " timeSeries.start().hour() " << timeSeries.start().hour()
                                 << " timeSeries.start().minute() " << timeSeries.start().minute()
                                 << " timeSeries.finish().hour() " << timeSeries.finish().hour()
                                 << " timeSeries.finish().minute() " << timeSeries.finish().minute()
                                 << " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15);
                }
            }
            else {
                BOOST_CHECK_MESSAGE(!matches,
                                    "Calendar should NOT match relative time series at "
                                        << suiteTm.tm_hour << Str::COLON() << suiteTm.tm_min
                                        << " suite time = " << to_simple_string(calendar.suiteTime()));

                if (matches) {
                    ECF_TEST_ERR(<< "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
                                 << " timeSeries.start().hour() " << timeSeries.start().hour()
                                 << " timeSeries.start().minute() " << timeSeries.start().minute()
                                 << " timeSeries.finish().hour() " << timeSeries.finish().hour()
                                 << " timeSeries.finish().minute() " << timeSeries.finish().minute()
                                 << " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_calendar_time_series_real) {
    ECF_NAME_THIS_TEST();

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);

    std::string expectedTime = "2010-Feb-10 00:00:00";
    std::string actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);

    // Create a test when we can match a time series
    // Create the time series: start  10:00
    //                         finish 20:00
    //                         incr    1:00
    TimeSeries timeSeries(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(1, 0));

    for (int hour = 1; hour < 24; hour++) {
        // Update calendar every hour, then see we can match time series, in REAL
        // Update will set the local time from the computers system clock, however
        // for testing this will need to be overriden below.
        calendar.update(time_duration(hours(1)));

        if (hour >= timeSeries.start().hour() && hour <= timeSeries.finish().hour()) {
            BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar), "Calendar should match time series at hour " << hour);
        }
        else {
            BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Calendar should NOT match time series at hour " << hour);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_calendar_time_series_real_complex) {
    ECF_NAME_THIS_TEST();

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);

    // Create a test when we can match a time series
    // Create the time series: start  10:00
    //                         finish 20:00
    //                         incr   00:15
    TimeSeries timeSeries(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(0, 15));

    for (int hour = 0; hour < 24; hour++) {
        for (int minute = 0; minute < 60; minute++) {

            // Update calendar every minute, then see we can match time series, *RELATIVE* to suite start
            calendar.update(minutes(1));

            tm suiteTm = to_tm(calendar.suiteTime());

            bool matches = timeSeries.isFree(calendar);

            bool intersects =
                (suiteTm.tm_hour >= timeSeries.start().hour() && suiteTm.tm_hour <= timeSeries.finish().hour() &&
                 (suiteTm.tm_min == 0 || suiteTm.tm_min % timeSeries.incr().minute() == 0));
            // Ovoid overshooting past end of series
            bool boundaryOk = true;
            if (suiteTm.tm_hour == timeSeries.finish().hour()) {
                boundaryOk = (suiteTm.tm_min <= timeSeries.finish().minute());
            }

            if (intersects && boundaryOk) {
                BOOST_CHECK_MESSAGE(matches,
                                    "Calendar should match relative time series at "
                                        << suiteTm.tm_hour << ":" << suiteTm.tm_min
                                        << " suite time = " << to_simple_string(calendar.suiteTime()));
                if (!matches) {
                    ECF_TEST_ERR(<< "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
                                 << " timeSeries.start().hour() " << timeSeries.start().hour()
                                 << " timeSeries.start().minute() " << timeSeries.start().minute()
                                 << " timeSeries.finish().hour() " << timeSeries.finish().hour()
                                 << " timeSeries.finish().minute() " << timeSeries.finish().minute()
                                 << " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15);
                }
            }
            else {
                BOOST_CHECK_MESSAGE(!matches,
                                    "Calendar should NOT match relative time series at "
                                        << suiteTm.tm_hour << ":" << suiteTm.tm_min
                                        << " suite time = " << to_simple_string(calendar.suiteTime()));

                if (matches) {
                    ECF_TEST_ERR(<< "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
                                 << " timeSeries.start().hour() " << timeSeries.start().hour()
                                 << " timeSeries.start().minute() " << timeSeries.start().minute()
                                 << " timeSeries.finish().hour() " << timeSeries.finish().hour()
                                 << " timeSeries.finish().minute() " << timeSeries.finish().minute()
                                 << " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_calendar_hybrid) {
    ECF_NAME_THIS_TEST();

    // The hybrid calendar should not change the suite date.
    // Test by updateing calendar by more than 24 hours

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::HYBRID);
    BOOST_CHECK_MESSAGE(calendar.hybrid(), "calendar type should be hybrid");

    std::string expectedTime = "2010-Feb-10 00:00:00";
    std::string actualTime   = to_simple_string(calendar.suiteTime());
    BOOST_CHECK_MESSAGE(actualTime == expectedTime, "Expected '" << expectedTime << "' but found " << actualTime);

    std::string expectedDate = "2010-Feb-10";

    // Check cache is correct
    int expected_day_of_week  = calendar.day_of_week();
    int expected_day_of_year  = calendar.day_of_year();
    int expected_day_of_month = calendar.day_of_month();
    int expected_month        = calendar.month();
    int expected_year         = calendar.year();

    for (int hour = 1; hour < 73; hour++) {
        // Update calendar every hour, for 73 hours
        // the date should be the same, i.e 2009, Feb, 10th

        ptime timeBeforeUpdate = calendar.suiteTime();

        calendar.update(time_duration(hours(1)));

        ptime timeAfterUpdate = calendar.suiteTime();

        if (hour != 24 && hour != 48 && hour != 72) {
            time_period diff(timeBeforeUpdate, timeAfterUpdate);
            time_duration gap = diff.length();
            BOOST_CHECK_MESSAGE(gap.hours() == 1,
                                "Expected one hour difference but found " << gap.hours() << " at hour " << hour);
        }

        std::string actualDate = to_simple_string(calendar.suiteTime().date());
        BOOST_CHECK_MESSAGE(actualDate == expectedDate,
                            "Expected '" << expectedDate << "' but found " << actualDate << " at hour " << hour);

        // check cache ECFLOW-458
        int actual_day_of_week  = calendar.day_of_week();
        int actual_day_of_year  = calendar.day_of_year();
        int actual_day_of_month = calendar.day_of_month();
        int actual_month        = calendar.month();
        int actual_year         = calendar.year();
        BOOST_CHECK_MESSAGE(actual_day_of_week == expected_day_of_week,
                            "Expected day of week '" << expected_day_of_week << "' but found " << actual_day_of_week
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_year == expected_day_of_year,
                            "Expected day of year '" << expected_day_of_year << "' but found " << actual_day_of_year
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_month == expected_day_of_month,
                            "Expected day of month '" << expected_day_of_month << "' but found " << actual_day_of_month
                                                      << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_month == expected_month,
                            "Expected month '" << expected_month << "' but found " << actual_month << " at hour "
                                               << hour);
        BOOST_CHECK_MESSAGE(actual_year == expected_year,
                            "Expected year '" << expected_year << "' but found " << actual_year << " at hour " << hour);
    }
}

BOOST_AUTO_TEST_CASE(test_day_changed_for_real) {
    ECF_NAME_THIS_TEST();

    // init the calendar to 2009, Feb, 10th,  0 minutes past midnight
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "calendar type should be real");

    // Check cache is correct
    int expected_day_of_week  = calendar.day_of_week();
    int expected_day_of_year  = calendar.day_of_year();
    int expected_day_of_month = calendar.day_of_month();

    for (int hour = 1; hour < 73; hour++) {
        // Update calendar every hour, for 72 hours
        calendar.update(time_duration(hours(1)));

        if (hour == 24 || hour == 48 || hour == 72) {
            BOOST_CHECK_MESSAGE(calendar.dayChanged(),
                                "Expected day change at hour " << hour << " calendar " << calendar.toString());
            expected_day_of_week++;
            expected_day_of_year++;
            expected_day_of_month++;
        }
        else {
            BOOST_CHECK_MESSAGE(!calendar.dayChanged(),
                                "Un-Expected day change at hour " << hour << " calendar " << calendar.toString());
        }

        // check cache ECFLOW-458
        int actual_day_of_week  = calendar.day_of_week();
        int actual_day_of_year  = calendar.day_of_year();
        int actual_day_of_month = calendar.day_of_month();
        BOOST_CHECK_MESSAGE(actual_day_of_week == expected_day_of_week,
                            "Expected day of week '" << expected_day_of_week << "' but found " << actual_day_of_week
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_year == expected_day_of_year,
                            "Expected day of year '" << expected_day_of_year << "' but found " << actual_day_of_year
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_month == expected_day_of_month,
                            "Expected day of month '" << expected_day_of_month << "' but found " << actual_day_of_month
                                                      << " at hour " << hour);
    }
}

BOOST_AUTO_TEST_CASE(test_day_changed_for_hybrid) {
    ECF_NAME_THIS_TEST();

    // init the calendar
    Calendar calendar; // default clock is real
    calendar.init(ptime(date(2015, 10, 31), minutes(0)), Calendar::HYBRID);
    BOOST_CHECK_MESSAGE(calendar.hybrid(), "calendar type should be hybrid");

    // HYBRID calendars allow for day change but not date.
    std::string expected_date = to_simple_string(calendar.date());

    // Check cache is correct
    int expected_day_of_week  = calendar.day_of_week();
    int expected_day_of_year  = calendar.day_of_year();
    int expected_day_of_month = calendar.day_of_month();

    for (int hour = 1; hour < 73; hour++) {
        // Update calendar every hour, for 72 hours
        calendar.update(time_duration(hours(1)));

        BOOST_CHECK_MESSAGE(expected_date == to_simple_string(calendar.date()),
                            "Unexpected date change for hybrid calendar at hour " << hour);

        // Day should change even for hybrid calendar,
        if (hour == 24 || hour == 48 || hour == 72) {
            BOOST_CHECK_MESSAGE(calendar.dayChanged(),
                                "Expected day change at hour " << hour << " calendar " << calendar.toString());
        }
        else {
            BOOST_CHECK_MESSAGE(!calendar.dayChanged(),
                                "Un-Expected day change at hour " << hour << " calendar " << calendar.toString());
        }

        // check cache ECFLOW-458
        int actual_day_of_week  = calendar.day_of_week();
        int actual_day_of_year  = calendar.day_of_year();
        int actual_day_of_month = calendar.day_of_month();
        BOOST_CHECK_MESSAGE(actual_day_of_week == expected_day_of_week,
                            "Expected day of week '" << expected_day_of_week << "' but found " << actual_day_of_week
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_year == expected_day_of_year,
                            "Expected day of year '" << expected_day_of_year << "' but found " << actual_day_of_year
                                                     << " at hour " << hour);
        BOOST_CHECK_MESSAGE(actual_day_of_month == expected_day_of_month,
                            "Expected day of month '" << expected_day_of_month << "' but found " << actual_day_of_month
                                                      << " at hour " << hour);
    }
}

BOOST_AUTO_TEST_CASE(test_calendar_julian) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    calendar.init(ptime(date(2017, 1, 1), minutes(0)), Calendar::REAL);
    BOOST_CHECK_MESSAGE(!calendar.hybrid(), "calendar type should be real");

    int days = 0;
    while (calendar.year() != 2018) {

        boost::gregorian::date cal_date = calendar.date();
        long boost_julian               = cal_date.julian_day();

        std::string iso_string = to_iso_string(cal_date);
        auto date_as_long      = ecf::convert_to<long>(iso_string);
        long ecmwf_julian      = Cal::date_to_julian(date_as_long);

        BOOST_CHECK_MESSAGE(boost_julian == ecmwf_julian,
                            "boost julian " << boost_julian << " != ecmwf julian " << ecmwf_julian << " for "
                                            << iso_string);

        // Update calendar every day for a year
        calendar.update(time_duration(hours(24)));
        days++;
    }
    BOOST_CHECK_MESSAGE(days == 365, "expected 365 days but found " << days);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

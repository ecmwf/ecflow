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

#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Cron)

BOOST_AUTO_TEST_CASE(test_cron_parsing) {
    ECF_NAME_THIS_TEST();

    TimeSlot start(10, 10);
    TimeSlot finish(23, 10);
    TimeSlot incr(0, 1);

    {
        CronAttr cron;
        CronAttr parsedCron;
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
    }
    {
        CronAttr cron;
        cron.add_time_series(10, 10, false);
        CronAttr cron1(10, 10, false);
        CronAttr parsedCron = CronAttr::create("cron 10:10");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }
    {
        CronAttr cron;
        cron.add_time_series(10, 10, true);
        CronAttr cron1(10, 10, true);
        CronAttr parsedCron = CronAttr::create("cron +10:10");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }
    {
        CronAttr cron;
        cron.addTimeSeries(start, finish, incr);
        CronAttr cron1(start, finish, incr);
        CronAttr parsedCron = CronAttr::create("cron 10:10 23:10 00:01");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }

    {
        std::vector<int> week_days{0, 1, 2, 3, 4, 5, 6};
        CronAttr cron;
        cron.addWeekDays(week_days);
        cron.add_time_series(10, 10, true);

        CronAttr cron1(10, 10, true);
        cron1.addWeekDays(week_days);

        std::string cron_str = "cron -w 0,1,2,3,4,5,6 +10:10";
        CronAttr parsedCron  = CronAttr::create(cron_str);
        BOOST_CHECK_MESSAGE(parsedCron.toString() == cron_str,
                            "Expected " << cron_str << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }
    {
        std::vector<int> week_days{0, 1, 2, 3, 4, 5, 6};
        CronAttr cron;
        cron.add_last_week_days_of_month(week_days);
        cron.add_time_series(10, 10, true);

        std::string cron_str = "cron -w 0L,1L,2L,3L,4L,5L,6L +10:10";
        CronAttr parsedCron  = CronAttr::create(cron_str);
        BOOST_CHECK_MESSAGE(parsedCron.toString() == cron_str,
                            "Expected " << cron_str << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
    }
    {
        std::vector<int> week_days{0, 1, 2};
        std::vector<int> last_week_days_of_month{3, 4, 5, 6};
        CronAttr cron;
        cron.addWeekDays(week_days);
        cron.add_last_week_days_of_month(last_week_days_of_month);
        cron.add_time_series(10, 10, true);

        std::string cron_str = "cron -w 0,1,2,3L,4L,5L,6L +10:10";
        CronAttr parsedCron  = CronAttr::create(cron_str);
        BOOST_CHECK_MESSAGE(parsedCron.toString() == cron_str,
                            "Expected " << cron_str << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
    }
    {
        std::vector<int> week_days{0, 1, 2};
        std::vector<int> last_week_days_of_month{3, 4};
        CronAttr cron;
        cron.addWeekDays(week_days);
        cron.add_last_week_days_of_month(last_week_days_of_month);
        cron.add_time_series(10, 10, true);

        try {
            CronAttr parsedCron = CronAttr::create("cron -w 0,1,2,0L,1L,2L +10:10");
            BOOST_CHECK_MESSAGE(false,
                                "Expected exception for " << parsedCron.toString() << " cron " << cron.toString());
        }
        catch (...) {
        }
    }

    {
        std::vector<int> days_of_month{1, 3, 4, 5, 6, 24, 25};
        CronAttr cron;
        cron.addDaysOfMonth(days_of_month);
        cron.add_last_day_of_month();
        cron.add_time_series(10, 10, true);

        CronAttr cron1(10, 10, true);
        cron1.addDaysOfMonth(days_of_month);
        cron1.add_last_day_of_month();

        CronAttr parsedCron  = CronAttr::create("cron -d L,1,3,4,5,6,24,25 +10:10");
        CronAttr parsedCron2 = CronAttr::create("cron -d 1,3,4,5,6,24,25,L +10:10");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron2.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron2.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }

    std::vector<int> months{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    {
        CronAttr cron;
        cron.addMonths(months);
        cron.add_time_series(10, 10, true);

        CronAttr cron1(10, 10, true);
        cron1.addMonths(months);

        CronAttr parsedCron = CronAttr::create("cron -m 1,2,3,4,5,6,7,8,9,10,11,12 +10:10");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }

    {
        std::vector<int> week_days{0, 1, 2};
        std::vector<int> last_week_days_of_month{3, 4, 5, 6};
        std::vector<int> days_of_month{1, 3, 4, 5, 6, 24, 25};

        CronAttr cron;
        cron.addWeekDays(week_days);
        cron.add_last_week_days_of_month(last_week_days_of_month);
        cron.addDaysOfMonth(days_of_month);
        cron.addMonths(months);
        cron.add_last_day_of_month();
        cron.addTimeSeries(start, finish, incr);

        CronAttr cron1(start, finish, incr);
        cron1.addWeekDays(week_days);
        cron1.add_last_week_days_of_month(last_week_days_of_month);
        cron1.addDaysOfMonth(days_of_month);
        cron1.addMonths(months);
        cron1.add_last_day_of_month();

        CronAttr parsedCron = CronAttr::create(
            "cron -w 0,1,2,3L,4L,5L,6L -d 1,3,4,5,6,24,L,25 -m 1,2,3,4,5,6,7,8,9,10,11,12 10:10 23:10 00:01");
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron),
                            "Expected " << cron.toString() << " but found " << parsedCron.toString());
        BOOST_CHECK_MESSAGE(parsedCron.structureEquals(cron1),
                            "Expected " << cron1.toString() << " but found " << parsedCron.toString());
    }

    {
        std::vector<std::string> cron_vec{
            "cron -w 0,1,2,3L,4L,5L,6L -d 1,3,4,5,6,24,L,25 -m 1,2,3,4,5,6,7,8,9,10,11,12 10:10 23:10 00:01",
            "cron -w 0L,1L,2L,3L,4L,5L,6L -d 1,L 23:00"};
        for (const auto& cron : cron_vec) {
            try {
                CronAttr parsedCron = CronAttr::create(cron);
            }
            catch (...) {
                BOOST_CHECK_MESSAGE(false, "Could not parse " << cron);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_cron_state_parsing) {
    ECF_NAME_THIS_TEST();

    size_t index = 1; // to get over the cron
    {
        std::string line = "cron 04:30 # isValid:false";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSeries series(4, 30);
        series.set_isValid(false); // to match isValid:false
        expected.addTimeSeries(series);
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }

    {
        std::string line = "cron 04:30 # free isValid:false";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSeries series(4, 30);
        series.set_isValid(false); // to match isValid:false
        expected.addTimeSeries(series);
        expected.setFree();
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }

    {
        std::string line = "cron 00:01 23:59 01:00 # nextTimeSlot/12:01";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSlot start(0, 1);
        TimeSlot finish(23, 59);
        TimeSlot incr(1, 0);
        TimeSeries series(start, finish, incr);
        series.set_next_time_slot(TimeSlot(12, 1));
        expected.addTimeSeries(series);
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }

    {
        std::string line = "cron 00:01 23:59 01:00 # free nextTimeSlot/12:01";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSlot start(0, 1);
        TimeSlot finish(23, 59);
        TimeSlot incr(1, 0);
        TimeSeries series(start, finish, incr);
        series.set_next_time_slot(TimeSlot(12, 1));
        expected.addTimeSeries(series);
        expected.setFree();
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }
    {
        std::string line = "cron 00:00 18:00 06:00 # isValid:false nextTimeSlot/24:00";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSlot start(0, 0);
        TimeSlot finish(18, 0);
        TimeSlot incr(6, 0);
        TimeSeries series(start, finish, incr);
        series.set_next_time_slot(TimeSlot(24, 0));
        series.set_isValid(false); // to match isValid:false
        expected.addTimeSeries(series);
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }

    {
        // ECFLOW-1693
        // Could not parse 'cron +00:00 23:59 00:01 # isValid:false nextTimeSlot/523:40' around line number 654
        std::string line = "cron +00:00 23:59 00:01 # isValid:false nextTimeSlot/523:40";
        std::vector<std::string> lineTokens;
        Str::split(line, lineTokens);
        bool parse_state = true;
        CronAttr parsed_cronAttr;
        CronAttr::parse(parsed_cronAttr, lineTokens, index, parse_state);

        CronAttr expected;
        TimeSlot start(0, 0);
        TimeSlot finish(23, 59);
        TimeSlot incr(0, 1);
        TimeSeries series(start, finish, incr, true);
        series.set_next_time_slot(TimeSlot(523, 40));
        series.set_isValid(false); // to match isValid:false
        expected.addTimeSeries(series);
        BOOST_CHECK_MESSAGE(parsed_cronAttr == expected,
                            "Expected " << expected.dump() << " : " << expected.time().dump() << " but found "
                                        << parsed_cronAttr.dump() << " : " << parsed_cronAttr.time().dump());
    }
}

BOOST_AUTO_TEST_CASE(test_cron_once_free_stays_free) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);

    TimeSeries timeSeriesX(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries2X(TimeSlot(11, 0), TimeSlot(15, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries3X(TimeSlot(15, 0), false /* relative */);
    TimeSeries timeSeries4X(TimeSlot(0, 0), false /* relative */);

    CronAttr timeSeries;
    timeSeries.addTimeSeries(timeSeriesX);
    CronAttr timeSeries2;
    timeSeries2.addTimeSeries(timeSeries2X);
    CronAttr timeSeries3;
    timeSeries3.addTimeSeries(timeSeries3X);
    CronAttr timeSeries4;
    timeSeries4.addTimeSeries(timeSeries4X);

    std::vector<boost::posix_time::time_duration> timeSeries_free_slots;
    std::vector<boost::posix_time::time_duration> timeSeries2_free_slots;
    timeSeries.time_series().free_slots(timeSeries_free_slots);
    timeSeries2.time_series().free_slots(timeSeries2_free_slots);
    BOOST_CHECK_MESSAGE(timeSeries_free_slots.size() == 11,
                        "Expected 11 free slots for " << timeSeries.toString() << " but found "
                                                      << timeSeries_free_slots.size());
    BOOST_CHECK_MESSAGE(timeSeries2_free_slots.size() == 5,
                        "Expected 5 free slots for " << timeSeries2.toString() << " but found "
                                                     << timeSeries_free_slots.size());

    bool day_changed = false; // after midnight make sure we keep day_changed
    for (int m = 1; m < 96; m++) {
        calendar.update(time_duration(minutes(30)));
        if (!day_changed) {
            day_changed = calendar.dayChanged();
        }
        boost::posix_time::time_duration time = calendar.suiteTime().time_of_day();

        timeSeries.calendarChanged(calendar);
        timeSeries2.calendarChanged(calendar);
        timeSeries3.calendarChanged(calendar);
        timeSeries4.calendarChanged(calendar);

        // cron should always reque regardless of time series
        BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar),
                            timeSeries.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar),
                            timeSeries2.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries3.checkForRequeue(calendar),
                            timeSeries3.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries4.checkForRequeue(calendar),
                            timeSeries4.toString() << " checkForRequeue should be true at time " << time);

        // **********************************************************************************
        // When a cron (regardless of whether its single slot or time series) is free, it stays free,
        // until explicitly re-queued,
        // ***********************************************************************************

        if (time < timeSeries.time_series().start().duration()) {
            if (!day_changed)
                BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should NOT be free at time " << time);
            else
                BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should be free at time " << time);
        }
        else if (time >= timeSeries.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                timeSeries.toString() << " should be free at time " << time);
        }

        if (time < timeSeries2.time_series().start().duration()) {
            if (!day_changed)
                BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),
                                    timeSeries2.toString() << " should NOT be free at time " << time);
            else
                BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should be free at time " << time);
        }
        else if (time >= timeSeries2.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),
                                timeSeries2.toString() << " should be free at time " << time);
        }

        if (!day_changed) {
            if (time == timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " should be free at time " << time);
            }
            else if (time > timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " isFree, once free should stay free at time " << time);
            }
        }
        else {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                timeSeries3.toString() << " should be free at time after day change " << time);
        }

        // single slot at midnight, Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent
        // repeat/cron)
        if (!day_changed) {
            if (time == timeSeries4.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),
                                    timeSeries4.toString() << " should be free at time " << time);
            }
            else {
                BOOST_CHECK_MESSAGE(!timeSeries4.isFree(calendar),
                                    timeSeries4.toString()
                                        << " day_changed(" << day_changed << ")  isFree should fail at time " << time);
            }
        }
        else {
            BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),
                                timeSeries4.toString()
                                    << " day_changed(" << day_changed << ")  isFree should pass at time " << time);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_cron_time_series) {
    ECF_NAME_THIS_TEST();

    // See TimeAttr.hpp for rules concerning isFree() and checkForReque()
    // test time attr isFree(), and checkForRequeue
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);

    TimeSeries timeSeriesX(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries2X(TimeSlot(11, 0), TimeSlot(15, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries3X(TimeSlot(15, 0), false /* relative */);
    TimeSeries timeSeries4X(TimeSlot(0, 0), false /* relative */);

    CronAttr timeSeries;
    timeSeries.addTimeSeries(timeSeriesX);
    CronAttr timeSeries2;
    timeSeries2.addTimeSeries(timeSeries2X);
    CronAttr timeSeries3;
    timeSeries3.addTimeSeries(timeSeries3X);
    CronAttr timeSeries4;
    timeSeries4.addTimeSeries(timeSeries4X);

    std::vector<boost::posix_time::time_duration> timeSeries_free_slots;
    std::vector<boost::posix_time::time_duration> timeSeries2_free_slots;
    timeSeries.time_series().free_slots(timeSeries_free_slots);
    timeSeries2.time_series().free_slots(timeSeries2_free_slots);
    BOOST_CHECK_MESSAGE(timeSeries_free_slots.size() == 11,
                        "Expected 11 free slots for " << timeSeries.toString() << " but found "
                                                      << timeSeries_free_slots.size());
    BOOST_CHECK_MESSAGE(timeSeries2_free_slots.size() == 5,
                        "Expected 5 free slots for " << timeSeries2.toString() << " but found "
                                                     << timeSeries_free_slots.size());

    bool day_changed = false; // after midnight make sure we keep day_changed
    for (int m = 1; m < 96; m++) {
        calendar.update(time_duration(minutes(30)));
        if (!day_changed) {
            day_changed = calendar.dayChanged();
        }
        boost::posix_time::time_duration time = calendar.suiteTime().time_of_day();

        timeSeries.calendarChanged(calendar);
        timeSeries2.calendarChanged(calendar);
        timeSeries3.calendarChanged(calendar);
        timeSeries4.calendarChanged(calendar);

        // cron should always reque regardless of time series
        BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar),
                            timeSeries.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar),
                            timeSeries2.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries3.checkForRequeue(calendar),
                            timeSeries3.toString() << " checkForRequeue should be true at time " << time);
        BOOST_CHECK_MESSAGE(timeSeries4.checkForRequeue(calendar),
                            timeSeries4.toString() << " checkForRequeue should be true at time " << time);

        // **********************************************************************************
        // When a cron (regardless of whether its single slot or time series) is free, it stays free
        // However in order to test crons with time series, we will re-queue ate the end of this loop
        // ***********************************************************************************

        if (time < timeSeries.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                timeSeries.toString() << " should NOT be free at time " << time);
        }
        else if (time >= timeSeries.time_series().start().duration() &&
                 time <= timeSeries.time_series().finish().duration()) {

            bool matches_free_slot = false;
            for (const auto& timeSeries_free_slot : timeSeries_free_slots) {
                if (time == timeSeries_free_slot) {
                    matches_free_slot = true;
                    break;
                }
            }
            if (matches_free_slot)
                BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should be free at time " << time);
            else
                BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should be fail at time " << time);
        }
        else {
            BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                timeSeries.toString() << " should be holding at time " << time);
        }

        if (time < timeSeries2.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),
                                timeSeries2.toString() << " should NOT be free at time " << time);
        }
        else if (time >= timeSeries2.time_series().start().duration() &&
                 time <= timeSeries2.time_series().finish().duration()) {

            bool matches_free_slot = false;
            for (const auto& timeSeries2_free_slot : timeSeries2_free_slots) {
                if (time == timeSeries2_free_slot) {
                    matches_free_slot = true;
                    break;
                }
            }
            if (matches_free_slot)
                BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),
                                    timeSeries2.toString() << " should be free at time " << time);
            else
                BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),
                                    timeSeries2.toString() << " should be fail at time " << time);
        }
        else {
            BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),
                                timeSeries2.toString() << " should be holding at time " << time);
        }

        // Single slot, Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent
        // repeat/cron)
        if (!day_changed) {
            if (time == timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " should be free at time " << time);
            }
            else if (time > timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " isFree, once free should stay free at time " << time);
            }
        }
        else {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                timeSeries3.toString() << " should be free at time after day change " << time);
        }

        // single slot at midnight, Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent
        // repeat/cron)
        if (!day_changed) {
            if (time == timeSeries4.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),
                                    timeSeries4.toString() << " should be free at time " << time);
            }
            else {
                BOOST_CHECK_MESSAGE(!timeSeries4.isFree(calendar),
                                    timeSeries4.toString()
                                        << " day_changed(" << day_changed << ")  isFree should fail at time " << time);
            }
        }
        else {
            BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),
                                timeSeries4.toString()
                                    << " day_changed(" << day_changed << ")  isFree should pass at time " << time);
        }

        // Typically when a cron is free, it stays free, until it is re-queued
        // However in order to test isFree for cron with time inetrvals, we need to re-queue
        timeSeries.requeue(calendar);
        timeSeries2.requeue(calendar);

        // Do not requeue cron 00, and cron 15, so that we can check for free
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

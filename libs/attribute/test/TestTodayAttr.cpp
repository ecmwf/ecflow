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
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_TodayAttr)

BOOST_AUTO_TEST_CASE(test_today_string_constrcutor) {
    ECF_NAME_THIS_TEST();

    {
        TodayAttr time("+00:30");
        BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 0 && time.time_series().start().minute() == 30 &&
                                time.time_series().finish().isNULL() && time.time_series().incr().isNULL() &&
                                time.time_series().relative(),
                            "Error in today constructor");
    }
    {
        TodayAttr time("12:30");
        BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 12 && time.time_series().start().minute() == 30 &&
                                time.time_series().finish().isNULL() && time.time_series().incr().isNULL() &&
                                !time.time_series().relative(),
                            "Error in today constructor");
    }
    {
        TodayAttr time("+00:30 11:30 00:01");
        BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 0 && time.time_series().start().minute() == 30 &&
                                time.time_series().finish().hour() == 11 &&
                                time.time_series().finish().minute() == 30 && time.time_series().incr().hour() == 0 &&
                                time.time_series().incr().minute() == 1 && time.time_series().relative(),
                            "Error in today constructor");
    }
    {
        BOOST_REQUIRE_THROW(TodayAttr(""), std::runtime_error);
        BOOST_REQUIRE_THROW(TodayAttr(" "), std::runtime_error);
        BOOST_REQUIRE_THROW(TodayAttr("sdsdsdsd"), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_today_attr) {
    ECF_NAME_THIS_TEST();

    // See TodayAttr.hpp for rules concerning isFree() and checkForReque()
    // test today attr isFree(), and checkForRequeue
    Calendar calendar;
    calendar.init(ptime(date(2010, 2, 10), minutes(0)), Calendar::REAL);

    // Create a test when we can match a time series. Need to sync hour with suite time
    // at hour 1, suite time should also be 01:00, for test to work
    //
    // Create the time series: start  10:00
    //                         finish 20:00
    //                         incr    1:00
    TimeSeries timeSeriesX(TimeSlot(10, 0), TimeSlot(20, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries2X(TimeSlot(11, 0), TimeSlot(15, 0), TimeSlot(1, 0), false /* relative */);
    TimeSeries timeSeries3X(TimeSlot(15, 0), false /* relative */);

    TimeSlot t1_min, t1_max, t2_min, t2_max, t3_min, t3_max;
    timeSeriesX.min_max_time_slots(t1_min, t1_max);
    timeSeries2X.min_max_time_slots(t2_min, t2_max);
    timeSeries3X.min_max_time_slots(t3_min, t3_max);
    BOOST_CHECK_MESSAGE(t1_min == TimeSlot(10, 0) && t1_max == TimeSlot(20, 0), "Not as expected");
    BOOST_CHECK_MESSAGE(t2_min == TimeSlot(11, 0) && t2_max == TimeSlot(15, 0), "Not as expected");
    BOOST_CHECK_MESSAGE(t3_min == TimeSlot(15, 0) && t3_max == TimeSlot(15, 0), "Not as expected");

    TodayAttr timeSeries(timeSeriesX);
    TodayAttr timeSeries2(timeSeries2X);
    TodayAttr timeSeries3(timeSeries3X);

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

    // follow normal process
    timeSeries.reset(calendar);
    timeSeries2.reset(calendar);
    timeSeries3.reset(calendar);

    bool cmd_context = true;
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

        if (!day_changed) {

            if (time < timeSeries.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should NOT be free at time " << time);
                BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar, t1_min, t1_max),
                                    timeSeries.toString() << " should pass at " << time);
                BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar, t1_min, t1_max, cmd_context),
                                    timeSeries.toString() << " should pass at " << time);
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
                // no else branch since once today is free it stays free, unti re-queue
                if (matches_free_slot)
                    BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                        timeSeries.toString() << " should be free at time " << time);

                /// At the last time checkForRequeue should return false; This ensures that value will
                /// not get incremented and so, should leave node in the complete state.
                if (time < timeSeries.time_series().finish().duration()) {
                    BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar, t1_min, t1_max),
                                        timeSeries.toString() << " checkForRequeue should be free at time " << time);
                }
                else {
                    BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar, t1_min, t1_max),
                                        timeSeries.toString() << "checkForRequeue should Not free at time " << time);
                }
            }
            else {
                // After end time, a Today Attr should be free to run.
                BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                    timeSeries.toString() << " should be free at time " << time);
                BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar, t1_min, t1_max),
                                    timeSeries.toString() << " should fail at " << time);
            }
        }
        else {
            // Once a today time series is free, it stays free
            BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),
                                timeSeries.toString() << " should be free at time " << time);
        }

        if (!day_changed) {
            if (time < timeSeries2.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),
                                    timeSeries2.toString() << " should NOT be free at time " << time);
                BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar, t2_min, t2_max),
                                    timeSeries2.toString() << " should pass at " << time);
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

                /// At the last time checkForRequeue should return false; This ensures that value will
                /// not get incremented and so, should leave node in the complete state.
                if (time < timeSeries2.time_series().finish().duration()) {
                    BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar, t2_min, t2_max),
                                        timeSeries2.toString() << " checkForRequeue should be free at time " << time);
                }
                else {
                    BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar, t2_min, t2_max),
                                        timeSeries2.toString() << "checkForRequeue should Not free at time " << time);
                }
            }
            else {
                // After end time, a Today Attr should be free to run.
                BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),
                                    timeSeries2.toString() << " should be holding at time " << time);
                BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar, t2_min, t2_max),
                                    timeSeries2.toString() << " should fail at " << time);
            }
        }
        else {
            // Once a today time series is free, it stays free
            BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),
                                timeSeries2.toString() << " should be holding at time " << time);
        }

        // Single slot
        if (!day_changed) {
            if (time == timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " should be free at time " << time);
            }
            else if (time < timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(!timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " isFree should fail at time " << time);
            }
            else if (time > timeSeries3.time_series().start().duration()) {
                BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                    timeSeries3.toString() << " isFree should fail at time " << time);
            }
        }
        else {
            // Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent repeat/cron)
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),
                                timeSeries3.toString() << " isFree should fail at time " << time);
        }
        BOOST_CHECK_MESSAGE(!timeSeries3.checkForRequeue(calendar, t3_min, t3_max),
                            timeSeries3.toString() << " checkForRequeue should fail at " << time);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

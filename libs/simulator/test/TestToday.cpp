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

#include <boost/test/unit_test.hpp>

#include "TestUtil.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/simulator/Simulator.hpp"

using namespace ecf;

/// Simulate definition files that are created on then fly. This allows us to create
/// tests with todays date/time this speeds up the testr, we can also validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE(S_Simulator)

BOOST_AUTO_TEST_SUITE(T_Today)

BOOST_AUTO_TEST_CASE(test_today) {
    std::cout << "Simulator:: ...test_today\n";

    // suite suite
    //   clock real <fixed date>
    //   family family
    //     task t1
    //       today <start>  # +1 minute
    //       today <start>  # +2 minute
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        // Initialise clock   then create a today attribute + minutes
        // such that the task should only run once, in the next minute
        auto theLocalTime =
            boost::posix_time::ptime(boost::gregorian::date(2010, 6, 21), boost::posix_time::time_duration(1, 2, 0));
        auto time_plus_minute    = theLocalTime + boost::posix_time::minutes(1);
        auto time_plus_10_minute = theLocalTime + boost::posix_time::minutes(10);

        suite_ptr suite = theDefs.add_suite("test_today");
        ClockAttr clockAttr(theLocalTime, false /*false means use real clock*/);
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addToday(ecf::TodayAttr(TimeSlot(time_plus_minute.time_of_day())));
        task->addToday(ecf::TodayAttr(TimeSlot(time_plus_10_minute.time_of_day())));
        task->addVerify(VerifyAttr(NState::COMPLETE, 2)); // expect task to complete 2 time
    }

    Simulator simulator;
    std::string errorMsg;
    bool result = simulator.run(theDefs, findTestDataLocation("test_today.def"), errorMsg);

    BOOST_CHECK_MESSAGE(result, errorMsg);

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_today.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_today_time_series) {
    std::cout << "Simulator:: ...test_today_time_series\n";

    // suite suite
    //   clock real <monday>
    //   family family
    //     task t1
    //       today 00:30 18:59 04:00  # should run 5 times 00:30 4:30 8:30 12:30 16:30
    //   endfamily
    // endsuite

    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_today_time_series");
        ClockAttr clockAttr(true /*false means use hybrid clock*/);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        TimeSeries timeSeries(TimeSlot(00, 30), TimeSlot(18, 59), TimeSlot(4, 0), false /* relative */);

        task->addToday(TodayAttr(timeSeries));
        task->addVerify(VerifyAttr(NState::COMPLETE, 5));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_today_time_series.def"), errorMsg), errorMsg);

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_today_time_series.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_today_time_and_date) {
    std::cout << "Simulator:: ...test_today_time_and_date\n";

    // suite suite
    //   clock real <todays date>
    //   family family
    //     task t1
    //       date  <today date>
    //       time  <start>
    //       today <start>
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        // To speed up simulation: start calendar with hour increment AND time attributes with hours only
        //
        // Task will only run if all time dependencies are satisfied
        auto theLocalTime = boost::posix_time::ptime(boost::gregorian::date(2010, 2, 10), boost::posix_time::hours(15));
        auto todaysDate   = theLocalTime.date();
        auto time_plus_hour = theLocalTime + boost::posix_time::hours(1);

        suite_ptr suite = theDefs.add_suite("test_today_time_and_date");
        ClockAttr clockAttr(theLocalTime, false /*false means use real clock*/);
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addDate(DateAttr(todaysDate));
        task->addTime(ecf::TimeAttr(TimeSlot(time_plus_hour.time_of_day())));
        task->addToday(ecf::TodayAttr(TimeSlot(time_plus_hour.time_of_day())));

        task->addVerify(VerifyAttr(NState::COMPLETE, 1));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_today_time_and_date.def"), errorMsg),
                        errorMsg);

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_today_time_and_date.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

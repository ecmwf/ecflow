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
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/simulator/Simulator.hpp"

using namespace ecf;

/// Simulate definition files that are created on then fly. This us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE(S_Simulator)

BOOST_AUTO_TEST_SUITE(T_Repeat)

BOOST_AUTO_TEST_CASE(test_repeat_integer) {
    std::cout << "Simulator:: ...test_repeat_integer\n";

    // suite suite
    //   repeat integer VAR 0 1 1          # run at 0, 1    2 times
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     repeat integer VAR 0 1 1     # run at 0, 1     2 times
    //     task t<n>
    //     ....
    //   endfamily
    // endsuite

    // Each task/job should be run *4* times, according to the repeats
    // Mimics nested loops
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("suite");
        suite->addRepeat(RepeatInteger("VAR", 0, 1, 1)); // repeat contents 2 times
        suite->addVerify(VerifyAttr(NState::COMPLETE, 2));
        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatInteger("VAR", 0, 1, 1));   // repeat contents 2 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 4)); // verify family repeats 2 times
        int taskSize = 2;
        for (int i = 0; i < taskSize; i++) {
            task_ptr t = fam->add_task("t" + ecf::convert_to<std::string>(i));
            t->addVerify(VerifyAttr(NState::COMPLETE, 4)); // Each task should run 4 times
        }
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_integer.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_integer.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_integer_relative) {
    std::cout << "Simulator:: ...test_repeat_integer_relative\n";

    // suite suite
    //   repeat integer VAR 0 1 1          # run at 0, 1    2 times
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     repeat integer VAR 0 1 1     # run at 0, 1     2 times
    //     task t1
    //       time +0;02
    //   endfamily
    // endsuite

    // Each task/job should be run *4* times relative to Node times, according to the repeats
    // Mimics nested loops
    Defs theDefs;
    {
        ClockAttr clockAttr(true /*false means use hybrid clock*/);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_repeat_integer_relative");
        suite->addClock(clockAttr);
        suite->addRepeat(RepeatInteger("VAR", 0, 1, 1)); // repeat contents 2 times
        suite->addVerify(VerifyAttr(NState::COMPLETE, 2));

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatInteger("VAR", 0, 1, 1)); // repeat contents 2 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 4));

        task_ptr t = fam->add_task("t1");
        t->addTime(ecf::TimeAttr(TimeSlot(0, 2), true /*relative*/));
        t->addVerify(VerifyAttr(NState::COMPLETE, 4)); // Each task should run 4 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_integer_relative.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_integer_relative.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_date) {
    std::cout << "Simulator:: ...test_repeat_date\n";
    // suite suite
    //   clock real <fixed date + time>
    //   family family
    //     repeat date YMD 20091001  20091015 1  # yyyymmdd
    //     task t
    //       time 10:00
    //   endfamily
    // endsuite

    // Each task should be run 15 times, ie every day at 10.00 am from  1st Oct->15 October 15 times
    Defs theDefs;
    {
        ClockAttr clockAttr;
        clockAttr.date(1, 10, 2009);
        suite_ptr suite = theDefs.add_suite("test_repeat_date");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDate("YMD", 20091001, 20091015, 1)); // repeat contents 15 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 15));

        task_ptr task = fam->add_task("t");
        task->addTime(ecf::TimeAttr(TimeSlot(10, 0)));
        task->addVerify(VerifyAttr(NState::COMPLETE, 15)); // task should complete 15 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_date.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_date.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_date_2) {
    std::cout << "Simulator:: ...test_repeat_date_2\n";
    // suite suite
    //   clock real <fixed date + time>
    //   family family
    //     repeat date YMD 20091015 20091001 -1
    //     task t
    //        time 10:00
    //     endfamily
    // endsuite

    // Each task should be run 15 times, ie every day at 10.00 am
    Defs theDefs;
    {
        ClockAttr clockAttr;
        clockAttr.date(1, 10, 2015);
        suite_ptr suite = theDefs.add_suite("test_repeat_date");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDate("YMD", 20091015, 20091001, -1)); // repeat contents 15 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 15));

        task_ptr task = fam->add_task("t");
        task->addTime(ecf::TimeAttr(TimeSlot(10, 0)));
        task->addVerify(VerifyAttr(NState::COMPLETE, 15)); // task should complete 15 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_date.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_date.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_date_for_loop) {
    std::cout << "Simulator:: ...test_repeat_date_for_loop\n";

    // suite suite
    //   clock real <todays date>
    //   repeat date YMD 20091001  20091005 1  # yyyymmdd
    //   family family
    //     repeat date YMD 20091001  20091005 1  # yyyymmdd
    //     task t
    //       time 10:00
    //   endfamily
    // endsuite

    // Each task should be run 5 * 5= 25 times, ie every day from from 1st Oct -> 5 Oct 5*5 times
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_date_for_loop");
        suite->addRepeat(RepeatDate("YMD", 20091001, 20091005, 1)); // repeat contents 5 times
        suite->addVerify(VerifyAttr(NState::COMPLETE, 5));

        // start at specific time other wise time dependent checks will not verify
        ClockAttr clockAttr;
        clockAttr.date(1, 10, 2009);
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDate("YMD", 20091001, 20091005, 1)); // repeat contents 5 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 25));

        task_ptr task = fam->add_task("t");
        task->addTime(ecf::TimeAttr(TimeSlot(10, 0)));
        task->addVerify(VerifyAttr(NState::COMPLETE, 25)); // task should complete 25 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_date_for_loop.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_date_for_loop.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_date_for_loop2) {
    std::cout << "Simulator:: ...test_repeat_date_for_loop2\n";

    // suite suite
    //   clock real <todays date>
    //   repeat date YMD 20091001  20091005 1  # yyyymmdd
    //   family family
    //     repeat date YMD 20091001  20091005 1  # yyyymmdd
    //     task t
    //       time 10:00
    //       time 11:00
    //   endfamily
    // endsuite

    // Each task should be run 5 * 5 * 2 = 50 times, ie every day from from 1st Oct -> 5 Oct 5*5 times * 2 time slots
    Defs theDefs;
    {
        // start at specific time other wise time dependent checks will not verify
        suite_ptr suite = theDefs.add_suite("test_repeat_date_for_loop2");
        suite->addRepeat(RepeatDate("YMD", 20091001, 20091005, 1)); // repeat contents 5 times
        suite->addVerify(VerifyAttr(NState::COMPLETE, 5));

        ClockAttr clockAttr;
        clockAttr.date(1, 10, 2009);
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDate("YMD", 20091001, 20091005, 1)); // repeat contents 5 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 25));

        task_ptr task = fam->add_task("t");
        task->addTime(ecf::TimeAttr(TimeSlot(10, 0)));
        task->addTime(ecf::TimeAttr(TimeSlot(11, 0)));
        task->addVerify(VerifyAttr(NState::COMPLETE, 50)); // task should complete 50 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_date_for_loop2.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_date_for_loop2.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_date_list) {
    std::cout << "Simulator:: ...test_repeat_date_list\n";
    // suite suite
    //   clock real <fixed date + time>
    //   family family
    //     repeat datelist YMD 20091001 20181001 20101001 # yyyymmdd
    //     task t
    //       time 10:00
    //   endfamily
    // endsuite

    Defs theDefs;
    {
        ClockAttr clockAttr;
        clockAttr.date(1, 10, 2009);
        suite_ptr suite = theDefs.add_suite("test_repeat_date_list");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDateList("YMD", {20091001, 20181001, 20101001}));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 3));

        task_ptr task = fam->add_task("t");
        task->addTime(ecf::TimeAttr(TimeSlot(10, 0)));
        task->addVerify(VerifyAttr(NState::COMPLETE, 3)); // task should complete 15 times
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_date.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_date.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_with_cron) {
    std::cout << "Simulator:: ...test_repeat_with_cron\n";
    // suite s
    //   clock real <today date>
    //   endclock <today date> + 1 week
    //   family f
    //     repeat date YMD 20091001 20091004 1  # yyyymmdd
    //     family plot
    //       complete plot/finish == complete
    //
    //       task finish
    //         trigger 1 == 0    # stops task from running
    //         complete checkdata::done or checkdata == complete
    //
    //       task checkdata
    //         event done
    //         cron <today date> + 2 minutes     # cron that run forever
    //      endfamily
    //   endfamily
    // endsuite

    Defs theDefs;
    {
        auto theLocalTime       = Calendar::second_clock_time();
        auto time_plus_2_minute = theLocalTime + boost::posix_time::minutes(2);
        ClockAttr clockAttr(theLocalTime, false /* real clock*/);
        ClockAttr end_clock(theLocalTime + boost::posix_time::hours(24 * 7), false /* real clock*/);

        suite_ptr suite = theDefs.add_suite("test_repeat_with_cron");
        suite->addClock(clockAttr);
        suite->add_end_clock(end_clock);

        family_ptr f = suite->add_family("f");
        f->addRepeat(RepeatDate("YMD", 20091001, 20091004, 1)); // repeat contents 4 times
        f->addVerify(VerifyAttr(NState::COMPLETE, 4));

        family_ptr family_plot = f->add_family("plot");
        family_plot->add_complete("plot/finish ==  complete");
        family_plot->addVerify(VerifyAttr(NState::COMPLETE, 4));

        task_ptr task_finish = family_plot->add_task("finish");
        task_finish->add_trigger("1 == 0");
        task_finish->add_complete("checkdata:done or checkdata == complete");
        task_finish->addVerify(VerifyAttr(NState::COMPLETE, 4));

        task_ptr task_checkdata = family_plot->add_task("checkdata");
        task_checkdata->addEvent(Event(1, "done"));

        CronAttr cronAttr;
        cronAttr.addTimeSeries(ecf::TimeSeries(ecf::TimeSlot(time_plus_2_minute.time_of_day())));
        task_checkdata->addCron(cronAttr);
        task_checkdata->addVerify(VerifyAttr(NState::COMPLETE, 8));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_with_cron.def"), errorMsg),
                          errorMsg << "\n"
                                   << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_with_cron.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_enumerated) {
    std::cout << "Simulator:: ...test_repeat_enumerated\n";
    // suite suite
    //   family family
    //     repeat enumerated ENUM "hello" "there" "bill" # run 3 times
    //     task t1
    //   endfamily
    // endsuite

    // Each task/job should be run 3 according to the repeats
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_enumerated");

        std::vector<std::string> theEnums;
        theEnums.emplace_back("hello"); // index 0
        theEnums.emplace_back("there"); // index 1
        theEnums.emplace_back("bill");  // index 2

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatEnumerated("ENUM", theEnums)); // repeat contents 3 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 3));

        task_ptr task = fam->add_task("t1");
        task->addVerify(VerifyAttr(NState::COMPLETE, 3));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_enumerated.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    auto tasks = ecf::get_all_tasks(theDefs);
    for (auto task : tasks) {
        // verify repeat has the last value
        auto* family         = dynamic_cast<Family*>(task->parent());
        const Repeat& repeat = family->findRepeat("ENUM");
        BOOST_REQUIRE_MESSAGE(!repeat.empty(), "Expected to find repeat on family " << family->absNodePath());
        BOOST_REQUIRE_MESSAGE(!repeat.valid(), "Expected invalid repeat");
        BOOST_REQUIRE_MESSAGE(repeat.value() == 3, "Expected to find repeat with value 3 but found " << repeat.value());
        BOOST_REQUIRE_MESSAGE(repeat.last_valid_value() == 2,
                              "Expected to find repeat with last valid value 2 but found "
                                  << repeat.last_valid_value());
    }

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_enumerated.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_repeat_string) {
    std::cout << "Simulator:: ...test_repeat_string\n";
    // suite suite
    //   family family
    //     repeat string STRING "hello" "there" # run 2 times
    //     task t1
    //   endfamily
    // endsuite

    // Each task/job should be run 3 according to the repeats
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_string");

        std::vector<std::string> theStrings;
        theStrings.emplace_back("hello"); // index 0
        theStrings.emplace_back("there"); // index 1

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatString("STRING", theStrings)); // repeat contents 2 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 2));

        task_ptr task = fam->add_task("t1");
        task->addVerify(VerifyAttr(NState::COMPLETE, 2));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_repeat_string.def"), errorMsg),
                        errorMsg << "\n"
                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    auto tasks = ecf::get_all_tasks(theDefs);
    for (auto task : tasks) {
        // verify repeat has the last value
        auto* family         = dynamic_cast<Family*>(task->parent());
        const Repeat& repeat = family->findRepeat("STRING");
        BOOST_REQUIRE_MESSAGE(!repeat.empty(), "Expected to find repeat on family " << family->absNodePath());
        BOOST_REQUIRE_MESSAGE(!repeat.valid(), "Expected invalid repeat");
        BOOST_REQUIRE_MESSAGE(repeat.value() == 2, "Expected to find repeat with value 2 but found " << repeat.value());
        BOOST_REQUIRE_MESSAGE(repeat.last_valid_value() == 1,
                              "Expected to find repeat with last valid value 1 but found "
                                  << repeat.last_valid_value());
    }

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_repeat_string.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

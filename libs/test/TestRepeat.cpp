/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/base/WhyCmd.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Repeat)

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE(test_repeat_integer) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // ********************************************************************************
    // IMPORTANT: A family will only complete when it has reached the end of the repeats
    // *********************************************************************************

    // Create the defs file corresponding to the text below
    // ECF_HOME variable is automatically added by the test harness.
    // ECF_INCLUDE variable is automatically added by the test harness.
    // SLEEPTIME variable is automatically added by the test harness.
    // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
    //                     This is substituted in sms includes
    //                     Allows test to run without requiring installation

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_repeat_integer
    //   repeat integer VAR 0 1 1          # run at 0, 1    2 times
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     repeat integer VAR 0 2 1     # run at 0, 1     2 times
    //     task t<n>
    //     ....
    //   endfamily
    // endsuite

    // Each task/job should be run *4* times, according to the repeats
    // Mimics nested loops
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_integer");
        suite->addRepeat(RepeatInteger("VAR", 0, 1, 1)); // repeat suite 2 times,
        suite->addVerify(VerifyAttr(NState::COMPLETE, 2));
        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatInteger("VAR", 0, 1, 1)); // repeat family 2 times
        fam->addVerify(VerifyAttr(NState::COMPLETE, 4));
        int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 4)); // task should complete 4 times
        }
    }

    // The test harness will create corresponding directory structure
    // and populate with standard sms files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_integer.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_repeat_date) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // ********************************************************************************
    // IMPORTANT: A family will only complete when it has reached the end of the repeats
    // *********************************************************************************
    // suite test_repeat_date
    // family family
    //   repeat date DATE 20110630 20110632
    //   task t<n>
    //      ....
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_date");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDate("DATE", 20110630, 20110704));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 5));
        int taskSize = 2;
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 5));
        }
    }

    // The test harness will create corresponding directory structure
    // and populate with standard sms files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_date.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_repeat_date_list) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // ********************************************************************************
    // IMPORTANT: A family will only complete when it has reached the end of the repeats
    // *********************************************************************************
    // suite test_repeat_date
    // family family
    //   repeat datelist DATE 20110630 20110632
    //   task t<n>
    //   ....
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_date_list");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatDateList("DATE", {20110630, 20110704}));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 2));
        int taskSize = 2;
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 2));
        }
    }

    // The test harness will create corresponding directory structure
    // and populate with standard sms files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_date.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_repeat_enumerator) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // ********************************************************************************
    // IMPORTANT: A family will only complete when it has reached the end of the repeats
    // *********************************************************************************
    // suite test_repeat_enumerator
    //   family top
    //     family plot
    //       family iasi_plots
    //         repeat enumerated month "200801" "200802"
    //         task t1
    //       endfamily
    //     endfamily
    //   endfamily
    // endsuite

    Defs theDefs;
    {
        suite_ptr suite       = theDefs.add_suite("test_repeat_enumerator");
        family_ptr top        = suite->add_family("top");
        family_ptr plot       = top->add_family("plot");
        family_ptr iasi_plots = plot->add_family("iasi_plots");
        vector<string> months;
        months.reserve(12);
        months.push_back("200801");
        months.push_back("200802");
        iasi_plots->addRepeat(RepeatEnumerated("month", months));
        task_ptr t1 = iasi_plots->add_task("t1");
        t1->addVerify(VerifyAttr(NState::COMPLETE, 2));
    }

    // The test harness will create corresponding directory structure
    // and populate with standard ecf files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_enumerator.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_repeat_defstatus) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // TEST SHOULD COMPLETE STRAIGHT AWAY SINCE WE HAVE A DEFSTATUS COMPLETE
    // since the complete state is set on all children of suite.

    // Create the defs file corresponding to the text below
    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_repeat_defstatus
    //   defstatus complete
    //   repeat integer VAR 0 1 1          # run at 0, 1    2 times
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     repeat integer VAR 0 2 1     # run at 0, 1     2 times
    //     task t<n>
    //     ....
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_repeat_defstatus");
        suite->addDefStatus(DState::COMPLETE);
        suite->addRepeat(RepeatInteger("VAR", 0, 1, 1));
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));

        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatInteger("VAR", 0, 1, 1));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));
        int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
        for (int i = 0; i < taskSize; i++) {

            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }
    }

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_defstatus.def"));
    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

// #define DEBUG_ME 1
BOOST_AUTO_TEST_CASE(test_repeat_clears_user_edit) {
    ECF_NAME_THIS_TEST();

    // Tests code:: Node::requeueOrSetMostSignificantStateUpNodeTree()
    // In *PARTICULAR* THE REQUE caused by the repeat, this ensures we clear NO_REQUE_IF_SINGLE_TIME_DEP
    // So that the effect of manual run/force complete are negated via automated re-queue caused by a REPEAT

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_repeat_clears_user_edit
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     repeat integer VAR 0 3     # run at 0,1,2    i.e 3 times
    //     task t<n>
    //       time <current time>
    //   endfamily
    // endsuite

    Defs theDefs;
    task_ptr task;
    {
        boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010, 6, 21), time_duration(10, 0, 0));
        boost::posix_time::ptime time1        = theLocalTime + minutes(3);

        suite_ptr suite = theDefs.add_suite("test_repeat_clears_user_edit");
        ClockAttr clockAttr(theLocalTime, false);
        suite->addClock(clockAttr);

        suite->addDefStatus(DState::SUSPENDED);
        family_ptr fam = suite->add_family("family");
        fam->addRepeat(RepeatInteger("VAR", 0, 2, 1)); // repeat family 3 times
        task = fam->add_task("t1");
        task->addTime(ecf::TimeAttr(ecf::TimeSlot(time1.time_of_day())));
        task->addVerify(VerifyAttr(NState::COMPLETE, 3));
    }

    // The test harness will create corresponding directory structure
    // and populate with standard sms files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs,
                          ServerTestHarness::testDataDefsLocation("test_repeat_integer.def"),
                          40,
                          false /* waitFortestcompletion */);

#ifdef DEBUG_ME
    PrintStyle style(PrintStyle::STATE);
    TestFixture::client().sync_local();
    defs_ptr server_defs = TestFixture::client().defs();
    cout << "At start ============================================\n";
    cout << server_defs << "\n";
#endif

    // USER EDIT, on task with a time. The force complete will expire the time.
    // Forcing a task with the time attribute, to complete state, should invalidate/expire the time,
    // Hence it should hold indefinitely, or until it is re-queued manually, or automatically via a Repeat/cron.
    // This Test ensures the the *REQUEUE* via the repeat, resets the time based attribute
    TestFixture::client().force(task->absNodePath(), "complete", true);
    TestFixture::client().force(task->absNodePath(), "complete", true);

#ifdef DEBUG_ME
    TestFixture::client().sync_local();
    server_defs = TestFixture::client().defs();
    cout << "After 2 force complete ============================================\n";
    cout << server_defs << "\n";

    node_ptr the_task = server_defs->findAbsNode(task->absNodePath());
    BOOST_REQUIRE_MESSAGE(the_task, "Task " << task->absNodePath() << " not found");
    WhyCmd whyCmd(server_defs, the_task->absNodePath());
    std::string reason = whyCmd.why();
    cout << "Why command ============================================\n";
    std::cout << reason << "\n\n";
#endif

    // resume the suspend suite
    TestFixture::client().resume("/test_repeat_clears_user_edit");

#ifdef DEBUG_ME
    TestFixture::client().sync_local();
    server_defs = TestFixture::client().defs();
    cout << "At Resume ============================================\n";
    cout << server_defs << "\n";
#endif

    // Wait for final LOOP of the repeat, and test to finish
    int timeout             = 20;
    bool verifyAttrInServer = true;
    defs_ptr serverDefs     = serverTestHarness.testWaiter(theDefs, timeout, verifyAttrInServer);
    BOOST_REQUIRE_MESSAGE(serverDefs.get(), " Failed to return defs after wait of 20 seconds");

#ifdef DEBUG_ME
    cout << "At test finish ============================================\n";
    cout << serverDefs << "\n";
#endif

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

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
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_ClkSync)

BOOST_AUTO_TEST_CASE(test_clk_sync) {
    ECF_NAME_THIS_TEST();

    // This test is used to test sync'ing of the suite calendars
    // The default clock type is *real*. We will create a suite with a hybrid clock attribute
    // For the suite calendar, we do not persist the clock type(hybrid/real), since this can be
    // obtained from the clock attribute. Hence a hybrid calendar in the server, will arrive as
    // real calendar at the client side. (i.e via the memento). It is then up to the client
    // to update the calendar with clock type stored in the clock attribute.
    // See:void Suite::set_memento( const SuiteCalendarMemento* memento )
    // This test(implicitly) will check that after an incremental sync that suite calendar and
    // suite clock attribute, that both are of the same clock type.
    // This is done in ServerTestHarness via invariant checking.

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Create the defs file corresponding to the text below
    // ECF_HOME variable is automatically added by the test harness.
    // ECF_INCLUDE variable is automatically added by the test harness.
    // SLEEPTIME variable is automatically added by the test harness.
    // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
    //                     This is substituted in ecf includes
    //                     Allows test to run without requiring installation

    // # Note: we have to use relative paths, since these tests are relocatable
    //  suite test_clk_sync
    //    clocl hybrid
    //    task a
    //       meter myMeter 0 100
    //  endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_clk_sync");
        suite->addClock(ClockAttr(true)); // add hybrid clock
        task_ptr task_a = suite->add_task("a");
        task_a->addMeter(Meter("myMeter", 0, 100, 100));
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_clk_sync.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_suite_calendar_sync) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // When using ECF_SSL sync is to slow.
    if (ecf::environment::has(ecf::environment::ECF_SSL)) {
        cout << " ignore test undel ECF_SSL\n";
        return;
    }

    // Test that sync_local(true), sync's the suite clock/calendar.
    Defs theDefs;
    {
        boost::posix_time::ptime today = Calendar::second_clock_time();
        suite_ptr suite                = theDefs.add_suite("test_suite_calendar_sync");
        family_ptr fam                 = suite->add_family("family");
        task_ptr task                  = fam->add_task("t1");

        // Don't use hybrid for day dependency as that will force node to complete if days is not the same
        ClockAttr clockAttr(today, false);
        suite->addClock(clockAttr);

        // ** add tomorrow days so that node stays queued, hence EXPECT no job and no output **
        task->addDay(DayAttr(today.date() + boost::gregorian::date_duration(1)));
    }

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs,
                          ServerTestHarness::testDataDefsLocation("test_suite_calendar_sync.def"),
                          1 /*timeout*/,
                          false /* waitForTestCompletion*/);

    // Get full defs, so that next sync_local does incremental update
    BOOST_REQUIRE_MESSAGE(TestFixture::client().getDefs() == 0,
                          CtsApi::get() << " failed should return 0 " << TestFixture::client().errorMsg());
    std::stringstream ss;
    ss << "\nStart time"
       << "\n  sync_full: suite time : " << TestFixture::client().defs()->suiteVec()[0]->calendar().toString()
       << " cal_count(" << TestFixture::client().defs()->updateCalendarCount() << ")\n";

    for (size_t i = 0; i < 3; i++) {
        // Occasionally we get a random failure.
        // It is suspected that BETWEEN the two calls below, one off
        //    Suite::updateCalendar() or Suite::resolveDependencies() is called in the server:
        // Unfortunately the data required to confirm this is not available on the client side: i.e.
        //  - Defs::updateCalendarCount_ not persisted with incremental clk sync
        //  - Suite::calendar_change_no_
        // To minimise this, avoid sleep that is same as job submission interval

        sleep(TestFixture::job_submission_interval() + 1); // expect 1 minute increment in suite_time.

        ss << "loop:" << i << "\n";

        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local(true /*sync suite clock*/) == 0,
                              "sync_local failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        boost::posix_time::ptime sync_clock_suiteTime =
            TestFixture::client().defs()->suiteVec()[0]->calendar().suiteTime();
        ss << "   Sync clock suite time:" << to_simple_string(sync_clock_suiteTime) << " full_sync("
           << TestFixture::client().server_reply().full_sync() << ")" << " in_sync("
           << TestFixture::client().server_reply().in_sync() << ") cal_count("
           << TestFixture::client().defs()->updateCalendarCount() << ")\n";

        // suiteVec is now invalidated
        BOOST_REQUIRE_MESSAGE(TestFixture::client().getDefs() == 0,
                              CtsApi::get() << " failed should return 0 " << TestFixture::client().errorMsg());
        boost::posix_time::ptime sync_full_suiteTime =
            TestFixture::client().defs()->suiteVec()[0]->calendar().suiteTime();
        ss << "   Sync full suite time :" << to_simple_string(sync_full_suiteTime) << " full_sync("
           << TestFixture::client().server_reply().full_sync() << ")" << " in_sync("
           << TestFixture::client().server_reply().in_sync() << ") cal_count("
           << TestFixture::client().defs()->updateCalendarCount() << ")\n";

        BOOST_REQUIRE_MESSAGE(sync_clock_suiteTime == sync_full_suiteTime,
                              ss.str() << "\nloop:" << i << " waited for " << TestFixture::job_submission_interval() + 1
                                       << "s for each loop"
                                          "\n"
                                       << TestFixture::client().defs());
    }

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

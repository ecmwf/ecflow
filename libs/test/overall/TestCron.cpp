// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Cron)

static void wait_for_cron(int max_time_to_wait, const std::string& path) {
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    TestFixture::client().set_throw_on_error(false);
    while (true) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "sync_local failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        if (defs) {
            node_ptr node = defs->findAbsNode(path);
            BOOST_REQUIRE_MESSAGE(node, "Could not find task at path " << path);
            if (node) {
                const std::vector<VerifyAttr>& verifys = node->verifys();
                BOOST_REQUIRE_MESSAGE(verifys.size() == 1, "Expected 1 verify");
                if (!verifys.empty()) {
                    if (verifys[0].actual() == verifys[0].expected()) {
                        break;
                    }
                }
            }
        }
        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < assertTimer.timeConstraint(),
                                  "wait_for_cron: Test wait " << assertTimer.duration()
                                                              << " taking longer than time constraint of "
                                                              << assertTimer.timeConstraint() << " aborting\n"
                                                              << ecf::as_string(*defs, PrintStyle::DEFS));
            break;
        }
        sleep(1);
    }
}

BOOST_AUTO_TEST_CASE(test_cron_time_series) {
    ECF_NAME_THIS_TEST();

    // Each poll of the test server (every TestFixture::job_submission_interval() seconds) advances the calendar by
    // one minute, and a cron time slot is taken only when the task is queued at that minute: a run still going at
    // the next slot misses it. The time series spans hours, so that a missed slot only delays the next run. The job
    // sleeps longer than the polls of one step, as a job on a loaded system can take, and must still complete the
    // expected number of times.
    const int job_duration = 2 * TestFixture::job_submission_interval() + 1; // seconds

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_cron_time_series
    //   edit SLEEPTIME 7 # with the usual 3-second poll
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   clock real 21.6.2010 +36000
    //   family family
    //     task t
    //       cron 10:01 22:01 00:02
    //       verify complete:3
    //   endfamily
    // endsuite
    Defs theDefs;
    std::string path;
    {
        // The clock starts at a fixed 10:00, rather than the current time, so that the whole time series lies
        // within one day (a series cannot end before it starts, as it would if started at 23:58)
        auto theLocalTime =
            boost::posix_time::ptime(boost::gregorian::date(2010, 6, 21), boost::posix_time::time_duration(10, 0, 0));
        auto time1 = theLocalTime + boost::posix_time::minutes(1);
        auto time2 = theLocalTime + boost::posix_time::hours(12) + boost::posix_time::minutes(1);

        suite_ptr suite = theDefs.add_suite("test_cron_time_series");
        suite->addVariable(Variable("SLEEPTIME", std::to_string(job_duration)));
        ClockAttr clockAttr(theLocalTime, false);
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addVerify(VerifyAttr(NState::COMPLETE, 3)); // task should complete 3 times

        CronAttr cronAttr;
        cronAttr.addTimeSeries(
            ecf::TimeSlot(time1.time_of_day()), ecf::TimeSlot(time2.time_of_day()), ecf::TimeSlot(0, 2));
        task->addCron(cronAttr);

        path = task->absNodePath();
    }

    // The test harness will create corresponding directory structure
    // and populate with standard ecf files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs,
                          ServerTestHarness::testDataDefsLocation("test_cron_time_series.def"),
                          1 /* timeout ignored*/,
                          false /*waitForTestCompletion*/);

    // crons are *infinite*, just wait for cron to complete 3 times
    wait_for_cron(180, path);

    std::cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount()
              << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

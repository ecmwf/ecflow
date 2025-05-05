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
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/DurationTimer.hpp"
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

BOOST_AUTO_TEST_SUITE(T_DayDate)

BOOST_AUTO_TEST_CASE(test_day_at_midnight) {
    ECF_NAME_THIS_TEST();

    // See ECFLOW-337 versus ECFLOW-1550
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // SLOW SYSTEMS
    // for each time attribute leave GAP of 3 * job submission interval
    // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
    // Also the task duration must be greater than job_submission_interval,  otherwise
    // we will get multiple invocation for the same time step

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_time_real_series
    //  edit SLEEPTIME 1
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //   clock real <date>
    //  family family
    //     day  monday
    //     time 23:59
    //     task t1
    //     task t2   # will stray into Tuesday but should still run
    //     task t3   # will stray into Tuesday but should still run
    //     endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_day_at_midnight");
        ClockAttr clockAttr(boost::posix_time::ptime(date(2019, 9, 9), time_duration(23, 58, 0)),
                            false); // Monday @ 23:58
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addTime(ecf::TimeAttr("23:59"));
        fam->addDay(DayAttr(DayAttr::MONDAY));
        fam->add_task("t1")->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr t2 = fam->add_task("t2");
        t2->addVerify(VerifyAttr(NState::COMPLETE, 1));
        t2->add_trigger("t1 == complete");

        task_ptr t3 = fam->add_task("t3");
        t3->addVerify(VerifyAttr(NState::COMPLETE, 1));
        t3->add_trigger("t2 == complete");
    }
    // cout << "\n" << theDefs;

    // The test harness will create corresponding directory structure
    // and populate with standard ecf files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_day_at_midnight.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_date_at_midnight) {
    ECF_NAME_THIS_TEST();

    // See ECFLOW-337 versus ECFLOW-1550
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_time_real_series
    //  edit SLEEPTIME 1
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  clock real <date>
    //  family family
    //     date 9.9.2019 # Monday
    //     time 23:59
    //     task t1
    //     task t2   # will stray into Tuesday but should still run
    //     task t3   # will stray into Tuesday but should still run
    //     endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_date_at_midnight");
        ClockAttr clockAttr(boost::posix_time::ptime(date(2019, 9, 9), time_duration(23, 58, 0)),
                            false); // Monday @ 23:58
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addTime(ecf::TimeAttr("23:59"));
        fam->addDate(DateAttr("9.9.2019"));
        fam->add_task("t1")->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr t2 = fam->add_task("t2");
        t2->addVerify(VerifyAttr(NState::COMPLETE, 1));
        t2->add_trigger("t1 == complete");

        task_ptr t3 = fam->add_task("t3");
        t3->addVerify(VerifyAttr(NState::COMPLETE, 1));
        t3->add_trigger("t2 == complete");
    }
    // cout << "\n" << theDefs;

    // The test harness will create corresponding directory structure
    // and populate with standard ecf files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_date_at_midnight.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

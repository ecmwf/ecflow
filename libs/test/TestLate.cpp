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
#include <limits> // for std::numeric_limits<int>::max()

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Converter.hpp"
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

BOOST_AUTO_TEST_SUITE(T_Late)

BOOST_AUTO_TEST_CASE(test_late) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    /// This test will sleep longer than the job submission interval
    /// which cause the task to be late
    /// as the active time has been set for 1 minute.
    /// The check for lateness is ONLY done are server poll time.
    /// Hence the task run time must be at least twice the poll time.
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_late");
        suite->add_variable(
            "SLEEPTIME",
            ecf::convert_to<std::string>(TestFixture::job_submission_interval() * 2)); // this will cause the late

        task_ptr task = suite->add_task("t1");
        ecf::LateAttr lateAttr;
        lateAttr.addComplete(ecf::TimeSlot(0, 1), true);

        task->addLate(lateAttr);

        suite->add_task("t2")->add_trigger(task->absNodePath() + "<flag>late");
    }

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_late.def"));

    TestFixture::client().set_throw_on_error(true);
    TestFixture::client().sync_local();
    BOOST_CHECK_MESSAGE(TestFixture::client().defs(), "Expected defs");

    node_ptr node = TestFixture::client().defs()->findAbsNode("/test_late/t1");
    BOOST_REQUIRE_MESSAGE(node, "Expected task to be found");

    ecf::LateAttr* late = node->get_late();
    BOOST_CHECK_MESSAGE(late->isLate(), "Expected late to be set");
    BOOST_CHECK_MESSAGE(node->flag().is_set(ecf::Flag::LATE), "Expected late flag to be set");

    node_ptr t2 = TestFixture::client().defs()->findAbsNode("/test_late/t2");
    BOOST_REQUIRE_MESSAGE(t2, "Expected task to be found");
    BOOST_CHECK_MESSAGE(t2->state() == NState::COMPLETE, "Expected late trigger to work");

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_late_hierarchically) {
    ECF_NAME_THIS_TEST();

    // ECFLOW-610
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    /// This test will sleep longer than the job submission interval
    /// which cause the task to be late
    /// as the active time has been set for 1 minute.
    /// The check for lateness is ONLY done are server poll time.
    /// Hence the task run time must be at least twice the poll time.
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_late_hierarchically");
        suite->add_variable(
            "SLEEPTIME",
            ecf::convert_to<std::string>(TestFixture::job_submission_interval() * 2)); // this will cause the late
        ecf::LateAttr lateAttr;
        lateAttr.addComplete(ecf::TimeSlot(0, 1), true);
        suite->addLate(lateAttr);

        family_ptr fam = suite->add_family("f1");
        fam->add_task("t1");
        fam->add_task("t2");
    }

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_late_hierarchically.def"));

    TestFixture::client().set_throw_on_error(true);
    TestFixture::client().sync_local();
    BOOST_CHECK_MESSAGE(TestFixture::client().defs(), "Expected defs");

    node_ptr t1 = TestFixture::client().defs()->findAbsNode("/test_late_hierarchically/f1/t1");
    BOOST_CHECK_MESSAGE(t1, "Expected task to be found");
    BOOST_CHECK_MESSAGE(t1->flag().is_set(ecf::Flag::LATE), "Expected late flag to be set");

    node_ptr t2 = TestFixture::client().defs()->findAbsNode("/test_late_hierarchically/f1/t2");
    BOOST_CHECK_MESSAGE(t2, "Expected task to be found");
    BOOST_CHECK_MESSAGE(t2->flag().is_set(ecf::Flag::LATE), "Expected late flag to be set");

    // cout << TestFixture::client().defs() << "\n";

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

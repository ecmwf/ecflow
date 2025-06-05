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

#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SpecificIssues)

BOOST_AUTO_TEST_CASE(test_ECFLOW_195) {
    ECF_NAME_THIS_TEST();

    defs_ptr defs = Defs::create();
    suite_ptr s1  = defs->add_suite("s1");
    family_ptr f1 = s1->add_family("f1");
    task_ptr t1   = f1->add_task("t1");

    s1->addLabel(Label("s1", "s1"));
    f1->addLabel(Label("f1", "f1"));
    t1->addLabel(Label("t1", "t1"));

    defs->beginAll();

    // Set the labels, with new values
    s1->changeLabel("s1", "xx");
    f1->changeLabel("f1", "xx");
    t1->changeLabel("t1", "xx");

    { // Check new values have not changed
        Label s1_label = s1->find_label("s1");
        BOOST_CHECK_MESSAGE(!s1_label.empty(), "expected to find label 's1'");
        BOOST_CHECK_MESSAGE(s1_label.new_value() == "xx", "expected xx but found " << s1_label.new_value());

        Label f1_label = f1->find_label("f1");
        BOOST_CHECK_MESSAGE(!f1_label.empty(), "expected to find label 'f1'");
        BOOST_CHECK_MESSAGE(f1_label.new_value() == "xx", "expected xx but found " << f1_label.new_value());

        Label t1_label = t1->find_label("t1");
        BOOST_CHECK_MESSAGE(!t1_label.empty(), "expected to find label 't1'");
        BOOST_CHECK_MESSAGE(t1_label.new_value() == "xx", "expected xx but found " << t1_label.new_value());
    }

    // Now requee. the suite and family should be cleared and task label should remain.
    defs->requeue();

    { // Suite and Family labels should be reset, and task labels should retain their values
        Label s1_label = s1->find_label("s1");
        BOOST_CHECK_MESSAGE(!s1_label.empty(), "expected to find label 's1'");
        BOOST_CHECK_MESSAGE(s1_label.new_value().empty(),
                            "expected empty string for suite label value after re-queue, but found "
                                << s1_label.new_value());

        Label f1_label = f1->find_label("f1");
        BOOST_CHECK_MESSAGE(!f1_label.empty(), "expected to find label 'f1'");
        BOOST_CHECK_MESSAGE(f1_label.new_value().empty(),
                            "expected empty string for family label value after re-queue, but found "
                                << f1_label.new_value());

        Label t1_label = t1->find_label("t1");
        BOOST_CHECK_MESSAGE(!t1_label.empty(), "expected to find label 't1'");
        BOOST_CHECK_MESSAGE(t1_label.new_value() == "xx",
                            "Expected task label to remain unchanged after re-queue but found "
                                << t1_label.new_value());
    }

    // After explicit re-queue expect new labels to be empty
    s1->requeue_labels();
    f1->requeue_labels();
    t1->requeue_labels();
    {
        Label s1_label = s1->find_label("s1");
        BOOST_CHECK_MESSAGE(!s1_label.empty(), "expected to find label 's1'");
        BOOST_CHECK_MESSAGE(s1_label.new_value().empty(),
                            "expected empty string for suite label value after explicit re-queue, but found "
                                << s1_label.new_value());

        Label f1_label = f1->find_label("f1");
        BOOST_CHECK_MESSAGE(!f1_label.empty(), "expected to find label 'f1'");
        BOOST_CHECK_MESSAGE(f1_label.new_value().empty(),
                            "expected empty string for family label value after explicit re-queue, but found "
                                << f1_label.new_value());

        Label t1_label = t1->find_label("t1");
        BOOST_CHECK_MESSAGE(!t1_label.empty(), "expected to find label 't1'");
        BOOST_CHECK_MESSAGE(t1_label.new_value().empty(),
                            "expected empty string for task label value after explicit re-queue, but found "
                                << t1_label.new_value());
    }
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_247) {
    ECF_NAME_THIS_TEST();

    defs_ptr defs = Defs::create();
    suite_ptr s1  = defs->add_suite("s1");
    family_ptr f1 = s1->add_family("f1");
    f1->add_complete("f1/t1 == complete");
    task_ptr t1 = f1->add_task("t1");
    task_ptr t2 = f1->add_task("t2");
    task_ptr t3 = f1->add_task("t3");
    task_ptr t4 = f1->add_task("t4");

    {
        defs->beginAll();
        t1->set_state(NState::COMPLETE);
        // cout << defs;

        t4->set_state(NState::ABORTED);
        t4->get_flag().set(ecf::Flag::FORCE_ABORT); // stopped bu user mimic, otherwise it be be queued

        Jobs jobs(defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);
        // cout << defs;

        BOOST_CHECK_MESSAGE(f1->state() == NState::ABORTED,
                            "The complete for family should not evaluate if child is aborted");
    }
    {
        defs->requeue();
        t1->set_state(NState::COMPLETE);
        // cout << defs;

        t4->set_state(NState::ACTIVE);

        Jobs jobs(defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);

        // cout << defs;
        BOOST_CHECK_MESSAGE(f1->state() == NState::ACTIVE,
                            "The complete for family should not evaluate if child is active");
    }

    {
        defs->requeue();
        t1->set_state(NState::COMPLETE);

        Jobs jobs(defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);

        t4->set_state(NState::SUBMITTED);

        BOOST_CHECK_MESSAGE(f1->state() == NState::SUBMITTED,
                            "The complete for family should not evaluate if child is submitted");
        // cout << defs;
    }

    {
        defs->requeue();
        t1->set_state(NState::COMPLETE);
        // cout << defs;

        Jobs jobs(defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);

        BOOST_CHECK_MESSAGE(f1->state() == NState::COMPLETE,
                            "The complete for family should not evaluate if child is submitted");
        // cout << defs;
    }
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_417_real_clock) {
    ECF_NAME_THIS_TEST();

    // Make sure reque resets calendar according to the clock attribute *FOR* a real clock

    defs_ptr defs = Defs::create();
    suite_ptr s1  = defs->add_suite("s1");
    s1->addRepeat(RepeatDay(1));
    s1->addClock(ClockAttr(1, 10, 2015, false /*real clock*/));
    task_ptr t1 = s1->add_task("t1");
    t1->addTime(ecf::TimeAttr(ecf::TimeSlot(16, 0), false));

    defs->beginAll();

    // ECF_DATE = year/month/day of month
    {
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001", "expected 20151001 but found " << ecf_date.theValue());
    }

    // now requeue and date should be reset
    {
        defs->requeue();
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001", "expected 20151001 but found " << ecf_date.theValue());
    }

    // Now change the clock date to 10.10.2015.
    {
        s1->changeClockDate("10.10.2015");
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010", "expected 20151010 but found " << ecf_date.theValue());
    }

    // now requeue
    {
        defs->requeue();
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010", "expected 20151010 but found " << ecf_date.theValue());
    }
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_417_hybrid_clock) {
    ECF_NAME_THIS_TEST();

    // ECFLOW-417
    // For a suite with a hybrid clock *AND* repeat day. requue should update calendar date, by the repeat day interval

    defs_ptr defs = Defs::create();
    suite_ptr s1  = defs->add_suite("s1");
    s1->addRepeat(RepeatDay(1));
    s1->addClock(ClockAttr(1, 10, 2015, true /*hybrid*/));
    task_ptr t1 = s1->add_task("t1");
    t1->addTime(ecf::TimeAttr(ecf::TimeSlot(16, 0), false));

    defs->beginAll();

    // ECF_DATE = year/month/day of month
    {
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001", "expected 20151001 but found " << ecf_date.theValue());
    }

    // now requeue and date should be incremented
    {
        defs->requeue();
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151002", "expected 20151002 but found " << ecf_date.theValue());
    }

    // now requeue again and date should be incremented
    {
        defs->requeue();
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151003", "expected 20151003 but found " << ecf_date.theValue());
    }

    // Now change the clock date to 10.10.2015. We expect date to decremented to 09.10.2015,
    // Since user should always re-queue after changing suite clock attributes.
    // This will get us back to the original date set by the user.
    {
        s1->changeClockDate("10.10.2015");
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151009", "expected 20151009 but found " << ecf_date.theValue());
    }

    // now requeue and date should be incremented
    {
        defs->requeue();
        const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
        BOOST_CHECK_MESSAGE(!ecf_date.empty(), "Did not find ECF_DATE");
        BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010", "expected 20151010 but found " << ecf_date.theValue());
    }

    // Now update calendar for more than 24 hours, and calendar date should *NOT* change for hybrid
    {
        using namespace boost::posix_time;
        using namespace boost::gregorian;

        boost::posix_time::ptime time_now                 = s1->calendar().suiteTime();
        boost::posix_time::time_duration serverPollPeriod = boost::posix_time::time_duration(0, 1, 0, 0);
        std::string expectedDate                          = "2015-Oct-10";

        for (int hour = 1; hour <= 60; hour++) {

            // Update calendar every hour, for 60 hours
            time_now += hours(1);
            CalendarUpdateParams param(time_now, serverPollPeriod, true, /* serverRunning */ false /* forTest */);

            defs->updateCalendar(param);

            // cout << "hour = " << hour << " timeAfterUpdate " << to_simple_string(s1->calendar().suiteTime()) << "\n";

            std::string actualDate = to_simple_string(s1->calendar().suiteTime().date());
            BOOST_CHECK_MESSAGE(actualDate == expectedDate,
                                "Expected '" << expectedDate << "' but found " << actualDate << " at hour " << hour);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

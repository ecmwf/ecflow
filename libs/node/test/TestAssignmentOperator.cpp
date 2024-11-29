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

#include "MyDefsFixture.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_AssignmentOperator)

BOOST_AUTO_TEST_CASE(test_defs_assignment_operator) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture theDefsFixture;

    Defs defs;
    defs = theDefsFixture.defsfile_;
    Ecf::set_debug_equality(true); // only has affect in DEBUG build
    BOOST_CHECK_MESSAGE(defs == theDefsFixture.defsfile_, "assignment failed");
    Ecf::set_debug_equality(false); // only has affect in DEBUG build

    Defs empty;
    defs = empty;
    Ecf::set_debug_equality(true); // only has affect in DEBUG build
    BOOST_CHECK_MESSAGE(defs == empty, "assignment failed");
    Ecf::set_debug_equality(false); // only has affect in DEBUG build

    BOOST_CHECK_MESSAGE(!(defs == theDefsFixture.defsfile_), "assignment failure EXPECTED");

    theDefsFixture.defsfile_ = empty;
    BOOST_CHECK_MESSAGE(theDefsFixture.defsfile_ == empty, "assignment failed");
}

BOOST_AUTO_TEST_CASE(test_suite_assignment_operator) {
    ECF_NAME_THIS_TEST();

    Suite empty("empty");

    Suite s1("s1");
    ClockAttr clockAttr(false);
    clockAttr.date(1, 1, 2009);
    clockAttr.set_gain_in_seconds(3600);
    s1.addClock(clockAttr);
    s1.addAutoCancel(ecf::AutoCancelAttr(2));
    s1.addVariable(Variable("VAR", "value"));
    s1.add_task("t1");
    s1.add_family("f1");
    std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();
    s1.addZombie(ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB, 10));
    s1.addZombie(ZombieAttr(ecf::Child::ECF, child_cmds, ecf::User::FAIL, 100));
    s1.addZombie(ZombieAttr(ecf::Child::PATH, child_cmds, ecf::User::BLOCK, 100));

    ecf::CronAttr cronAttr;
    ecf::TimeSlot start(0, 0);
    ecf::TimeSlot finish(10, 0);
    ecf::TimeSlot incr(0, 5);
    std::vector<int> weekdays;
    for (int i = 0; i < 7; ++i)
        weekdays.push_back(i);
    std::vector<int> daysOfMonth;
    for (int i = 1; i < 32; ++i)
        daysOfMonth.push_back(i);
    std::vector<int> months;
    for (int i = 1; i < 13; ++i)
        months.push_back(i);
    cronAttr.addTimeSeries(start, finish, incr);
    cronAttr.addWeekDays(weekdays);
    cronAttr.addDaysOfMonth(daysOfMonth);
    cronAttr.addMonths(months);
    s1.addCron(cronAttr);

    ecf::LateAttr lateAttr;
    lateAttr.addSubmitted(ecf::TimeSlot(3, 12));
    lateAttr.addActive(ecf::TimeSlot(3, 12));
    lateAttr.addComplete(ecf::TimeSlot(4, 12), true);

    std::string suiteLimit = "suiteLimit";
    s1.addLimit(Limit(suiteLimit, 10));

    DebugEquality debug_equality; // only as affect in DEBUG build
    Suite s2(s1);                 // s2 is copy
    BOOST_CHECK_MESSAGE(s1 == s2, "copy constructor failed");

    Suite s3("s3");
    s3 = empty;
    BOOST_CHECK_MESSAGE(s3 == empty, "assignment failed");

    s3 = s1;
    BOOST_CHECK_MESSAGE(s3 == s1, "assignment failed");

    s1 = s2;
    BOOST_CHECK_MESSAGE(s1 == s2, "assignment failed");
}

BOOST_AUTO_TEST_CASE(test_task_assignment_operator) {
    ECF_NAME_THIS_TEST();

    Task empty("empty");

    Task s1("s1");
    s1.addAutoCancel(ecf::AutoCancelAttr(2));
    s1.addVariable(Variable("VAR", "value"));
    std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();
    s1.addZombie(ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB, 10));
    s1.addZombie(ZombieAttr(ecf::Child::ECF, child_cmds, ecf::User::FAIL, 100));
    s1.addZombie(ZombieAttr(ecf::Child::PATH, child_cmds, ecf::User::BLOCK, 100));

    ecf::CronAttr cronAttr;
    ecf::TimeSlot start(0, 0);
    ecf::TimeSlot finish(10, 0);
    ecf::TimeSlot incr(0, 5);
    std::vector<int> weekdays;
    for (int i = 0; i < 7; ++i)
        weekdays.push_back(i);
    std::vector<int> daysOfMonth;
    for (int i = 1; i < 32; ++i)
        daysOfMonth.push_back(i);
    std::vector<int> months;
    for (int i = 1; i < 13; ++i)
        months.push_back(i);
    cronAttr.addTimeSeries(start, finish, incr);
    cronAttr.addWeekDays(weekdays);
    cronAttr.addDaysOfMonth(daysOfMonth);
    cronAttr.addMonths(months);
    s1.addCron(cronAttr);

    ecf::LateAttr lateAttr;
    lateAttr.addSubmitted(ecf::TimeSlot(3, 12));
    lateAttr.addActive(ecf::TimeSlot(3, 12));
    lateAttr.addComplete(ecf::TimeSlot(4, 12), true);

    std::string suiteLimit = "suiteLimit";
    s1.addLimit(Limit(suiteLimit, 10));

    s1.addDate(DateAttr(1, 2, 2009));
    s1.addDay(DayAttr(DayAttr::MONDAY));
    s1.addVariable(Variable("VAR1", "\"value\""));
    s1.addEvent(Event(1));
    s1.addEvent(Event(2, "eventname"));
    s1.addEvent(Event(3, "eventname2", true /* init value */));
    s1.addEvent(Event(4, "eventname4", true /* init value */, true /* value */));
    s1.addMeter(Meter("myMeter", 0, 100, 100));
    s1.addLabel(Label("label", "\"labelValue\""));
    s1.addTime(ecf::TimeAttr(ecf::TimeSlot(10, 10), true));
    s1.addToday(ecf::TodayAttr(ecf::TimeSlot(10, 12)));
    s1.addToday(ecf::TodayAttr(ecf::TimeSlot(0, 1), ecf::TimeSlot(0, 3), ecf::TimeSlot(0, 1), true));
    s1.addDefStatus(DState::COMPLETE);
    s1.addInLimit(InLimit(suiteLimit, "/name"));
    s1.addVerify(VerifyAttr(NState::COMPLETE, 3));
    s1.addLate(lateAttr);

    DebugEquality debug_equality; // only as affect in DEBUG build
    Task s2(s1);                  // s2 is copy
    BOOST_CHECK_MESSAGE(s1 == s2, "copy constructor failed");

    Task s3("s3");
    s3 = empty;
    BOOST_CHECK_MESSAGE(s3 == empty, "assignment failed");

    s3 = s1;
    BOOST_CHECK_MESSAGE(s3 == s1, "assignment failed");

    s1 = s2;
    BOOST_CHECK_MESSAGE(s1 == s2, "assignment failed");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

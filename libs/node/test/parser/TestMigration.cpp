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
#include <string>

#include <boost/test/unit_test.hpp>

#include "MyDefsFixture.hpp"
#include "PersistHelper.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;

// ********************************************************************
// These test are used to check that MIGRATE is equivalent to check pt
// MIGRATE will be used for migration from old to new release
// MIGRATE is essentially the defs structure with state.
// The state is written out as comments
// It is loaded like a normal Defs, the parser detects MIGRATE
// and loads the state in.
//
// By default  persistence/MIGRATE *ONLY* writes the state when it not the default.
// Hence the defaults should *NOT* change. These test will change the state
// to a non default value.
//
// Write the Defs with state and the compare with in memory defs
// Write the Defs as check pt an then compare with in memory defs
// Finally compare the two *RELOADED* defs file.
// ********************************************************************

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_Migration)

BOOST_AUTO_TEST_CASE(test_state_parser) {
    ECF_NAME_THIS_TEST();

    // **** The persistence will NOT write the defaults, hence we need to change the states
    // **** to test the persistence
    PersistHelper helper;
    std::vector<Flag::Type> flag_list = Flag::list();
    {
        Defs defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Empty Defs failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");

        // Change state other the default
        defs.beginAll();
        suite->set_state(NState::ABORTED);
        for (auto& i : flag_list)
            suite->flag().set(i);
        suite->suspend();
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one suite failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        family_ptr f1   = suite->add_family("f1");

        // Change state other the default
        f1->set_state(NState::COMPLETE);
        for (auto& i : flag_list)
            f1->flag().set(i);
        f1->suspend();
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one family failed: " << helper.errorMsg());

        // Test multiple
        suite->add_family("f2");
        suite->add_family("f3");
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one family failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        family_ptr f1 = defs.add_suite("s1")->add_family("f1");
        task_ptr t1   = f1->add_task("t1");

        for (auto& i : flag_list)
            t1->flag().set(i);
        t1->suspend();
        t1->set_state(NState::COMPLETE);

        // Use memento to modify task state
        SubmittableMemento memento("Jobs_password", "the_rid", "the abort  reason with spaces", 12);
        std::vector<ecf::Aspect::Type> aspects;
        t1->set_memento(&memento, aspects, false);

        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one task failed: " << helper.errorMsg());

        // Test multiple
        f1->add_task("t2");
        f1->add_task("t3");
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one task failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        task_ptr task = defs.add_suite("s1")->add_family("f1")->add_task("t1");
        alias_ptr t1  = task->add_alias_only();
        for (auto& i : flag_list)
            t1->flag().set(i);
        t1->suspend();
        t1->set_state(NState::COMPLETE);
        // Use memento to modify alias state
        SubmittableMemento memento("Jobs_password", "the_rid", "the abort  reason with spaces", 12);
        std::vector<ecf::Aspect::Type> aspects;
        t1->set_memento(&memento, aspects, false);
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add one alias failed: " << helper.errorMsg());

        // Test multiple
        task->add_alias_only();
        task->add_alias_only();
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Add multiple alias failed: " << helper.errorMsg());
    }
}

BOOST_AUTO_TEST_CASE(test_state_node_attributes) {
    ECF_NAME_THIS_TEST();

    PersistHelper helper;
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        task_ptr task   = suite->add_task("t1");
        ecf::LateAttr lateAttr;
        lateAttr.addSubmitted(ecf::TimeSlot(3, 12));
        lateAttr.addActive(ecf::TimeSlot(3, 12));
        lateAttr.addComplete(ecf::TimeSlot(4, 12), true);
        lateAttr.setLate(true);
        task->addLate(lateAttr);

        ecf::LateAttr lateAttr1;
        lateAttr1.addSubmitted(ecf::TimeSlot(3, 12));
        lateAttr1.addActive(ecf::TimeSlot(3, 12));
        lateAttr1.addComplete(ecf::TimeSlot(4, 12), false);
        lateAttr1.setLate(true);
        task_ptr task1 = suite->add_task("t2");
        task1->addLate(lateAttr1);

        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Late state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        task_ptr task = defs.add_suite("s1")->add_task("t1");
        Meter meter("meter", 0, 100, 100);
        meter.set_value(10);
        task->addMeter(meter);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Meter state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        task_ptr task = defs.add_suite("s1")->add_task("t1");
        Event event("event");
        event.set_value(true);
        Event event2(10, "event");
        event2.set_value(true);
        Event event3(10);
        event3.set_value(true);
        Event event4(10, "event4", true /*init value*/, true /*value*/);
        Event event5(10, "event5", true /*init value*/, false /*value*/);
        task->addEvent(event);
        task->addEvent(event2);
        task->addEvent(event3);
        task->addEvent(event4);
        task->addEvent(event5);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Event state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        family_ptr fam  = suite->add_family("f1");
        task_ptr t      = fam->add_task("t");
        std::vector<std::string> queue_items;
        queue_items.emplace_back("001");
        queue_items.emplace_back("002");
        queue_items.emplace_back("003");
        QueueAttr queue("queue", queue_items);
        queue.active();
        queue.active();
        QueueAttr queue1("queue1", queue_items);
        queue1.active();
        suite->add_queue(queue);
        suite->add_queue(queue1);
        fam->add_queue(queue);
        fam->add_queue(queue1);
        t->add_queue(queue);
        t->add_queue(queue1);
        // PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Queue state: failed: " << helper.errorMsg());
    }
    {
        {
            Defs defs;
            task_ptr task = defs.add_suite("s1")->add_task("t1");
            Label label("name", "value");
            label.set_new_value("new  value");
            task->addLabel(label);
            //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
            BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                                "Label state: failed: " << helper.errorMsg());
        }
        {
            Defs defs;
            suite_ptr suite = defs.add_suite("s1");
            Label label("name", "value");
            label.set_new_value("new  value");
            suite->addLabel(label);
            //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
            BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                                "Label state: failed: " << helper.errorMsg());
        }
        {
            Defs defs;
            suite_ptr suite = defs.add_suite("s1");
            Label label("name", "value\nvalue");
            suite->addLabel(label);
            //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
            BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                                "Label state: failed: " << helper.errorMsg());
        }
        {
            Defs defs;
            suite_ptr suite = defs.add_suite("s1");
            Label label("name", "value\nvalue");
            label.set_new_value("value\nwith\nmany\nnewlines");
            suite->addLabel(label);
            //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
            BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                                "Label state: failed: " << helper.errorMsg());
        }
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        task_ptr t1     = suite->add_task("t1");
        task_ptr t2     = suite->add_task("t2");
        task_ptr t3     = suite->add_task("t3");
        task_ptr t4     = suite->add_task("t4");
        Limit limit("limit", 10);
        limit.increment(1, t1->absNodePath());
        limit.increment(1, t2->absNodePath());
        limit.increment(1, t3->absNodePath());
        limit.increment(1, t4->absNodePath());
        suite->addLimit(limit);
        //       PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Limit state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        task_ptr t1     = suite->add_task("t1");
        InLimit inlimit1("limit", "/s1", 10, true); // limit has state when 'limit this node only' is set
        inlimit1.set_incremented(true);             // the state
        InLimit inlimit2("limit2", "/s1", 10);
        t1->addInLimit(inlimit1);
        t1->addInLimit(inlimit2);

        //       PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "InLimit state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        task_ptr t1     = suite->add_task("t1");
        task_ptr t2     = suite->add_task("t2");
        task_ptr t3     = suite->add_task("t3");
        task_ptr t4     = suite->add_task("t4");
        task_ptr t5     = suite->add_task("t5");
        task_ptr t6     = suite->add_task("t6");

        std::vector<std::string> stringList;
        stringList.reserve(3);
        stringList.emplace_back("20130101");
        stringList.emplace_back("20130201");
        stringList.emplace_back("20130301");

        RepeatEnumerated rep("AEnum", stringList);
        rep.increment();
        t1->addRepeat(rep);

        RepeatString rep2("AEnum", stringList);
        rep2.increment();
        t2->addRepeat(rep2);

        RepeatDate rep3("YMD", 20090916, 20090916, 1);
        rep3.increment();
        t3->addRepeat(rep3);

        RepeatInteger rep4("rep", 0, 100, 1);
        rep4.increment();
        t4->addRepeat(rep4);
        t4->increment_repeat();

        RepeatDay rep5(2);
        rep5.increment();
        t5->addRepeat(rep5);

        RepeatDateList rep6("YMD", {20090916, 20090916});
        rep6.increment();
        t6->addRepeat(rep6);

        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Repeat state: failed: " << helper.errorMsg());
    }
}

BOOST_AUTO_TEST_CASE(test_state_time_attributes) {
    ECF_NAME_THIS_TEST();

    PersistHelper helper;
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        suite->begin();
        task_ptr task = suite->add_task("t1");
        TimeAttr time(10, 10);
        time.setFree();
        time.miss_next_time_slot();
        TimeAttr time2(10, 10, true);
        time2.calendarChanged(suite->calendar());
        time2.setFree();
        time2.miss_next_time_slot();
        TimeAttr time3(TimeSlot(10, 10), TimeSlot(12, 10), TimeSlot(0, 10), true);
        time3.calendarChanged(suite->calendar());
        time3.setFree();
        time3.miss_next_time_slot();

        task->addTime(time);
        task->addTime(time2);
        task->addTime(time3);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Time state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        suite->begin();
        task_ptr task = suite->add_task("t1");
        TodayAttr time(10, 10);
        time.setFree();
        time.miss_next_time_slot();
        TodayAttr time2(10, 10, true);
        time2.calendarChanged(suite->calendar());
        time2.setFree();
        time2.miss_next_time_slot();
        TodayAttr time3(TimeSlot(10, 10), TimeSlot(12, 10), TimeSlot(0, 10), true);
        time3.calendarChanged(suite->calendar());
        time3.setFree();
        time3.miss_next_time_slot();
        task->addToday(time);
        task->addToday(time2);
        task->addToday(time3);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Today state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        task_ptr task = defs.add_suite("s1")->add_task("t1");
        DayAttr day;
        DayAttr day1;
        day1.setFree();
        DayAttr day2(DayAttr::FRIDAY);
        day2.setFree();
        task->addDay(day);
        task->addDay(day1);
        task->addDay(day2);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Day state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        task_ptr task = defs.add_suite("s1")->add_task("t1");
        DateAttr d;
        DateAttr d1;
        d1.setFree();
        DateAttr d2(1, 1, 2012);
        d2.setFree();
        DateAttr d3(0, 0, 2012);
        d3.setFree();
        task->addDate(d);
        task->addDate(d1);
        task->addDate(d2);
        task->addDate(d3);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Date state: failed: " << helper.errorMsg());
    }
    {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        suite->begin();
        task_ptr task  = suite->add_task("t1");
        task_ptr task2 = suite->add_task("t2");

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
        cronAttr.setFree();
        task->addCron(cronAttr);

        // Change TimeSeries state
        TimeSeries ts(start, finish, incr, true);
        ts.calendarChanged(suite->calendar());
        ts.miss_next_time_slot();
        cronAttr.addTimeSeries(ts);
        task2->addCron(cronAttr);

        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Date state: failed: " << helper.errorMsg());
    }

    // ZombieAttr do not have any changeable state
}

BOOST_AUTO_TEST_CASE(test_state_edit_history) {
    ECF_NAME_THIS_TEST();

    PersistHelper helper(true /* compare edit History */);
    Defs defs;
    suite_ptr suite = defs.add_suite("s1");
    defs.add_edit_history(suite->absNodePath(), "request1 with single spaces");
    defs.add_edit_history(suite->absNodePath(), "request2 with double  spaces");
    defs.add_edit_history(suite->absNodePath(), "request3_with_no_spaces!|?<>$%^&*()_{}:@<>?");
    suite_ptr suite2 = defs.add_suite("s2");
    defs.add_edit_history(suite2->absNodePath(), "request1 with single spaces");
    defs.add_edit_history(suite2->absNodePath(), "request2 with double  spaces");
    defs.add_edit_history(suite2->absNodePath(), "request3_with_no_spaces!|?<>$%^&*()_{}:@<>?");
    //   PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
    BOOST_REQUIRE_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                          "Edit history failed: " << helper.errorMsg());
}

static std::string dump_edit_history(const std::unordered_map<std::string, std::vector<std::string>>& edit_history) {
    std::stringstream ss;
    for (const auto& i : edit_history) {
        ss << "node: " << i.first << "\n";
        for (const auto& h : i.second) {
            ss << "  " << h << "\n";
        }
    }
    return ss.str();
}

BOOST_AUTO_TEST_CASE(test_state_edit_history_pruning) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr suite = defs.add_suite("s1");
    defs.add_edit_history(suite->absNodePath(), "MSG:[07:36:05 10.3.2016] --requeue force /s1  :maj");
    defs.add_edit_history(suite->absNodePath(), "MSG:[09:15:33 26.8.2016] --run /s1  :mab");
    defs.add_edit_history(suite->absNodePath(), "MSG:[06:45:18 26.8.2017] --force=queued /s1  :ma0");
    suite_ptr suite2 = defs.add_suite("s2");
    defs.add_edit_history(suite2->absNodePath(), "MSG:[19:13:44 21.6.2016] --suspend /s2  :ma");
    defs.add_edit_history(suite2->absNodePath(), "MSG:[11:28:12 6.10.2016] --requeue abort /s2  :ma");
    defs.add_edit_history(suite2->absNodePath(), "MSG:[09:15:33 26.8.2016] --run /s2  :mab");
    //   PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
    //   cout << dump_edit_history(defs.get_edit_history()) << "\n";

    std::string tmpFilename = "test_state_edit_history_pruning.def";
    defs.save_as_checkpt(tmpFilename);

    {
        // If any edit history is older than 1 day, then prune
        Defs reloaded_defs;
        reloaded_defs.ecf_prune_node_log(1);

        std::string errorMsg, warningMsg;
        BOOST_REQUIRE_MESSAGE(reloaded_defs.restore(tmpFilename, errorMsg, warningMsg),
                              "RE-PARSE failed for " << tmpFilename);

        const std::unordered_map<std::string, std::vector<std::string>>& edit_history =
            reloaded_defs.get_edit_history();
        BOOST_REQUIRE_MESSAGE(edit_history.empty(),
                              "Expected no edit history after pruning, but found:\n"
                                  << dump_edit_history(edit_history));
    }

    std::remove(tmpFilename.c_str());
}

BOOST_AUTO_TEST_CASE(test_state_edit_history_pruning2) {
    ECF_NAME_THIS_TEST();

    // Create edit history for today, this should not be affected by pruning
    date todays_date_in_utc = day_clock::universal_day();
    std::string history     = "MSG:[07:36:05 ";
    history += ecf::convert_to<std::string>(todays_date_in_utc.day());
    history += ".";
    history += ecf::convert_to<std::string>(todays_date_in_utc.month());
    history += ".";
    history += ecf::convert_to<std::string>(todays_date_in_utc.year());
    history += "] --requeue force /s1  :maj";

    Defs defs;
    suite_ptr suite = defs.add_suite("s1");
    defs.add_edit_history(suite->absNodePath(), history);
    defs.add_edit_history(suite->absNodePath(), history);
    defs.add_edit_history(suite->absNodePath(), history);
    //   PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
    //   cout << dump_edit_history(defs.get_edit_history()) << "\n";

    std::string tmpFilename = "test_state_edit_history_pruning2.def";
    defs.save_as_checkpt(tmpFilename);

    {
        // Since the history is todays it should be preserved, and not affected by pruning
        Defs reloaded_defs;
        reloaded_defs.ecf_prune_node_log(1);

        std::string errorMsg, warningMsg;
        BOOST_REQUIRE_MESSAGE(reloaded_defs.restore(tmpFilename, errorMsg, warningMsg),
                              "RE-PARSE failed for " << tmpFilename);

        const std::unordered_map<std::string, std::vector<std::string>>& edit_history =
            reloaded_defs.get_edit_history();
        //      cout << dump_edit_history(defs.get_edit_history()) << "\n";

        BOOST_REQUIRE_MESSAGE(!edit_history.empty(), "Expected edit history but found none");
    }

    std::remove(tmpFilename.c_str());
}

BOOST_AUTO_TEST_CASE(test_server_state) {
    ECF_NAME_THIS_TEST();

    PersistHelper helper(true /* compare edit History */);
    {
        Defs defs;
        defs.set_server().set_state(SState::HALTED);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Set server state failed " << helper.errorMsg());
    }
    {
        Defs defs;
        defs.set_server().set_state(SState::RUNNING);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Set server state failed " << helper.errorMsg());
    }
    {
        Defs defs;
        defs.set_server().set_state(SState::SHUTDOWN);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Set server state failed " << helper.errorMsg());
    }
    {
        Defs defs;
        std::vector<Variable> vec;
        vec.emplace_back("name", "value1");
        vec.emplace_back("name2", "val with 'spaces' ");
        vec.emplace_back("name3", "");
        defs.set_server().set_user_variables(vec);
        //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs),
                            "Set server variables failed " << helper.errorMsg());
    }
}

BOOST_AUTO_TEST_CASE(test_state_fixture_defs) {
    ECF_NAME_THIS_TEST();

    PersistHelper helper;
    MyDefsFixture theDefsFixture;
    BOOST_REQUIRE_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(theDefsFixture.defsfile_),
                          "Fixture failed: " << helper.errorMsg());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

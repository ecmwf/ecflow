/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Request
// Author      : Avi
// Revision    : $Revision$
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <string>
#include <iostream>
#include <fstream>

#include <boost/archive/tmpdir.hpp>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "PrintStyle.hpp"
#include "PersistHelper.hpp"
#include "Flag.hpp"
#include "Memento.hpp"
#include "MyDefsFixture.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

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

BOOST_AUTO_TEST_SUITE( ParserTestSuite )

BOOST_AUTO_TEST_CASE( test_state_parser )
{
   cout << "AParser:: ...test_state_parser\n";
   // **** The persistence will NOT write the defaults, hence we need to change the states
   // **** to test the persistence
   PersistHelper helper;
   std::vector<Flag::Type> flag_list = Flag::list();
   {
      Defs defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Empty Defs failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");

      // Change state other the default
      defs.beginAll();
      suite->set_state(NState::ABORTED);
      for (size_t i = 0; i < flag_list.size(); ++i)  suite->flag().set( flag_list[i] );
      suite->suspend();
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"Add one suite failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      family_ptr f1 = suite->add_family("f1");

      // Change state other the default
      f1->set_state(NState::COMPLETE);
      for (size_t i = 0; i < flag_list.size(); ++i)  f1->flag().set( flag_list[i] );
      f1->suspend();
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"Add one family failed: " <<  helper.errorMsg());

      // Test multiple
      suite->add_family("f2");
      suite->add_family("f3");
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"Add one family failed: " <<  helper.errorMsg());
   }
   {
      Defs defs;
      family_ptr f1 = defs.add_suite("s1")->add_family("f1");
      task_ptr t1 = f1->add_task("t1");

      for (size_t i = 0; i < flag_list.size(); ++i)  t1->flag().set( flag_list[i] );
      t1->suspend();
      t1->set_state(NState::COMPLETE);

      // Use memento to modify task state
      SubmittableMemento memento( "Jobs_password","the_rid","the abort  reason with spaces",12);
      t1->set_memento(&memento);

      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Add one task failed: " << helper.errorMsg());

      // Test multiple
      f1->add_task("t2");
      f1->add_task("t3");
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Add one task failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      task_ptr task = defs.add_suite("s1")->add_family("f1")->add_task("t1");
      alias_ptr t1 = task->add_alias_only();
      for (size_t i = 0; i < flag_list.size(); ++i)  t1->flag().set( flag_list[i] );
      t1->suspend();
      t1->set_state(NState::COMPLETE);
      // Use memento to modify alias state
      SubmittableMemento memento( "Jobs_password","the_rid","the abort  reason with spaces",12);
      t1->set_memento(&memento);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Add one alias failed: " << helper.errorMsg());

      // Test multiple
      task->add_alias_only();
      task->add_alias_only();
      //      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Add multiple alias failed: " << helper.errorMsg());
   }
}

BOOST_AUTO_TEST_CASE( test_state_node_attributes )
{
   cout << "AParser:: ...test_state_node_attributes\n";
   PersistHelper helper;
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      task_ptr task = suite->add_task("t1");
      ecf::LateAttr lateAttr;
      lateAttr.addSubmitted( ecf::TimeSlot(3,12) );
      lateAttr.addActive( ecf::TimeSlot(3,12) );
      lateAttr.addComplete( ecf::TimeSlot(4,12), true);
      lateAttr.setLate(true);
      task->addLate(lateAttr);

      ecf::LateAttr lateAttr1;
      lateAttr1.addSubmitted( ecf::TimeSlot(3,12) );
      lateAttr1.addActive( ecf::TimeSlot(3,12) );
      lateAttr1.addComplete( ecf::TimeSlot(4,12), false);
      lateAttr1.setLate(true);
      task_ptr task1 = suite->add_task("t2");
      task1->addLate(lateAttr1);

//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Late state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      task_ptr task = defs.add_suite("s1")->add_task("t1");
      Meter meter("meter",0,100,100); meter.set_value(10);
      task->addMeter(meter);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Meter state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      task_ptr task = defs.add_suite("s1")->add_task("t1");
      Event event("event");     event.set_value(true);
      Event event2(10,"event"); event2.set_value(true);
      Event event3(10);         event3.set_value(true);
      task->addEvent(event);
      task->addEvent(event2);
      task->addEvent(event3);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Event state: failed: " << helper.errorMsg());
   }
   {
      {
         Defs defs;
         task_ptr task = defs.add_suite("s1")->add_task("t1");
         Label label("name","value"); label.set_new_value("new  value");
         task->addLabel(label);
         //      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
         BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Label state: failed: " << helper.errorMsg());
      }
      {
         Defs defs;
         suite_ptr suite = defs.add_suite("s1");
         Label label("name","value"); label.set_new_value("new  value");
         suite->addLabel(label);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
         BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Label state: failed: " << helper.errorMsg());
      }
      {
         Defs defs;
         suite_ptr suite = defs.add_suite("s1");
         Label label("name","value\nvalue");
         suite->addLabel(label);
      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
         BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Label state: failed: " << helper.errorMsg());
      }
      {
         Defs defs;
         suite_ptr suite = defs.add_suite("s1");
         Label label("name","value\nvalue");  label.set_new_value("value\nwith\nmany\nnewlines");
         suite->addLabel(label);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
         BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Label state: failed: " << helper.errorMsg());
      }
   }
   {
       Defs defs;
       suite_ptr suite = defs.add_suite("s1");
       task_ptr t1 = suite->add_task("t1");
       task_ptr t2 = suite->add_task("t2");
       task_ptr t3 = suite->add_task("t3");
       task_ptr t4 = suite->add_task("t4");
       Limit limit("limit",10);
       limit.increment(1,t1->absNodePath());
       limit.increment(1,t2->absNodePath());
       limit.increment(1,t3->absNodePath());
       limit.increment(1,t4->absNodePath());
       suite->addLimit(limit);
//       PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
       BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Limit state: failed: " << helper.errorMsg());
    }

   // **** Note InLimit does not have any changeable state

   {
        Defs defs;
        suite_ptr suite = defs.add_suite("s1");
        task_ptr t1 = suite->add_task("t1");
        task_ptr t2 = suite->add_task("t2");
        task_ptr t3 = suite->add_task("t3");
        task_ptr t4 = suite->add_task("t4");
        task_ptr t5 = suite->add_task("t5");
        task_ptr t6 = suite->add_task("t6");

        std::vector<std::string> stringList; stringList.reserve(3);
        stringList.push_back("20130101");
        stringList.push_back("20130201");
        stringList.push_back("20130301");

        RepeatEnumerated rep("AEnum",stringList);
        rep.increment();
        t1->addRepeat( rep );

        RepeatString rep2("AEnum",stringList);
        rep2.increment();
        t2->addRepeat( rep2 );

        RepeatDate rep3("YMD",20090916,20090916,1);
        rep3.increment();
        t3->addRepeat( rep3 );

        RepeatInteger rep4("rep",0,100,1);
        rep4.increment();
        t4->addRepeat( rep4 );
        t4->increment_repeat();

        RepeatDay rep5(2);
        rep5.increment();
        t5->addRepeat( rep5 );

//        PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
        BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Repeat state: failed: " << helper.errorMsg());
     }
}

BOOST_AUTO_TEST_CASE( test_state_time_attributes )
{
   cout << "AParser:: ...test_state_time_attributes\n";
   PersistHelper helper;
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1"); suite->begin();
      task_ptr task = suite->add_task("t1");
      TimeAttr time(10,10);  time.setFree(); time.miss_next_time_slot();
      TimeAttr time2(10,10,true); time2.calendarChanged(suite->calendar()); time2.setFree(); time2.miss_next_time_slot();
      TimeAttr time3(TimeSlot(10,10),TimeSlot(12,10),TimeSlot(0,10),true); time3.calendarChanged(suite->calendar());time3.setFree(); time3.miss_next_time_slot();

      task->addTime(time);
      task->addTime(time2);
      task->addTime(time3);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Time state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1"); suite->begin();
      task_ptr task = suite->add_task("t1");
      TodayAttr time(10,10);  time.setFree(); time.miss_next_time_slot();
      TodayAttr time2(10,10,true); time2.calendarChanged(suite->calendar()); time2.setFree(); time2.miss_next_time_slot();
      TodayAttr time3(TimeSlot(10,10),TimeSlot(12,10),TimeSlot(0,10),true); time3.calendarChanged(suite->calendar());time3.setFree(); time3.miss_next_time_slot();
      task->addToday(time);
      task->addToday(time2);
      task->addToday(time3);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Today state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      task_ptr task = defs.add_suite("s1")->add_task("t1");
      DayAttr day;
      DayAttr day1; day1.setFree();
      DayAttr day2(DayAttr::FRIDAY); day2.setFree();
      task->addDay(day);
      task->addDay(day1);
      task->addDay(day2);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Day state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      task_ptr task = defs.add_suite("s1")->add_task("t1");
      DateAttr d;
      DateAttr d1; d1.setFree();
      DateAttr d2(1,1,2012); d2.setFree();
      DateAttr d3(0,0,2012); d3.setFree();
      task->addDate(d);
      task->addDate(d1);
      task->addDate(d2);
      task->addDate(d3);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Date state: failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1"); suite->begin();
      task_ptr task = suite->add_task("t1");
      task_ptr task2 = suite->add_task("t2");

      ecf::CronAttr cronAttr;
      ecf::TimeSlot start( 0, 0 );
      ecf::TimeSlot finish( 10, 0 );
      ecf::TimeSlot incr( 0, 5 );
      std::vector<int> weekdays;   for(int i=0;i<7;++i) weekdays.push_back(i);
      std::vector<int> daysOfMonth;for(int i=1;i<32;++i) daysOfMonth.push_back(i);
      std::vector<int> months;     for(int i=1;i<13;++i) months.push_back(i);
      cronAttr.addTimeSeries(start,finish,incr);
      cronAttr.addWeekDays( weekdays  );
      cronAttr.addDaysOfMonth(daysOfMonth);
      cronAttr.addMonths(  months );
      cronAttr.setFree();
      task->addCron(cronAttr);

      // Change TimeSeries state
      TimeSeries ts(start,finish,incr,true);
      ts.calendarChanged(suite->calendar());
      ts.miss_next_time_slot();
      cronAttr.addTimeSeries(ts);
      task2->addCron(cronAttr);

//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Date state: failed: " << helper.errorMsg());
   }

   // ZombieAttr do not have any changeable state
}

BOOST_AUTO_TEST_CASE( test_state_edit_history )
{
   cout << "AParser:: ...test_state_edit_history\n";
   PersistHelper helper(true /* compare edit History */);
   Defs defs;
   suite_ptr suite = defs.add_suite("s1");
   defs.add_edit_history(suite->absNodePath(),"request1 with single spaces");
   defs.add_edit_history(suite->absNodePath(),"request2 with double  spaces");
   defs.add_edit_history(suite->absNodePath(),"request3_with_no_spaces!|?<>$%^&*()_{}:@<>?");
   suite_ptr suite2 = defs.add_suite("s2");
   defs.add_edit_history(suite2->absNodePath(),"request1 with single spaces");
   defs.add_edit_history(suite2->absNodePath(),"request2 with double  spaces");
   defs.add_edit_history(suite2->absNodePath(),"request3_with_no_spaces!|?<>$%^&*()_{}:@<>?");
//   PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
   BOOST_REQUIRE_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Edit history failed: " << helper.errorMsg());
}

BOOST_AUTO_TEST_CASE( test_server_state )
{
   cout << "AParser:: ...test_server_state\n";
   PersistHelper helper(true /* compare edit History */);
   {
      Defs defs;
      defs.set_server().set_state(SState::HALTED);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Set server state failed " << helper.errorMsg());
   }
   {
      Defs defs;
      defs.set_server().set_state(SState::RUNNING);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Set server state failed " << helper.errorMsg());
   }
   {
      Defs defs;
      defs.set_server().set_state(SState::SHUTDOWN);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Set server state failed " << helper.errorMsg());
   }
   {
      Defs defs;
      std::vector<Variable> vec;
      vec.push_back(Variable("name","value1"));
      vec.push_back(Variable("name2","val with 'spaces' "));
      vec.push_back(Variable("name3",""));
      defs.set_server().set_user_variables(vec);
//      PrintStyle::setStyle(PrintStyle::MIGRATE); std::cout << defs;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs), "Set server variables failed " << helper.errorMsg());
   }
}

BOOST_AUTO_TEST_CASE( test_state_fixture_defs )
{
   cout << "AParser:: ...test_state_fixture_defs\n";
   PersistHelper helper;
   MyDefsFixture theDefsFixture;
   BOOST_REQUIRE_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(theDefsFixture.defsfile_), "Fixture failed: " << helper.errorMsg());
}

BOOST_AUTO_TEST_SUITE_END()


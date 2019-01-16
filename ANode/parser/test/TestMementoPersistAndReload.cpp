/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Request
// Author      : Avi
// Revision    : $Revision$
//
// Copyright 2009-2019 ECMWF.
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

#include "Defs.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "PersistHelper.hpp"
#include "Flag.hpp"
#include "Memento.hpp"

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

BOOST_AUTO_TEST_CASE( test_memento_persist_and_reload )
{
   std::vector<ecf::Aspect::Type> aspects;
   bool aspect_only = false;
   cout << "AParser:: ...test_memento_persist_and_reload\n";
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      StateMemento memento(NState::ABORTED);
      t->set_memento(&memento,aspects,aspect_only);

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"StateMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      NodeDefStatusDeltaMemento memento(DState::ABORTED);
      t->set_memento(&memento,aspects,aspect_only);

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeDefStatusDeltaMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      SuspendedMemento memento(true);
      t->set_memento(&memento,aspects,aspect_only);

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"SuspendedMemento failed: " << helper.errorMsg());

      SuspendedMemento memento1;
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"SuspendedMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Event event(1);
      NodeEventMemento memento(event);
      t->set_memento(&memento,aspects,aspect_only); // add event

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeEventMemento failed: " << helper.errorMsg());

      event.set_value(true);
      NodeEventMemento memento1(event);           // set event
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeEventMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Meter meter("meter",0,100);

      NodeMeterMemento memento(meter);
      t->set_memento(&memento,aspects,aspect_only); // add meter

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeMeterMemento failed: " << helper.errorMsg());

      meter.set_value(100);
      NodeMeterMemento memento1( meter);          // change meter
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeMeterMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Label label("label","xxx");

      NodeLabelMemento memento(label);
      t->set_memento(&memento,aspects,aspect_only); // add label;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLabelMemento failed: " << helper.errorMsg());

      label.set_new_value("yyy");
      NodeLabelMemento memento1( label );          // change label
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLabelMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Expression exp("1 == 0");

      NodeTriggerMemento memento(exp);
      t->set_memento(&memento,aspects,aspect_only); // add trigger;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTriggerMemento failed: " << helper.errorMsg());

      exp.setFree();
      NodeTriggerMemento memento1( exp );          // free trigger
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTriggerMemento failed: " << helper.errorMsg());

      exp.clearFree();
      NodeTriggerMemento memento2( exp );          // clear trigger
      t->set_memento(&memento2,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTriggerMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Expression exp("1 == 0");

      NodeCompleteMemento memento(exp);
      t->set_memento(&memento,aspects,aspect_only); // add trigger;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeCompleteMemento failed: " << helper.errorMsg());

      exp.setFree();
      NodeCompleteMemento memento1( exp );          // free trigger
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeCompleteMemento failed: " << helper.errorMsg());

      exp.clearFree();
      NodeCompleteMemento memento2( exp );          // clear trigger
      t->set_memento(&memento2,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeCompleteMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Repeat repeat(RepeatDate("YMD",20090916,20090916,1)  );

      NodeRepeatMemento memento(repeat);
      t->set_memento(&memento,aspects,aspect_only); // add repeat;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeRepeatMemento failed: " << helper.errorMsg());

      repeat.increment();
      NodeRepeatMemento memento1( repeat );          // change repeat
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeRepeatMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Limit limit("suiteLimit",10);

      NodeLimitMemento memento(limit);
      t->set_memento(&memento,aspects,aspect_only); // add limit;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLimitMemento failed: " << helper.errorMsg());

      std::set<std::string> paths;
      paths.insert("/s1/t1");

      limit.set_state(20,2,paths);
      NodeLimitMemento memento1( limit );          // change limit
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLimitMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      InLimit inlimit("suiteLimit","/path/to/node",2);

      NodeInLimitMemento memento(inlimit);
      t->set_memento(&memento,aspects,aspect_only); // add inlimit only, no state

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeInLimitMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      Variable variable("name","value");

      NodeVariableMemento memento(variable);
      t->set_memento(&memento,aspects,aspect_only); // add variable;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeVariableMemento failed: " << helper.errorMsg());

      variable.set_value("new value");
      NodeVariableMemento memento1( variable );          // change variable
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeVariableMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      ecf::LateAttr lateAttr;
      lateAttr.addSubmitted( ecf::TimeSlot(3,12) );
      lateAttr.addActive( ecf::TimeSlot(3,12) );
      lateAttr.addComplete( ecf::TimeSlot(4,12), true);

      NodeLateMemento memento(lateAttr);
      t->set_memento(&memento,aspects,aspect_only); // add late;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLateMemento failed: " << helper.errorMsg());

      lateAttr.setLate(true);
      NodeLateMemento memento1( lateAttr );          // change late
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeLateMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      ecf::TodayAttr attr(ecf::TimeSlot(10,12)) ;

      NodeTodayMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add today;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTodayMemento failed: " << helper.errorMsg());

      attr.setFree();
      NodeTodayMemento memento1( attr );          // change today
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTodayMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      ecf::TimeAttr attr(ecf::TimeSlot(10,12)) ;

      NodeTimeMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add time;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTimeMemento failed: " << helper.errorMsg());

      attr.setFree();
      NodeTimeMemento memento1( attr );          // change time
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeTimeMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      DayAttr attr(DayAttr::MONDAY);

      NodeDayMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add day;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeDayMemento failed: " << helper.errorMsg());

      attr.setFree();
      NodeDayMemento memento1( attr );          // change day
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeDayMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      DateAttr attr(1,2,2009);

      NodeDateMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add date;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeDateMemento failed: " << helper.errorMsg());

      attr.setFree();
      NodeDateMemento memento1( attr );          // change date
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeDateMemento failed: " << helper.errorMsg());
   }
   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      ecf::CronAttr attr;
      ecf::TimeSlot start( 0, 0 );
      ecf::TimeSlot finish( 10, 0 );
      ecf::TimeSlot incr( 0, 5 );
      std::vector<int> weekdays;   for(int i=0;i<7;++i) weekdays.push_back(i);
      std::vector<int> daysOfMonth;for(int i=1;i<32;++i) daysOfMonth.push_back(i);
      std::vector<int> months;     for(int i=1;i<13;++i) months.push_back(i);
      attr.addTimeSeries(start,finish,incr);
      attr.addWeekDays( weekdays  );
      attr.addDaysOfMonth(daysOfMonth);
      attr.addMonths(  months );

      NodeCronMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add cron;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeCronMemento failed: " << helper.errorMsg());

      attr.setFree();
      NodeCronMemento memento1( attr );          // change cron
      t->set_memento(&memento1,aspects,aspect_only);
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeCronMemento failed: " << helper.errorMsg());
   }

   {
      Defs defs;
      suite_ptr suite = defs.add_suite("s1");
      node_ptr t = suite->add_task("t1");

      std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();

      ZombieAttr attr(ecf::Child::USER, child_cmds, ecf::User::FOB,10);

      NodeZombieMemento memento(attr);
      t->set_memento(&memento,aspects,aspect_only); // add zombie;

      PersistHelper helper;
      BOOST_CHECK_MESSAGE( helper.test_state_persist_and_reload_with_checkpt(defs),"NodeZombieMemento failed: " << helper.errorMsg());
   }
}

BOOST_AUTO_TEST_SUITE_END()

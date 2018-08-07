//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "Defs.hpp"
#include "ExprAst.hpp"
#include "MockServer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_free_dep_cmd )
{
   cout << "Base:: ...test_free_dep_cmd\n";

   // Create a test and add the date and time dependencies
   ptime theLocalTime(date(2011,Nov,4),  time_duration(9,30,0));
   ptime time_plus_hour   =  theLocalTime +  hours(1);
   ptime time_plus_2_hour =  theLocalTime +  hours(2);
   date todaysDate = theLocalTime.date();
   date tomorrows_date   = todaysDate + date_duration(1);
   date tomorrows_date_1 = todaysDate + date_duration(2);

   // Get tomorrow as a day so that isFree fails.
   tm day_1 = to_tm(tomorrows_date);
   tm day_2 = to_tm(tomorrows_date_1);

   suite_ptr suite;
   task_ptr task;
   Defs theDefs; {
      suite = theDefs.add_suite( "test_time" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task = fam->add_task( "t" );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_hour.time_of_day()) ) );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_2_hour.time_of_day()) ) );
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(1,1), ecf::TimeSlot(1,3), ecf::TimeSlot(0,1),
               true /*relative to suite start*/
      ));
      task->addToday( ecf::TodayAttr( TimeSlot(time_plus_hour.time_of_day()) ) );
      task->addToday( ecf::TodayAttr( TimeSlot(time_plus_2_hour.time_of_day()) ) );
      task->addDate( DateAttr(tomorrows_date.day(),tomorrows_date.month(),tomorrows_date.year()) );
      task->addDate( DateAttr(tomorrows_date_1.day(),tomorrows_date_1.month(),tomorrows_date_1.year()) );
      task->addDay( DayAttr(static_cast<DayAttr::Day_t>(day_1.tm_wday) ));
      task->addDay( DayAttr(static_cast<DayAttr::Day_t>(day_2.tm_wday) ));
      task->add_trigger( "t2 == complete" );

      CronAttr cronAttr;
      cronAttr.addTimeSeries( ecf::TimeSlot(time_plus_hour.time_of_day()),
               ecf::TimeSlot(time_plus_2_hour.time_of_day()),
               ecf::TimeSlot(0,1)  );
      task->addCron( cronAttr );

      fam->add_task( "t2" );
      //  	cout << theDefs << "\n";
   }
   std::vector<task_ptr> vec;
   theDefs.get_all_tasks(vec);
   BOOST_REQUIRE_MESSAGE(vec.size() == 2,"Error in number of tasks");

   // This will initialise the calendar from the Clock attribute
   theDefs.beginAll();

   // First all trigger should fail to evaluate AND all dependencies should NOT be free
   BOOST_REQUIRE_MESSAGE(task->triggerAst(),"Expected trigger abstract syntax tree for task " << task->absNodePath());
   BOOST_REQUIRE_MESSAGE(!task->triggerAst()->evaluate(),"Trigger should not evaluate");

   const Calendar& cal = suite->calendar();
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Time should not be free");
   }
   BOOST_FOREACH(const ecf::TodayAttr&  attr, task->todayVec()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Today should not be free");
   }
   BOOST_FOREACH(const ecf::CronAttr&  attr, task->crons()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Cron should not be free");
   }
   BOOST_FOREACH(const DateAttr&  attr, task->dates()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Date should not be free");
   }
   BOOST_FOREACH(const DayAttr&  attr, task->days()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal),"Day should not be free");
   }


   // Invoke the FreeDepCmd
   MockServer mockServer(&theDefs);

   FreeDepCmd cmd(task->absNodePath(),true/*trigger*/,true/* all */);
   cmd.setup_user_authentification();
   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
   BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to for FreeDepCmd");


   // Dependencies should now all be free now.
   BOOST_REQUIRE_MESSAGE(task->get_trigger()->isFree(),"Trigger should evaluate");
   bool at_least_one_free = false;
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Time should be free");

   at_least_one_free = false;
   BOOST_FOREACH(const ecf::TodayAttr&  attr, task->todayVec()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Today should be free");

   at_least_one_free = false;
   BOOST_FOREACH(const ecf::CronAttr&  attr, task->crons()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Cron should be free");

   at_least_one_free = false;
   BOOST_FOREACH(const DateAttr&  attr, task->dates()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Date should be free");

   at_least_one_free = false;
   BOOST_FOREACH(const DayAttr&  attr, task->days()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Day should be free");
}

BOOST_AUTO_TEST_CASE( test_free_dep_cmd_single_time_slot )
{
   cout << "Base:: ...test_free_dep_cmd_single_time_slot\n";

   // We add a time dependency *AFTER* the suite/calendar time
   // This checks that we DO NOT re-queue a task with a future time dependency
   // when a time dependency has been freed
   ptime theLocalTime(date(2011,Nov,4),  time_duration(9,30,0));
   ptime time_plus_2minute =  theLocalTime + minutes(2);

   suite_ptr suite;
   task_ptr task;
   Defs theDefs; {
      suite = theDefs.add_suite( "test_time" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );
      task = suite->add_task( "t" );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_2minute.time_of_day()) ) );
   }

   // This will initialise the calendar from the Clock attribute
   theDefs.beginAll();

   // expect task to be holding at 9:30
   const Calendar& cal = suite->calendar();
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Time should not be free");
   }

   // Invoke the FreeDepCmd
   MockServer mockServer(&theDefs);
   FreeDepCmd cmd(task->absNodePath(),true/*trigger*/,true/* all */);
   cmd.setup_user_authentification();
   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
   BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to for FreeDepCmd");

   // expect task to be free
   bool at_least_one_free = false;
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Time should be free");

   // The crux of the test. Should not requeue.
   BOOST_CHECK_MESSAGE(task->state() == NState::ACTIVE,"Free dependency should have done an immediate job submission. Expected active state");
   task->complete();
   BOOST_CHECK_MESSAGE(task->state() == NState::COMPLETE,"Expected Complete but found " << NState::toString(task->state()) << ", free dependency should avoid re-queue for a future single time dependency.");
}

BOOST_AUTO_TEST_CASE( test_free_dep_cmd_with_time_series )
{
   cout << "Base:: ...test_free_dep_cmd_with_time_series\n";

   // We start the suite *IN BETWEEN* a time series, and the force free dependency
   // This checks that we DO NOT re-queue a task with a future time dependency
   // when a time dependency has been freed
   ptime theLocalTime(date(2011,Nov,4),  time_duration(9,29,0));
   suite_ptr suite;
   task_ptr task;
   Defs theDefs; {
      suite = theDefs.add_suite( "test_time" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );
      task = suite->add_task( "t" );
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(9,28), ecf::TimeSlot(9,30), ecf::TimeSlot(0,2),
               false /*relative to suite start*/
      ));
   }

   // This will initialise the calendar from the Clock attribute
   theDefs.beginAll();

   // expect task to be holding, at 9:29
   const Calendar& cal = suite->calendar();
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Time should not be free");
   }

   // Invoke the FreeDepCmd
   MockServer mockServer(&theDefs);
   FreeDepCmd cmd(task->absNodePath(),true/*trigger*/,true/* all */);
   cmd.setup_user_authentification();
   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
   BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to for FreeDepCmd");

   // expect task to be free
   bool at_least_one_free = false;
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Time should be free");

   // The crux of the test. Should not requeue, as we should have advanced the time slot beyond the END point
   // i.e FreeDepCmd will advance the time slot, beyond end time
   BOOST_CHECK_MESSAGE(task->state() == NState::ACTIVE,"Free dependency should have done an immediate job submission. Expected active state");
   task->complete();
   BOOST_CHECK_MESSAGE(task->state() == NState::COMPLETE,"Expected Complete but found " << NState::toString(task->state()) << ", free dependency should avoid re-queue for a future single time dependency.");
}

BOOST_AUTO_TEST_CASE( test_free_dep_cmd_with_time_series_2 )
{
   cout << "Base:: ...test_free_dep_cmd_with_time_series_2\n";

   // We start the suite *IN BETWEEN* a time series, and the force free dependency
   // This time we have a larger range, and *SHOULD* re-queue, even if we miss a time slot
   ptime theLocalTime(date(2011,Nov,4),  time_duration(9,29,0));
   suite_ptr suite;
   task_ptr task;
   Defs theDefs; {
      suite = theDefs.add_suite( "test_time" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );
      task = suite->add_task( "t" );
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(9,28), ecf::TimeSlot(9,34), ecf::TimeSlot(0,2),
               false /*relative to suite start*/
      ));
   }

   // This will initialise the calendar from the Clock attribute
   // Will initialise  nextTimeSlot=09:30
   theDefs.beginAll();

   // expect task to be holding, at 9:29
   const Calendar& cal = suite->calendar();
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      BOOST_CHECK_MESSAGE(!attr.isFree(cal)," Time should not be free");
   }

   // Invoke the FreeDepCmd. This will update next nextTimeSlot to 09:32
   MockServer mockServer(&theDefs);
   FreeDepCmd cmd(task->absNodePath(),true/*trigger*/,true/* all */);
   cmd.setup_user_authentification();
   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
   BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to for FreeDepCmd");

   // expect task to be free
   bool at_least_one_free = false;
   BOOST_FOREACH(const ecf::TimeAttr&  attr, task->timeVec()) {
      if (attr.isFree(cal)) { at_least_one_free = true; break;}
   }
   BOOST_CHECK_MESSAGE(at_least_one_free,"At least one Time should be free");

   // The crux of the test. Even if we advance the time slot we should *STILL* be in range
   BOOST_CHECK_MESSAGE(task->state() == NState::ACTIVE,"Free dependency should have done an immediate job submission. Expected active state");
   task->complete();
   BOOST_CHECK_MESSAGE(task->state() == NState::QUEUED,"Expected QUEUED but found " << NState::toString(task->state()) << ", free dependency should avoid re-queue for a future single time dependency.");
}


BOOST_AUTO_TEST_SUITE_END()

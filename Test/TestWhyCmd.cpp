//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #27 $ 
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
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "PrintStyle.hpp"
#include "ClientToServerCmd.hpp"
#include "AssertTimer.hpp"
#include "WhyCmd.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )

static unsigned int waitForWhy(const std::string& path, const std::string& why, int max_time_to_wait)
{
   unsigned int updateCalendarCount = 0;
   TestFixture::client().set_throw_on_error( false );
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {

      /// Why command relies on the Suite serializing the calendar. If this is changed we need to get the full defs
      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr server_defs = TestFixture::client().defs();
      updateCalendarCount = server_defs->updateCalendarCount();

      WhyCmd whyCmd( server_defs, path);
      std::string reason = whyCmd.why();
      // std::cout << reason;

      if (reason.find(why) != std::string::npos) {
         // see the reason why job is not running
         std::cout << reason;
         break;
      }

      BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
               "waitForWhy Test wait " << assertTimer.duration() <<
               " taking longer than time constraint of " << assertTimer.timeConstraint() <<
               " aborting\n" << *server_defs);
      sleep(1);
   }
   return updateCalendarCount;
}

BOOST_AUTO_TEST_CASE( test_why_day )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_day "<< flush;
   TestClean clean_at_start_and_end;

   Defs theDefs;
   {
      boost::posix_time::ptime today = Calendar::second_clock_time();
      suite_ptr suite  = Suite::create("test_why_day" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr task = Task::create(  "t1" );

      // Dont use hybrid for day dependency as that will force node to complete if days is not the same
      ClockAttr clockAttr(today, false);
      suite->addClock( clockAttr );

      // ** add tomorrow days so that node stays queued **
      task->addDay( DayAttr( today.date() +  boost::gregorian::date_duration(1 ) ) );

      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_day.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // The job should not be submitted and should stay queued since the day does not match
   // running the why command, should report dependency on day
   unsigned int updateCalendarCount = waitForWhy("/test_why_day/family/t1", "day", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_date )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_date "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_date" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr task = Task::create(  "t1" );

      // Don't use hybrid for date dependency as that will force node to complete if date is not the same
      boost::posix_time::ptime today = Calendar::second_clock_time();
      ClockAttr clockAttr(today, false);
      suite->addClock( clockAttr );

      // ** add tomorrow date so that node stays queued **
      task->addDate( DateAttr( today.date() + boost::gregorian::date_duration(1 )) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_date.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // The job should not be submitted and should stay queued since the day does not match
   // running the why command, should report dependency on day
   unsigned int updateCalendarCount = waitForWhy("/test_why_date/family/t1", "date", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_time )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_time "<< flush;

   Defs theDefs;
   {
      boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
      boost::posix_time::ptime   time1 =  theLocalTime -  hours(1);

      suite_ptr suite  = Suite::create("test_why_time" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr task = Task::create(  "t1" );

      ClockAttr clockAttr(theLocalTime);
      suite->addClock( clockAttr );

      task->addTime( ecf::TimeAttr(  ecf::TimeSlot(time1.time_of_day()) ) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }


   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_time.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // The job should not be submitted and should stay queued since the time does not match
   // running the why command, should report dependency on time
   unsigned int updateCalendarCount = waitForWhy("/test_why_time/family/t1", "time", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_today )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_today "<< flush;

   Defs theDefs;
   {
      // IMPORTANT: A Node with a today attribute is *free* to run *IF* the suite
      //            calendar time is greater than the today date.
      //            Hence if we start this test just before midnight i.e 11.20,
      //            and add hour , then the today time would be 00:20
      //            *i.e node is free to run*
      // This is not what we want for this test. i.e we depend on node being queued
      // in order to test the why command. Also the following tests, start
      // by deleting all nodes. This will fail due to active/submitted tasks
      // Hence can *NOT* use:
      // boost::posix_time::ptime theLocalTime =  Calendar::second_clock_time();
      // boost::posix_time::ptime time1 =  theLocalTime + hours(1);

      // Use a hard coded a time, to avoid failure if test is run just before midnight
      boost::posix_time::ptime theLocalTime(date(2010,2,10), hours(1));
      boost::posix_time::ptime time1 =  theLocalTime + hours(1);

      suite_ptr suite  = theDefs.add_suite("test_why_today" ) ;
      ClockAttr clockAttr(theLocalTime);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family(  "family" );
      task_ptr task = fam->add_task(  "t1" );
      task->addToday( ecf::TodayAttr(  ecf::TimeSlot(time1.time_of_day()) ) );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_today.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // The job should not be submitted and should stay queued since the time does not match
   // running the why command, should report dependency on time
   unsigned int updateCalendarCount = waitForWhy("/test_why_today/family/t1", "today", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_cron )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_cron "<< flush;

   Defs theDefs;
   {
      boost::posix_time::ptime   theLocalTime =  boost::posix_time::ptime(date(2010,6,21),time_duration(11,0,0));
      boost::posix_time::ptime   time1 =  theLocalTime - hours(3);
      boost::posix_time::ptime   time2 =  theLocalTime -  hours(1);
      boost::gregorian::date todaysDate = theLocalTime.date();

      suite_ptr suite  = Suite::create("test_why_cron" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr task = Task::create(  "t1" );

      ClockAttr clockAttr(theLocalTime,false/*real*/);
      suite->addClock( clockAttr );

      // ** IMPORTANT **, for a date in the future we must match day of week,
      // ***************  day of month, and year, i.e the results are ANDED
      // ***************  this can lead to unexpected time in the future
      // ***************  if all 3 dependencies are present, as *below*
      ecf::CronAttr cronAttr;
      ecf::TimeSlot start( time1.time_of_day() );  // in the past
      ecf::TimeSlot finish( time2.time_of_day() ); // in the past
      ecf::TimeSlot incr( 0, 5 );
      cronAttr.addTimeSeries(start,finish,incr);

      std::vector<int> weekDays; weekDays.push_back(todaysDate.day_of_week().as_number());
      std::vector<int> daysOfMonth; daysOfMonth.push_back( todaysDate.day() );
      std::vector<int> months; months.push_back( todaysDate.month() );
      cronAttr.addWeekDays(     weekDays  );
      cronAttr.addDaysOfMonth(  daysOfMonth );
      cronAttr.addMonths(       months );

      task->addCron(cronAttr);

      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_cron.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // The job should not be submitted and should stay queued we are past the cron time
   // running the why command, should report dependency on cron
   unsigned int updateCalendarCount = waitForWhy("/test_why_cron/family/t1", "cron", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_limit )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_limit "<< flush;

   // Testing for limits requires that we have least some jobs are submitted.
   // we need to kill these jobs, at the end of test.

   //	suite test_why_limit
   //    limit disk 50
   //	   family family
   //          inlimit /suite1:disk 50
   //	   		task t0
   //	   		task t1
   //	   		task t2
   //	   		task t3
   //	   		task t4
   //	   		task t5
   //	   endfamily
   //	endsuite

   Defs theDefs;
   {
      std::string suiteName = "test_why_limit";
      std::string pathToLimit = "/" + suiteName;
      suite_ptr suite = theDefs.add_suite(suiteName );
      suite->addVariable( Variable("ECF_TRIES","1") );
      suite->addLimit(Limit("disk",50));

      family_ptr fam = suite->add_family( "family");
      fam->addInLimit(InLimit("disk",pathToLimit,50));
      int taskSize = 6;
      for(int i=0; i < taskSize; i++) {
         fam->addTask( Task::create("t" + boost::lexical_cast<std::string>(i))  );
      }
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation( "test_why_limit.def" ),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Only one job should be submitted, at a time, we will query on task t5, which
   // should still be queued because the limit is full
   // running the why command, should report dependency on limit
   waitForWhy("/test_why_limit/family/t5", "limit", 10);

   // The main check is over. Wait for jobs to complete
   int updateCalendarCount = 0;
   TestFixture::client().set_throw_on_error(false) ;
   {
      AssertTimer assertTimer(50,false); // Bomb out after n seconds, fall back if test fail
      while (1) {
         BOOST_REQUIRE_MESSAGE(TestFixture::client().getDefs() == 0,CtsApi::get() << " failed should return 0\n" << TestFixture::client().errorMsg());
         defs_ptr defs = TestFixture::client().defs();
         updateCalendarCount = defs->updateCalendarCount();
         bool wait = false;
         vector<Task*> tasks; defs->getAllTasks(tasks);
         BOOST_FOREACH(Task* task, tasks) {
            if (task->state() != NState::COMPLETE) wait = true;
         }
         if (!wait) break;
         if ( assertTimer.duration() >=  assertTimer.timeConstraint()) {
            cout << "waitFor jobs to complete, wait time of " << assertTimer.duration() <<
                               " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                               " aborting, ......breaking out\n" << *defs << "\n";
         }
         sleep(1);
      }
   }
   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_trigger )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_trigger "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_trigger" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr task = Task::create(  "t1" );
      task->add_trigger(  "1 == 0");
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_trigger.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Waiting for a trigger expression which will never evaluate
   unsigned int updateCalendarCount = waitForWhy("/test_why_trigger/family/t1", "expression", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_meter )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_meter "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_meter" ) ;
      family_ptr fam = Family::create(  "family" );
      fam->addMeter( Meter("meter",0,100,100) );
      task_ptr task = Task::create(  "t1" );
      task->add_trigger(  "/test_why_meter/family:meter > 20");
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_meter.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Waiting for a trigger expression which references a meter.
   unsigned int updateCalendarCount = waitForWhy("/test_why_meter/family/t1", "/test_why_meter/family:meter", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_event )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_event "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_event" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr t1 = Task::create(  "t1" );
      task_ptr t2 = Task::create(  "t2" );
      t2->addEvent( Event(1,"theEvent") );
      t2->add_trigger(   "1 == 0" ); // make sure task t2 never runs
      t1->add_trigger(   "/test_why_event/family/t2:theEvent == set" );
      fam->addTask( t1 );
      fam->addTask( t2 );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_event.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Task t1, should stay queued because the event on task t2 is never set.
   unsigned int updateCalendarCount = waitForWhy("/test_why_event/family/t1", "/test_why_event/family/t2:theEvent", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_user_var )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_user_var "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_user_var" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr t1 = Task::create(  "t1" );
      suite->addVariable( Variable("user_var","10") );
      t1->add_trigger(  "/test_why_user_var:user_var eq 100" );
      fam->addTask( t1 );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_user_var.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Task t1, should stay queued because the user variable 'user_var' value  = 10, is not 100
   unsigned int updateCalendarCount = waitForWhy("/test_why_user_var/family/t1", "/test_why_user_var:user_var", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_gen_var )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_gen_var "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_gen_var" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr t1 = Task::create(  "t1" );
      t1->add_trigger( "/test_why_gen_var/family/t1:ECF_TRYNO eq 100" );
      fam->addTask( t1 );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_gen_var.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Task t1, should stay queued because the generated variable 'ECF_TRYNO' value  = 0, is not 100
   unsigned int updateCalendarCount = waitForWhy("/test_why_gen_var/family/t1", "/test_why_gen_var/family/t1:ECF_TRYNO", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_CASE( test_why_repeat )
{
   DurationTimer timer;
   cout << "Test:: ...test_why_repeat "<< flush;

   Defs theDefs;
   {
      suite_ptr suite  = Suite::create("test_why_repeat" ) ;
      family_ptr fam = Family::create(  "family" );
      task_ptr t1 = Task::create(  "t1" );
      suite->addRepeat( RepeatInteger("suite_repeat_var",0,3,1));
      fam->addRepeat( RepeatInteger("family_repeat_var",0,3,1));
      t1->add_trigger( "/test_why_repeat/family:family_repeat_var eq 100  or /test_why_repeat:suite_repeat_var eq 100" );
      fam->addTask( t1 );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_why_repeat.def"),
                         1/*timeout*/,
                         false /* waitForTestCompletion*/);

   // Task t1, should stay queued because the repeat variable on the suite and family do not match
   unsigned int updateCalendarCount = waitForWhy("/test_why_repeat/family/t1", "/test_why_repeat/family:family_repeat_var", 10);

   cout << " " << timer.duration() << " update-calendar-count(" << updateCalendarCount << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()


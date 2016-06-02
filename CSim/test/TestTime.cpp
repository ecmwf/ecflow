//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include "Simulator.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestUtil.hpp"

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace fs = boost::filesystem;

/// Simulate definition files that are created on then fly. This allows us to create
/// tests with todays date/time this speeds up the testr, we can also validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

BOOST_AUTO_TEST_CASE( test_time )
{
   cout << "Simulator:: ...test_time\n";

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   //  clock real <todays date>
   //	family family
   //   	task t1
   //      time <start>
   //   endfamily
   //endsuite

   // Initialise clock with todays date  then create a time attribute + minutes
   // such that the task should only run once, in the next minute
   boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
   boost::posix_time::ptime   time_plus_minute =  theLocalTime +  minutes(1);

   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_time");
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_minute.time_of_day()) ) );
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );  // expect task to complete 1 time

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_time.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_time_series )
{
   cout << "Simulator:: ...test_time_series\n";

   //suite suite
   //  clock real <sunday>
   //	family family
   //   	task t1
   //       time 00:30 23:59 04:00  # should run 6 times 00:30 4:30 8:30 12:30 16:30 20:30
   //  	endfamily
   //endsuite

   // Initialise real clock on a Moday, such that task should _only_ run
   // on Monday since we are using a hybrid clock
   Defs theDefs;
   {
      ClockAttr clockAttr(true/*false means use hybrid clock*/);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_time_series");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task("t");
      TimeSeries timeSeries(TimeSlot(00,30), TimeSlot(23,59), TimeSlot(4,0), false/* relative */);

      task->addTime( TimeAttr( timeSeries ));
      task->addVerify( VerifyAttr(NState::COMPLETE,6) );  // expect task to complete 6 times

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_time_series.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_time_and_date )
{
   cout << "Simulator:: ...test_time_and_date\n";

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   //  clock real <todays date>
   //	family family
   //   	task t1
   //       date  <today date>
   //       time <start>
   //  	endfamily
   //endsuite


   Defs theDefs;
   {
      // Initialise clock with todays date  then create a time attribute + minutes
      // such that the task should only run once, in the next minute
      boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
      boost::gregorian::date todaysDate = theLocalTime.date();
      boost::posix_time::ptime   time_plus_minute =  theLocalTime +  minutes(1);

      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite_ptr suite = theDefs.add_suite("test_time_and_date");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task("t");
      task->addDate( DateAttr(todaysDate.day(),todaysDate.month(),todaysDate.year()) );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_minute.time_of_day()) ) );
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );  // expect task to complete 1 time

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_time_and_date.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_time_and_tomorrows_date )
{
   cout << "Simulator:: ...test_time_and_tomorrows_date\n";

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   //  clock real <tomorrows date>
   // family family
   //    task t1
   //       date  <tomorrows date>
   //       time <start>
   //    endfamily
   //endsuite


   Defs theDefs;
   {
      // Initialise clock with todays date  then create a time attribute + minutes
      // such that the task should only run once, in the next minute
      boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
      boost::gregorian::date tomorrows_date = theLocalTime.date();
      tomorrows_date += days(1);
      boost::posix_time::ptime   time_plus_minute =  theLocalTime +  minutes(1);

      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite_ptr suite = theDefs.add_suite("test_time_and_tomorrows_date");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task("t");
      task->addDate( DateAttr(tomorrows_date.day(),tomorrows_date.month(),tomorrows_date.year()) );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_minute.time_of_day()) ) );
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );  // expect task to complete 1 time

      //    cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_time_and_tomorrows_date.def"), errorMsg),errorMsg);
}


BOOST_AUTO_TEST_CASE( test_multiple_times_and_dates )
{
   cout << "Simulator:: ...test_multiple_times_and_dates\n";

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   //  clock real <todays date>
   //	family family
   //    repeat integer 0 1 1  # repeat twice
   //   	task t1
   //       date  <today date>
   //       date  <tomrrows date>
   //       time <start>
   //       time <start>
   //  	endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date  then create a time attribute + minutes
      // Note: we don't use:
      //    boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
      // because if run at late we can end up with illegal for the hour, ie. > 24

      boost::gregorian::date todaysDate(2009,2,10);
      ptime theLocalTime(todaysDate, hours(3));
      boost::gregorian::date tomarrows_date =  todaysDate + date_duration(1);
      boost::posix_time::time_duration td_plus_minute  =  theLocalTime.time_of_day() +  minutes(1);
      boost::posix_time::time_duration td_plus_hour    =  theLocalTime.time_of_day() +  hours(1);

      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite_ptr suite = theDefs.add_suite("test_multiple_times_and_dates");
      suite->addClock( clockAttr );


      family_ptr fam = suite->add_family( "family" );
      //fam->addRepeat( RepeatInteger("rep",0,1,1) );
      task_ptr task = fam->add_task("t");
      task->addDate( DateAttr(todaysDate.day(),todaysDate.month(),todaysDate.year()) );
      task->addDate( DateAttr(tomarrows_date.day(),tomarrows_date.month(),tomarrows_date.year()) );
      task->addTime( TimeAttr( TimeSlot(td_plus_minute) ));
      task->addTime( TimeAttr( TimeSlot(td_plus_hour) ));
      task->addVerify( VerifyAttr(NState::COMPLETE,4) );  // expect task to complete 4 time

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_multiple_times_and_dates.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_multiple_times_and_dates_hybrid )
{
   cout << "Simulator:: ...test_multiple_times_and_dates_hybrid\n";

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   //  clock hybrid <todays date>
   //	family family
   //   	task t1
   //       date  <today date>
   //       date  <tomrrows date>
   //       time <start>
   //       time <start>
   //  	endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date  then create a time attribute + minutes
      // such that the task should only run once, in the next minute
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2012,2,22),time_duration(10,20,0));
      boost::gregorian::date todaysDate = theLocalTime.date();
      boost::gregorian::date tomorrows_date =  todaysDate + date_duration(1);
      boost::posix_time::time_duration td_plus_minute  =  theLocalTime.time_of_day() +  minutes(1);
      boost::posix_time::time_duration td_plus_10_minute  =  theLocalTime.time_of_day() +  minutes(10);

      suite_ptr suite = theDefs.add_suite("test_multiple_times_and_dates_hybrid");
      suite->addClock( ClockAttr(theLocalTime,true) ); // true means use hybrid clock

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addDate( DateAttr(todaysDate.day(),todaysDate.month(),todaysDate.year()) );
      task->addDate( DateAttr(tomorrows_date.day(),tomorrows_date.month(),tomorrows_date.year()) );
      task->addTime( TimeAttr( TimeSlot(td_plus_minute) ));
      task->addTime( TimeAttr( TimeSlot(td_plus_10_minute) ));
      task->addVerify( VerifyAttr(NState::COMPLETE,2) );  // expect task to complete 2 time
      //cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_multiple_times_and_dates_hybrid.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_multiple_times_and_days )
{
   cout << "Simulator:: ...test_multiple_times_and_days\n";

   //suite suite
   //  clock real <sunday>
   //	family family
   //    rep integer 0 1 1 # repeat twice
   //   	task t1
   //       day  monday
   //       day  tuesday
   //       time <start>
   //       time <start>
   //  	endfamily
   //endsuite

   // Initialise real clock on a sunday, such that task should run on Monday & tuesday twice
   Defs theDefs;
   {
      ClockAttr clockAttr(false/*false means use real clock*/);
      clockAttr.date(11,10,2009); // 11 October 2009 was a sunday
      suite_ptr suite = theDefs.add_suite("test_multiple_times_and_days");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      //fam->addRepeat( RepeatInteger("rep",0,1,1) );
      task_ptr task = fam->add_task("t");
      task->addDay( DayAttr(DayAttr::MONDAY) );
      task->addDay( DayAttr(DayAttr::TUESDAY) );
      task->addTime( TimeAttr( TimeSlot(10,0) ));
      task->addTime( TimeAttr( TimeSlot(20,0) ));
      task->addVerify( VerifyAttr(NState::COMPLETE,4) );  // expect task to complete 4 time

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_multiple_times_and_days.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_CASE( test_multiple_times_and_days_hybrid )
{
   cout << "Simulator:: ...test_multiple_times_and_days_hybrid\n";

   //suite suite
   //  clock real <sunday>
   //	family family
   //   	task t1
   //       day  monday
   //       day  tuesday
   //       time <start>
   //       time <start>
   //  	endfamily
   //endsuite

   // Initialise real clock on a Monday, such that task should _only_ run
   // on Monday since we are using a hybrid clock
   Defs theDefs;
   {
      ClockAttr clockAttr(true/*false means use hybrid clock*/);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_multiple_times_and_days_hybrid");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addDay( DayAttr(DayAttr::MONDAY) );
      task->addDay( DayAttr(DayAttr::TUESDAY) );
      task->addTime( TimeAttr( TimeSlot(10,0) ));
      task->addTime( TimeAttr( TimeSlot(11,0) ));
      task->addTime( TimeAttr( TimeSlot(20,0) ));
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );  // expect task to complete 3 times

      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_multiple_times_and_days_hybrid.def"), errorMsg),errorMsg);
}

BOOST_AUTO_TEST_SUITE_END()

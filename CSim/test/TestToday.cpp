//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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

BOOST_AUTO_TEST_CASE( test_today )
{
   cout << "Simulator:: ...test_today\n";

   //suite suite
   //  clock real <fixed date>
   //	family family
   //   	task t1
   //      	today <start>  # +1 minute
   //      	today <start>  # +2 minute
   //   endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock   then create a today attribute + minutes
      // such that the task should only run once, in the next minute
      ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(1,2,0));
      boost::posix_time::ptime time_plus_minute =  theLocalTime +  minutes(1);
      boost::posix_time::ptime time_plus_10_minute =  theLocalTime +  minutes(10);

      suite_ptr suite =  theDefs.add_suite( "test_today" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task( "t" );
      task->addToday( ecf::TodayAttr( TimeSlot(time_plus_minute.time_of_day()) ) );
      task->addToday( ecf::TodayAttr( TimeSlot(time_plus_10_minute.time_of_day()) ) );
      task->addVerify( VerifyAttr(NState::COMPLETE,2) );  // expect task to complete 2 time
      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   bool result = simulator.run(theDefs, TestUtil::testDataLocation("test_today.def"), errorMsg);

   BOOST_CHECK_MESSAGE(result,errorMsg);

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_today.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE( test_today_time_series )
{
   cout << "Simulator:: ...test_today_time_series\n";

   //suite suite
   //  clock real <monday>
   //	family family
   //   	task t1
   //         today 00:30 18:59 04:00  # should run 5 times 00:30 4:30 8:30 12:30 16:30
   //   endfamily
   //endsuite

   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite( "test_today_time_series" );
      ClockAttr clockAttr(true/*false means use hybrid clock*/);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task =  fam->add_task( "t" );
      TimeSeries timeSeries(TimeSlot(00,30), TimeSlot(18,59), TimeSlot(4,0), false/* relative */);

      task->addToday( TodayAttr( timeSeries ));
      task->addVerify( VerifyAttr(NState::COMPLETE,5) );
      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_today_time_series.def") , errorMsg),errorMsg);

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_today_time_series.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE( test_today_time_and_date )
{
   cout << "Simulator:: ...test_today_time_and_date\n";

   //suite suite
   //  clock real <todays date>
   //	family family
   //   	task t1
   //       date  <today date>
   //       time  <start>
   //       today <start>
   //   endfamily
   //endsuite
   Defs theDefs;
   {
      // To speed up simulation: start calendar with hour increment AND time attributes with hours only
      //
      // Task will only run if all time dependencies are satisfied
      boost::posix_time::ptime   theLocalTime = ptime(date(2010,2,10),hours(15));  ;
      boost::gregorian::date todaysDate = theLocalTime.date();
      boost::posix_time::ptime   time_plus_hour =  theLocalTime +  hours(1);

      suite_ptr suite = theDefs.add_suite( "test_today_time_and_date" );
      ClockAttr clockAttr(theLocalTime,false/*false means use real clock*/);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task( "t" );
      task->addDate( DateAttr(todaysDate.day(),todaysDate.month(),todaysDate.year()) );
      task->addTime( ecf::TimeAttr( TimeSlot(time_plus_hour.time_of_day()) ) );
      task->addToday( ecf::TodayAttr( TimeSlot(time_plus_hour.time_of_day()) ) );

      task->addVerify( VerifyAttr(NState::COMPLETE,1) );
      //  	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_today_time_and_date.def"),errorMsg),errorMsg);

//   cout << theDefs;

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_today_time_and_date.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()

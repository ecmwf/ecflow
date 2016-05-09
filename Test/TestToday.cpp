//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #35 $ 
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
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( TestSuite )

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
BOOST_AUTO_TEST_CASE( test_today_single_slot )
{
   DurationTimer timer;
   cout << "Test:: ...test_today_single_slot " << flush;
   TestClean clean_at_start_and_end;

   // **************************************************************************
   // It does not really make sense to have multiple today attributes
   // since as soon as one today has expired the task is free to run
   // suite test_today_mutiple_single_slot
   //    edit SLEEPTIME 1
   //    edit ECF_INCLUDE $ECF_HOME/includes
   //    clock real <todays date>
   //    family family
   //       task t1
   //          today 10.01  // after 10.01, the node is free, for 10.02,10.03
   //          today 10.04
   //    endfamily
   // endsuite

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_today_single_slot
   //	edit SLEEPTIME 1
   //	edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   //	family family
   //   	task t1
   //       today 11.12
   //  	endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a today attribute
      // one hour past the clock todays time. The node should be free to run.
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time_minus_hour =  theLocalTime -  hours(1);

      suite_ptr suite = theDefs.add_suite( "test_today_single_slot");
      ClockAttr clockAttr(theLocalTime,false,true/*positive gain*/);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family");
      task_ptr task = fam->add_task( "t");
      task->addToday( ecf::TodayAttr(ecf::TimeSlot(time_minus_hour.time_of_day())) );
   }

   // The test harness will create corresponding directory structure
   // and populate with standard sms files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_today_single_slot.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_today_relative_time_series )
{
   DurationTimer timer;
   cout << "Test:: ...test_today_relative_time_series " << flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_today_relative_time_series
   //	edit SLEEPTIME 1
   //	edit ECF_INCLUDE $ECF_HOME/includes
   // clock real <todays date>
   //	family family
   //   	task t1
   //       today <start> <finish> <incr>
   //  	endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a today attribute
      // with a time series, so that task runs 3 times
      suite_ptr suite = theDefs.add_suite( "test_today_relative_time_series");
      suite->add_variable("SLEEPTIME",boost::lexical_cast<std::string>(TestFixture::job_submission_interval()-1));
      ClockAttr clockAttr(Calendar::second_clock_time(),false);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family");
      task_ptr task = fam->add_task( "t");
      task->addToday( ecf::TodayAttr(
               ecf::TimeSlot(0,1),
               ecf::TimeSlot(0,7),
               ecf::TimeSlot(0,3),
               true /*relative to suite start*/
      )
      );
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );      // task should complete 3 times
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_today_relative_time_series.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_today_real_time_series )
{
   DurationTimer timer;
   cout << "Test:: ...test_today_real_time_series " << flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_today_real_time_series
   //	edit ECF_INCLUDE $ECF_HOME/includes
   // clock real <todays date>
   //	family family
   //   	task t1
   //       today 10:01 10:07 00:03
   //  	endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with a fixed date and time, then create a today attribute
      // with a time series, so that task runs 3 times
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time1 = theLocalTime + minutes(1);
      boost::posix_time::ptime time2 = theLocalTime + minutes(7);

      suite_ptr suite = theDefs.add_suite( "test_today_real_time_series");
      ClockAttr clockAttr(theLocalTime,false);
      suite->addClock( clockAttr );
      suite->add_variable("SLEEPTIME",boost::lexical_cast<std::string>(TestFixture::job_submission_interval()-1));
      suite->addVerify( VerifyAttr(NState::COMPLETE,1) );

      family_ptr fam = suite->add_family( "family");
      fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
      task_ptr task = fam->add_task( "t");
      task->addToday( ecf::TodayAttr(
               ecf::TimeSlot(time1.time_of_day()),
               ecf::TimeSlot(time2.time_of_day()),
               ecf::TimeSlot(0,3)
      )
      );
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );      // task should complete 3 times
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_today_real_time_series.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

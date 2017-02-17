//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #35 $
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
#include "PrintStyle.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( TestSuite )

BOOST_AUTO_TEST_CASE( test_single_real_time )
{
   DurationTimer timer;
   cout << "Test:: ...test_single_real_time " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   // family family
   //    task t1
   //         time 10:00
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with fixed date and time, then create a time attribute
      // with todays time + minute
      // Avoid adding directly to TimeSlot
      // i.e if local time is 9:59 and we create a TimeSlot like
      //       task->addTime( ecf::TimeAttr( ecf::TimeSlot(theTm.tm_hour,theTm.tm_min+3) )  );
      // The the minute will be 62, which is illegal and will not parse
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time1 =  theLocalTime +  minutes(1);

      // For each 2 seconds of poll in the server update calendar by 1 minute
      // Note: if we use 1 seconds poll to update calendar by 1 minute, then
      //       we will find that state change happens at job submission interval,
      //       and hence skews time series.  Which can leave state in a queued state,
      //       and hence test never completes
      suite_ptr suite = theDefs.add_suite("test_single_real_time");
      ClockAttr clockAttr(theLocalTime,false);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr(ecf::TimeSlot(time1.time_of_day())));
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_single_real_time.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_single_time_trigger )
{
   // ECFLOW-833
   DurationTimer timer;
   cout << "Test:: ...test_single_time_trigger " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   // family family
   //    task t1
   //         trigger /suite:TIME == 1001
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with fixed date and time, then create a time attribute
      // with todays time + minute
      // Avoid adding directly to TimeSlot
      // i.e if local time is 9:59 and we create a TimeSlot like
      //       task->addTime( ecf::TimeAttr( ecf::TimeSlot(theTm.tm_hour,theTm.tm_min+3) )  );
      // The the minute will be 62, which is illegal and will not parse
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));

      // For each 2 seconds of poll in the server update calendar by 1 minute
      // Note: if we use 1 seconds poll to update calendar by 1 minute, then
      //       we will find that state change happens at job submission interval,
      //       and hence skews time series.  Which can leave state in a queued state,
      //       and hence test never completes
      suite_ptr suite = theDefs.add_suite("test_single_time_trigger");
      ClockAttr clockAttr(theLocalTime,false);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      std::stringstream ss; ss << "/test_single_time_trigger:TIME == " << "1001";
      task->add_trigger( ss.str());
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_single_time_trigger.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_time_multiple_single_slot )
{
   DurationTimer timer;
   cout << "Test:: ...test_time_multiple_single_slot "<< flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step.
   // *sometimes* just submitted->active can take many times job submission interval.

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_time_multiple_single_slot
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   // family family
   //    task t1
   //         time 10:01
   //         time 10:04
   //         time 10:07
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a time attribute
      // with todays time + minute.
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time1 = theLocalTime + minutes(1);
      boost::posix_time::ptime time2 = time1 + minutes(TestFixture::job_submission_interval());
      boost::posix_time::ptime time3 = time2 + minutes(TestFixture::job_submission_interval());

      suite_ptr suite = theDefs.add_suite("test_time_multiple_single_slot");
      ClockAttr clockAttr(theLocalTime,false);
      suite->addClock( clockAttr );
      suite->add_variable("SLEEPTIME","1");

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(time1.time_of_day()) )  );
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(time2.time_of_day()) )  );
      task->addTime( ecf::TimeAttr( ecf::TimeSlot(time3.time_of_day()) )  );
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );      // task should complete 3 times
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_time_multiple_single_slot.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}


BOOST_AUTO_TEST_CASE( test_time_relative_time_series )
{
   DurationTimer timer;
   cout << "Test:: ...test_time_relative_time_series " << flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_time_relative_time_series
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   // family family
   //    task t1
   //       time +<start> <finish> incr
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a time attribute
      // with a time series, so that task runs 3 times relative to suite start
      suite_ptr suite = theDefs.add_suite("test_time_relative_time_series");
      ClockAttr clockAttr(Calendar::second_clock_time(),false);
      suite->addClock( clockAttr );
      suite->add_variable("SLEEPTIME",boost::lexical_cast<std::string>(TestFixture::job_submission_interval()-1));

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr(
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
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_time_relative_time_series.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_time_real_series )
{
   DurationTimer timer;
   cout << "Test:: ...test_time_real_series " << flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_time_real_series
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <date>
   // family family
   //    task t1
   //       time <start> <finish> <incr>
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a time attribute
      // with a time series, so that task runs 3 times
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time1 = theLocalTime + minutes(1);
      boost::posix_time::ptime time2 = time1 + minutes(TestFixture::job_submission_interval()*2);

      suite_ptr suite = theDefs.add_suite("test_time_real_series");
      suite->add_variable("SLEEPTIME","1");

      ClockAttr clockAttr(theLocalTime,false);
      suite->addClock( clockAttr );

      family_ptr  fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr(
               ecf::TimeSlot(time1.time_of_day()),
               ecf::TimeSlot(time2.time_of_day()),
               ecf::TimeSlot(0,TestFixture::job_submission_interval())
      ));
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );      // task should complete 3 times
      // 1 +    7  + 13
      // 1 + (2*n) + (2*n) = 1 + 4n
      // start = 1, finish = 13, when n=3, when n = job submission interval
      // to complete 3 times, we must use interval of n*2
   }

   // The test harness will create corresponding directory structure
   // and populate with standard sms files.
   ServerTestHarness serverTestHarness;
   //serverTestHarness.add_default_sleep_time(false); // avoid missing time steps due to submit->active->complete > job submission interval
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_time_real_series.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_single_real_time_near_midnight )
{
   DurationTimer timer;
   cout << "Test:: ...test_single_real_time_near_midnight " << flush;
   int the_server_version = TestFixture::server_version() ;
   if (the_server_version == 403 ) {
      cout << " SKIPPING, This test does not work with 403, current server version is " << the_server_version << "\n";
      return;
   }

   TestClean clean_at_start_and_end;

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite suite
   // edit SLEEPTIME 4
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   // family family
   //    task t1
   //         time 23:59
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // ECFLOW-130
      // Make sure job completes after midnight.
      // The task SHOULD stay complete and *NOT* requeue
      // Since TestHarness requires suite completion, we dont need to do anything.
      boost::posix_time::ptime time_start = boost::posix_time::ptime(date(2010,6,21),time_duration(23,59,0));
      boost::posix_time::ptime clock_start = time_start -  minutes(1);

      suite_ptr suite = theDefs.add_suite("test_single_real_time_near_midnight");
      ClockAttr clockAttr(clock_start,false);
      suite->addClock( clockAttr );
      suite->add_variable("SLEEPTIME",boost::lexical_cast<string>(TestFixture::job_submission_interval()*2));

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr(ecf::TimeSlot(time_start.time_of_day())));
      task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times
   }

   // The test harness will create corresponding directory structure and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_single_real_time_near_midnight.def"));

#ifdef DEBUG_ME
   BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "Could not get the defs from server\n" << TestFixture::client().errorMsg());
   defs_ptr defs = TestFixture::client().defs();
   PrintStyle::setStyle(PrintStyle::STATE);
   std::cout << *defs;
#endif

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_time_real_series_near_midnight )
{
   DurationTimer timer;
   cout << "Test:: ...test_time_real_series_near_midnight " << flush;
   int the_server_version = TestFixture::server_version() ;
   if (the_server_version == 403 ) {
      cout << " SKIPPING, This test does not work with 403, current server version is " << the_server_version << "\n";
      return;
   }

   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_time_real_series
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <date>
   // family family
   //    task t1
   //       time <start> <finish> <incr>
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      // ECFLOW-130
      // make sure that last job, *runs* and completes after midnight.
      // It should stay complete and not requeue.
      // Test harness, will check suite task->family->suite completes, hence no need to do anything
      boost::posix_time::ptime last_time = boost::posix_time::ptime(date(2010,6,21),time_duration(23,59,0));
      boost::posix_time::ptime first_time = last_time - minutes(TestFixture::job_submission_interval()*2);
      boost::posix_time::ptime clock_start = first_time - minutes(1);

      suite_ptr suite = theDefs.add_suite("test_time_real_series_near_midnight");
      suite->add_variable("SLEEPTIME",boost::lexical_cast<string>(TestFixture::job_submission_interval()*2));

      ClockAttr clockAttr(clock_start,false);
      suite->addClock( clockAttr );

      family_ptr  fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr(
               ecf::TimeSlot(first_time.time_of_day()),
               ecf::TimeSlot(last_time.time_of_day()),
               ecf::TimeSlot(0,TestFixture::job_submission_interval()*2)
      ));
      task->addVerify( VerifyAttr(NState::COMPLETE,2) );      // task should complete 2 times
   }

   // The test harness will create corresponding directory structure and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_time_real_series_near_midnight.def"));

#ifdef DEBUG_ME
   BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "Could not get the defs from server\n" << TestFixture::client().errorMsg());
   defs_ptr defs = TestFixture::client().defs();
   PrintStyle::setStyle(PrintStyle::STATE);
   std::cout << *defs;
#endif

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}
BOOST_AUTO_TEST_SUITE_END()

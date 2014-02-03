//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #30 $ 
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
#include "AssertTimer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( TestSuite )


static void wait_for_cron( int max_time_to_wait, const std::string& path)
{
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   TestFixture::client().set_throw_on_error( false );
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr defs = TestFixture::client().defs();
      if (defs) {
         node_ptr node = defs->findAbsNode(path);
         BOOST_REQUIRE_MESSAGE(node, "Could not find task at path " << path );
         if (node) {
            const std::vector<VerifyAttr>&      verifys  = node->verifys();
            BOOST_REQUIRE_MESSAGE(verifys.size() == 1,"Expected 1 verify");
            if (!verifys.empty()) {
               if ( verifys[0].actual() == verifys[0].expected()) {
                  break;
               }
            }
         }
      }
      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "wait_for_cron: Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n" << *defs);
         break;
      }
      sleep(1);
   }
}


BOOST_AUTO_TEST_CASE( test_cron_time_series )
{
   DurationTimer timer;
   cout << "Test:: ...test_cron_time_series " << flush;
   TestClean clean_at_start_and_end;

   // SLOW SYSTEMS
   // for each time attribute leave GAP of 3 * job submission interval
   // on slow systems submitted->active->complete > TestFixture::job_submission_interval()
   // Also the task duration must be greater than job_submission_interval,  otherwise
   // we will get multiple invocation for the same time step

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_cron_time_series
   //	edit SLEEPTIME 1
   //	edit ECF_INCLUDE $ECF_HOME/includes
   //  clock real <todays date>
   //	family family
   //   	task t1
   //       cron <start> <finish> <incr>
   //  	endfamily
   //endsuite
   Defs theDefs;
   std::string path;
   {
      // Initialise clock with todays date and time, then create a cron attribute
      // with a time series, so that task runs 3 times

      // Note: we don't use:
      //    boost::posix_time::ptime theLocalTime = Calendar::second_clock_time();
      // Because this can fail with:
      //    Test:: ...test_cron_time_series unknown location(0): fatal error in
      //             "test_cron_time_series": std::out_of_range:
      //             TimeSeries::TimeSeries: Invalid time series: Start time(23:58) is greater than end time(00:02)
      // i.e
      // if the test is started at 23:58, then adding the end time of by doing start_time + 5 will fail the check
      boost::posix_time::ptime theLocalTime = boost::posix_time::ptime(date(2010,6,21),time_duration(10,0,0));
      boost::posix_time::ptime time1 =  theLocalTime +  minutes(1);
      boost::posix_time::ptime time2 =  theLocalTime +  minutes(5);

      suite_ptr suite = theDefs.add_suite("test_cron_time_series");
      ClockAttr clockAttr(theLocalTime, false);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      task_ptr task = fam->add_task("t");
      task->addVerify( VerifyAttr(NState::COMPLETE,3) );      // task should complete 3 times

      CronAttr cronAttr;
      cronAttr.addTimeSeries( ecf::TimeSlot(time1.time_of_day()),
                              ecf::TimeSlot(time2.time_of_day()),
                              ecf::TimeSlot(0,2)  );
      task->addCron( cronAttr);

      path = task->absNodePath();
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness(false/*do log file verification*/,false/* dont do standard verification */);
   serverTestHarness.add_default_sleep_time(false); // avoid missing time steps due to submit->active->complete > job submission interval
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_cron_time_series.def"),1 /* timeout ignored*/, false /*waitForTestCompletion*/);

   // crons are *infinite*, just wait for cron to complete 3 times
   wait_for_cron(40,path);

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()


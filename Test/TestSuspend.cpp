//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
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

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ServerTestHarness.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "ClientInvoker.hpp"
#include "PrintStyle.hpp"
#include "AssertTimer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

#if defined(AIX)
static int timeout = 30 ;
#else
static int timeout = 20 ;
#endif


BOOST_AUTO_TEST_SUITE( TestSuite )

static void waitForTimeDependenciesToBeFree(int max_time_to_wait,ClientInvoker& theClient)
{
   // wait for a period of time, while time dependencies fire.
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0,"sync_local failed should return 0\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      vector<Task*> tasks; defs->getAllTasks(tasks);
      size_t taskTimeDepIsFree = 0 ;
      BOOST_FOREACH(Task* task, tasks) {
         size_t attSetFree = 0;
         BOOST_FOREACH( const ecf::TimeAttr& timeAttr, task->timeVec()) {
            if (timeAttr.isFree(task->suite()->calendar())) attSetFree++;
         }
         if ( attSetFree == task->timeVec().size()) taskTimeDepIsFree++;
      }
      if ( taskTimeDepIsFree == tasks.size()) break;

      BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
               "waitForTimeDependenciesToBeFree Test wait " << assertTimer.duration() <<
               " taking longer than time constraint of " << assertTimer.timeConstraint() <<
               " aborting");
      sleep(1);
   }
}

// test:: suspend/shutdown. During suspend the time dependencies should still
// be handled. When the server/node is restarted/resumed the task should
// be submitted straight away.(i.e providing no trigger/complete dependencies)
BOOST_AUTO_TEST_CASE( test_shutdown )
{
   DurationTimer timer;
   cout << "Test:: ...test_shutdown " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a time attribute
      // with todays time + minute Avoid adding directly to TimeSlot
      // i.e if local time is 9:59 and we create a TimeSlot like
      // 		task->addTime( ecf::TimeAttr( ecf::TimeSlot(theTm.tm_hour,theTm.tm_min+3) )  );
      // The the minute will be 62, which is illegal and will not parse
      boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();

      // For each 2 seconds of poll in the server update calendar by 1 minute
      // Note: if we use 1 seconds poll to update calendar by 1 minute, then
      //       we will find that state change happens at job submission interval,
      //       and hence skews time series.  Which can leave state in a queued state,
      //       and hence test never completes
      suite_ptr suite = theDefs.add_suite("test_shutdown");
      ClockAttr clockAttr(theLocalTime);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      for(int i=0; i < taskSize; i++) {
         task_ptr task = fam->add_task("t" + boost::lexical_cast<std::string>(i));

         boost::posix_time::ptime   time1 =  theLocalTime +  minutes(1 + i);
         task->addTime( ecf::TimeAttr( ecf::TimeSlot(time1.time_of_day())  ));

         task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times
      }
   }

   // The test harness will create corresponding directory structure and populate with standard sms files.
   ServerTestHarness serverTestHarness(false/*do verification*/);
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_shutdown.def"),
                         1,
                         false/* don't wait for test to finish */);


   // Shutdown the server. The time dependencies *should* still be handled
   // *** If this test fails, in that we fail to restart() server it will mess up
   // *** any following test.
   ClientInvoker theClient ;
   BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " failed should return 0.\n" << theClient.errorMsg());

   // wait for a period of time, while time dependencies fire.
   (void)waitForTimeDependenciesToBeFree(timeout, theClient );

   // restart server, all jobs should be launched straight away
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " failed should return 0.\n" << theClient.errorMsg());

   // Wait for submitted jobs in restart server to complete
   bool verifyAttrInServer = true;
   defs_ptr serverDefs = serverTestHarness.testWaiter(theClient,theDefs,timeout,verifyAttrInServer);
   BOOST_REQUIRE_MESSAGE(serverDefs.get()," Failed to return server after restartServer");

   //	cout << "Printing Defs \n";
   //	std::cout << *serverDefs.get();

   cout << timer.duration() << " update-calendar-count(" << serverDefs->updateCalendarCount() << ")\n";
}


// test:: suspend/shutdown. During suspend the time dependencies should still
// be handled. When the server/node is restarted/resumed the task should
// be submitted straight away.(i.e providing no trigger/complete dependencies)
BOOST_AUTO_TEST_CASE( test_suspend_node )
{
   DurationTimer timer;
   cout << "Test:: ...test_suspend_node " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation
   Defs theDefs;
   {
      // Initialise clock with todays date and time, then create a time attribute
      // with todays time + minute Avoid adding directly to TimeSlot
      // i.e if local time is 9:59 and we create a TimeSlot like
      // 		task->addTime( ecf::TimeAttr( ecf::TimeSlot(theTm.tm_hour,theTm.tm_min+3) )  );
      // The the minute will be 62, which is illegal and will not parse
      boost::posix_time::ptime theLocalTime =  Calendar::second_clock_time();

      suite_ptr suite = theDefs.add_suite("test_suspend_node");
      ClockAttr clockAttr(theLocalTime);
      suite->addClock( clockAttr );

      task_ptr t1 = suite->add_task( "t1");
      boost::posix_time::ptime time1 =  theLocalTime +  minutes(1);
      t1->addTime( ecf::TimeAttr( ecf::TimeSlot(time1.time_of_day())  ));

      task_ptr t2 = suite->add_task( "t2");
      boost::posix_time::ptime time2 =  theLocalTime +  minutes(2);
      t2->addTime( ecf::TimeAttr( ecf::TimeSlot(time2.time_of_day())  ));

      family_ptr fam = suite->add_family("family");
      for(int i=0; i < 2; i++) {
         task_ptr task = fam->add_task("t" + boost::lexical_cast<std::string>(i));
         task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times

         boost::posix_time::ptime time1 =  theLocalTime +  minutes(1 + i);
         task->addTime( ecf::TimeAttr( ecf::TimeSlot(time1.time_of_day())  ));
      }
   }

   // The test harness will create corresponding directory structure and populate with standard sms files.
   ServerTestHarness serverTestHarness(false/*do verification*/);
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_suspend_node.def"),
                         1,
                         false/* don't wait for test to finish */);

   // SUSPEND the family. The time dependencies *should* still be handled
   ClientInvoker theClient ;
   BOOST_REQUIRE_MESSAGE( theClient.suspend("/test_suspend_node/family") == 0,CtsApi::to_string(CtsApi::suspend("/test_suspend_node/family")) << " failed should return 0.\n" << theClient.errorMsg());

   // wait for a period of time, while time dependencies fire.
   waitForTimeDependenciesToBeFree(timeout, theClient );

   // RESUME the family, all jobs should be launched straight away, since time dependencies should be free
   BOOST_REQUIRE_MESSAGE( theClient.resume("/test_suspend_node/family") == 0,CtsApi::to_string(CtsApi::resume("/test_suspend_node/family")) << " failed should return 0.\n" << theClient.errorMsg());

   // Wait for submitted jobs to complete
   bool verifyAttrInServer = true;
   defs_ptr serverDefs = serverTestHarness.testWaiter(theClient,theDefs,timeout,verifyAttrInServer);
   BOOST_REQUIRE_MESSAGE(serverDefs.get()," Failed to return server after restartServer");

   //	cout << "Printing Defs \n";
   //	std::cout << *serverDefs.get();

   cout << timer.duration() << " update-calendar-count(" << serverDefs->updateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

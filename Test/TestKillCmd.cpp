//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #25 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This is used to INVOKE a SINGLE test. Easier for debugging
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
#include "File.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite  )

// forward declare functions
static bool kill_cmd(bool kill_task);
static bool waitForTaskState(NState::State state, int max_time_to_wait);

// test:: test the kill command. Create a task that runs a long time
// The associated job is then killed. This should leave task in aborted state
BOOST_AUTO_TEST_CASE( test_kill_cmd )
{
   DurationTimer timer;
   cout << "Test:: ...test_kill_cmd " << flush;
   TestClean clean_at_start_and_end;
   BOOST_REQUIRE_MESSAGE(kill_cmd(true)," kill of task '/test_kill_cmd/family/t0' failed");
   cout << timer.duration() << "\n";
}

BOOST_AUTO_TEST_CASE( test_hierarchical_kill_cmd )
{
   DurationTimer timer;
   cout << "Test:: ...test_hierarchical_kill_cmd " << flush;
   TestClean clean_at_start_and_end;
   BOOST_REQUIRE_MESSAGE(kill_cmd(false),"kill of suite '/test_kill_cmd' failed");
   cout << timer.duration() << "\n";
}


static bool kill_cmd(bool kill_task)
{
   /// Create dir location for log file.
   std::string defs_location = ServerTestHarness::testDataDefsLocation("test_kill_cmd.def");
   fs::path new_path = defs_location;
   if (!fs::exists(new_path.parent_path())) {
      File::createMissingDirectories(new_path.parent_path().string());
   }

   Defs theDefs;
   std::string kill_path;
   {
      suite_ptr suite;
      if (kill_task ) suite = theDefs.add_suite( "test_kill_task");
      else            suite = theDefs.add_suite( "test_kill_suite");
      suite->addVariable( Variable("ECF_TRIES","1") ); // do not try again
      family_ptr fam = suite->add_family( "family" );
      task_ptr task = fam->add_task(  "t0");
      task->addMeter( Meter("meter",0,200,100) );      // Make sure it run long enough, to receive kill, on slow systems
      task->addVerify( VerifyAttr(NState::ABORTED,1) );// task should abort 1 times
      if (kill_task) kill_path = task->absNodePath();
      else           kill_path = suite->absNodePath();
   }

   // *******************************************************************
   // Important: The following will *not* work:
   //    theDefs.set_server().add_or_update_variables("ECF_TRIES","1");                     // Override ECF_TRIES so don't try to restart aborted jobs
   //    theDefs.set_server().add_or_update_variables("ECF_KILL_CMD","kill -15 %ECF_RID%"); // Provide a mechanism to kill the running job
   // Since calling the begin command in the server,
   // will update the defs with the server environment, and hence overriding
   // any env variable of the same name, set here. Hence just use addVariable as above.
   // *************************************************************************
   //	cout << theDefs << "\n";

   // cout << "test_kill_cmd Start test\n";
   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,defs_location,1 /*timeout*/,  false/* don't wait for test to finish */);


   // cout << "test_kill_cmd Waiting for task to become active\n";
   TestFixture::client().set_throw_on_error(false);
   (void)waitForTaskState(NState::ACTIVE,10);


   // cout << "test_kill_cmd Now kill the active jobs\n";
   BOOST_REQUIRE_MESSAGE( TestFixture::client().kill(kill_path) == 0,CtsApi::to_string(CtsApi::kill(kill_path)) << " failed should return 0.\n" << TestFixture::client().errorMsg());


   // cout << "test_kill_cmd Wait for the task to be aborted\n";
   return waitForTaskState(NState::ABORTED,20 );
}

static bool waitForTaskState(NState::State state, int max_time_to_wait)
{
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr defs = TestFixture::client().defs();
      vector<Task*> tasks; defs->getAllTasks(tasks);
      BOOST_FOREACH(Task* task, tasks) {
         if (task->state() == state) {
            return true;
         }
      }

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "waitForTaskState " << NState::toString(state) << " Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n" << *defs);
         break;
      }
      sleep(2);
   }
   return false;
}

BOOST_AUTO_TEST_SUITE_END()


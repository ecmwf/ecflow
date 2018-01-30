#define BOOST_TEST_MODULE TEST_SINGLE
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #80 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This is used to INVOKE a SINGLE test.
//               Making it easier for Easier for debugging and development
//============================================================================
#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "TestFixture.hpp"
#include "ServerTestHarness.hpp"

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

BOOST_GLOBAL_FIXTURE( TestFixture );

BOOST_AUTO_TEST_SUITE( TestSingleSuite  )

BOOST_AUTO_TEST_CASE( test_stress )
{
   // at 4000 : limit reached os can not create more than 3946 process
   //   Job creation failed for task /test_stress/family/t3996 could not create child process
   int no_of_tasks_to_run = 200;
   int run_test_for_n_seconds = 180;
   int max_time_for_suspended_suite_to_complete = 60;

   DurationTimer timer;
   std::string test_name = "test_stress";
   std::string test_name_path = "/" + test_name;
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite( test_name );
      family_ptr fam = suite->add_family("family");
      for(int i=0; i < no_of_tasks_to_run ; i++) {
         fam->add_task ( "t" + boost::lexical_cast<std::string>(i) ) ->addRepeat(RepeatDay(1));
      }
   }

   //BOOST_REQUIRE_MESSAGE(TestFixture::job_submission_interval()==60,"Job submission interval not set correctly expected 60 but found " << TestFixture::job_submission_interval());

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation(test_name + ".def"),120/*timeout ignored*/,false/*don't wait for test completion */);


   cout << "Timing test for " << run_test_for_n_seconds << "... seconds  no_of_tasks_to_run " << no_of_tasks_to_run << endl;

   TestFixture::client().set_throw_on_error( false );
   sleep(run_test_for_n_seconds);
   cout << "   suspending suite, to let jobs complete" << endl;
   BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE(TestFixture::client().suspend(test_name_path) == 0, "suspend failed should return 0\n" << TestFixture::client().errorMsg());


   cout << "   Waiting " << max_time_for_suspended_suite_to_complete << " seconds for all jobs to complete" << endl;
   AssertTimer assertTimer(max_time_for_suspended_suite_to_complete,false); // Bomb out after n seconds, fall back if test fail
   while(1) {
      sleep(3);

      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr defs = TestFixture::client().defs();
      if (defs) {
         int no_of_active_tasks = 0;
         std::vector<Task*> vec; defs->getAllTasks(vec);
         for(size_t i = 0; i < vec.size(); i++) {
            if (vec[i]->get_state().first == NState::ACTIVE) no_of_active_tasks++;
         }
         cout << "   " << no_of_active_tasks << " still active" << endl;
         if (no_of_active_tasks == 0) break;
      }

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "   test_stress : Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n");
         break;
      }
   }
   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

// DO NOT delete
// This is used to test large defs. and hence server performance & memory leaks
// Be sure to comment out defstatus suspended in mega.def
//BOOST_AUTO_TEST_CASE( test_mega_def )
//{
//   DurationTimer timer;
//
//   // location to mega.def and left over log file
//   std::string log_file = File::test_data("ANode/parser/test/data/single_defs/mega.def_log","parser");
//   std::string path = File::test_data("ANode/parser/test/data/single_defs/mega.def","parser");
//
//   cout << "Test:: ..." << path << " log file: " << log_file << flush;
//
//   // Remove the log file.
//   boost::filesystem::remove(log_file);
//
//   // parse in file
//   Defs theDefs;
//   std::string errorMsg,warningMsg;
//   BOOST_REQUIRE_MESSAGE(theDefs.restore(path,errorMsg,warningMsg),errorMsg);
//
//   ServerTestHarness serverTestHarness;
//   serverTestHarness.check_task_duration_less_than_server_poll(false); // Add variable CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL
//   serverTestHarness.run(theDefs,path,std::numeric_limits<int>::max()/*timeout*/  );
//
//   cout << timer.duration() << "\n";
//}

// DO NOT delete
// This is used to test with thousands of labels
//BOOST_AUTO_TEST_CASE( test_large_client_labels_calls )
//{
//   DurationTimer timer;
//   cout << "Test:: ...test_large_client_labels_calls "<< flush;
//   TestClean clean_at_start_and_end;
//
//   //# Note: we have to use relative paths, since these tests are relocatable
//   // suite test_large_client_labels_calls
//   //   family family
//   //       task t1
//   //          label name "value"
//   //    endfamily
//   // endsuite
//   Defs theDefs;
//   {
//      suite_ptr suite = theDefs.add_suite("test_large_client_labels_calls");
//      family_ptr fam = suite->add_family("family");
//      task_ptr t1 = fam->add_task("t1");
//      t1->addLabel( Label("name","value"));
//   }
//
//   // Create a custom ecf file for test_large_client_labels_calls/family/t1 to invoke label commands
//   std::string templateEcfFile;
//   templateEcfFile += "%include <head.h>\n";
//   templateEcfFile += "\n";
//   templateEcfFile += "echo do some work\n";
//   templateEcfFile += "i=1\n";
//   templateEcfFile += "while [ \"$i\" -le 10000 ] \n";
//   templateEcfFile += "do\n";
//   templateEcfFile += "  %ECF_CLIENT_EXE_PATH% --label=name \"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\"\n";
//   templateEcfFile += "  i=$((i+1))\n";
//   templateEcfFile += "done\n";
//   templateEcfFile += "\n";
//   templateEcfFile += "%include <tail.h>\n";
//
//   // The test harness will create corresponding directory structure
//   // Override the default ECF file, with our custom ECF_ file
//   std::map<std::string,std::string> taskEcfFileMap;
//   taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs,"t1"),templateEcfFile));
//
//    // Avoid standard verification since we expect to abort many times
//   ServerTestHarness serverTestHarness;
//   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_large_client_labels_calls.def"), taskEcfFileMap,1000000);
//
//   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n" << endl;
//}
//

//// DO NOT delete
/// The following  test verifies that the job generation is largely linear.
/// Job submission interval was 3 seconds. In release mode.
///
/// Test duration is highly dependent on the OS/process handling.
/// No of tasks     Job generation    Peak request/sec in server
///   10                  -
///   100                 -
///   500                 -                     364
///   1000               4                      450
///   1500               6                      550
///   2000               9                      880
///   3000               15                     800
///   4000               24                     923
///   5000               29                     800
///   6000               37                     900
//void time_for_tasks(int tasks) {
//
//   std::string test_name = "test_stress_" + boost::lexical_cast<std::string>(tasks);
//   cout << "Test:: ..." << test_name  << flush;
//
//   Defs theDefs;
//   {
//      suite_ptr suite = theDefs.add_suite( test_name );
//      family_ptr fam = suite->add_family("fam" );
//      for(int i = 0; i < tasks; i++) {
//         fam->add_task( "t" +   boost::lexical_cast<std::string>(i));
//      }
//      //      suite->addRepeat( RepeatDate("YMD",19000101,99991201,1) );
//   }
//
//   ServerTestHarness serverTestHarness;
//   serverTestHarness.check_task_duration_less_than_server_poll(false); // Add variable CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL
//   DurationTimer timer;
//   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation(test_name + ".def"), std::numeric_limits<int>::max()/*timeout*/  );
//   cout << "  " << timer.duration() << "s\n";
//}
//
//// Use to stress test server. i.e continually fire client requests.
//BOOST_AUTO_TEST_CASE( test_stress )
//{
//   time_for_tasks(10);
//   time_for_tasks(100);
//   time_for_tasks(500);
//   time_for_tasks(1000);
//   time_for_tasks(1500);
//   time_for_tasks(2000);
//   time_for_tasks(3000);
//   time_for_tasks(4000);
//   time_for_tasks(5000);
//}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
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
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

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
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )

static void create_defs(Defs& theDefs, const std::string& suite_name)
{
   // suite test_wait_cmd
   //   family family0
   //       task wait
   //   family family1
   //       task a
   //       task b
   //   endfamily
   //   family family2
   //          task aa
   //          task bb
   //    endfamily
   // endsuite
   suite_ptr suite = theDefs.add_suite(suite_name);
   family_ptr fam0 = suite->add_family("family0");
   task_ptr wait = fam0->add_task("wait");
   wait->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 1 times

   family_ptr fam1 = suite->add_family("family1");
   fam1->add_task( "a" );
   fam1->add_task( "b" );

   family_ptr fam2 = suite->add_family("family2");
   fam2->add_task( "aa" );
   fam2->add_task( "bb" );
}

static bool wait_for_state(
         std::vector< std::pair<std::string,NState::State> >& path_state_vec,
         int max_time_to_wait )
{
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr defs = TestFixture::client().defs();
      bool all_states_ok = true;
      for(size_t i =0; i < path_state_vec.size(); ++i) {
         node_ptr node = defs->findAbsNode( path_state_vec[i].first );
         BOOST_REQUIRE_MESSAGE(node,"Could not find path '" << path_state_vec[i].first << "' in the defs\n" << *defs);
         if (node->state() != path_state_vec[i].second) {
            all_states_ok = false;
            break;
         }
      }
      if (all_states_ok) return true;

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "wait_for_state: Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n" << *defs);
         break;
      }
      sleep(1);
   }
   return false;
}

// Test the wait command. The Wait command is a child command.
// The wait command is provided an expression. This expression is evaluated
// in the server. If the evaluate returns false then the client should
// blocks until the evaluation is true.
// In this case the job associated with task 'wait' should block until the expression evaluates
// to true, which should be after the completion of all other tasks
BOOST_AUTO_TEST_CASE( test_wait_cmd )
{
   DurationTimer timer;
   cout << "Test:: ...test_wait_cmd "<< flush;
   TestClean clean_at_start_and_end;

   Defs theDefs;
   create_defs(theDefs,"test_wait_cmd");

   // Create a custom sms file for test_wait_cmd/family0/wait to invoke the child wait command
   // with an expression that forces it to block, until all other tasks complete
   std::string templateEcfFileForWait;
   templateEcfFileForWait += "%include <head.h>\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "echo do some work\n";
   templateEcfFileForWait += "%ECF_CLIENT_EXE_PATH% --wait=\"../family1/a eq complete and ../family1/b eq complete and ../family2/aa eq complete and ../family2/bb eq complete\"\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "%include <tail.h>\n";

   // The test harness will create corresponding directory structure
   // Override the default ECF_ file, with our custom ECF_ file
   std::map<std::string,std::string> taskEcfFileMap;
   taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs,"wait"),templateEcfFileForWait));

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_wait_cmd.def"), taskEcfFileMap);

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_wait_cmd_parse_fail )
{
   DurationTimer timer;
   cout << "Test:: ...test_wait_cmd_parse_fail "<< flush;

   // This time we add a wait expression that
   // should fail to parse, and we should return an error
   // The error should captured by the trap in .ecf script, which will
   // then abort the task
   Defs theDefs;
   create_defs(theDefs,"test_wait_cmd_parse_fail");

   // Create a custom ecf file for test_wait_cmd/family0/wait to invoke the child wait command
   std::string templateEcfFileForWait;
   templateEcfFileForWait += "%include <head.h>\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "echo do some work\n";
   templateEcfFileForWait += "%ECF_CLIENT_EXE_PATH% --wait=\"(((((((((../family1/a eq complete and ../family1/b eq complete and ../family2/aa eq complete and ../family2/bb eq complete\"\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "%include <tail.h>\n";

   std::map<std::string,std::string> taskEcfFileMap;
   taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs,"wait"),templateEcfFileForWait));

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_wait_cmd_parse_fail.def"), taskEcfFileMap,1 /*timeout*/,  false/* don't wait for test to finish */);

   // wait for family1 and family2 to complete
   std::vector< std::pair<std::string,NState::State> > path_state_vec;
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_parse_fail/family1"), NState::COMPLETE) );
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_parse_fail/family2"), NState::COMPLETE) );
   wait_for_state(path_state_vec,10);

   // wait for 'wait' task to abort
   path_state_vec.clear();
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_parse_fail/family0/wait"), NState::ABORTED) );
   wait_for_state(path_state_vec,10);

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_wait_cmd_non_existant_paths )
{
   DurationTimer timer;
   cout << "Test:: ...test_wait_cmd_non_existant_paths "<< flush;

   // This time we add a wait expression that should fail
   // because the paths referenced in the expression don't exist
   Defs theDefs;
   create_defs(theDefs,"test_wait_cmd_non_existant_paths");

   // Create a custom ecf file for test_wait_cmd/family0/wait to invoke the child wait command
   // NOTE: ../family1/FRED does not exist
   std::string templateEcfFileForWait;
   templateEcfFileForWait += "%include <head.h>\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "echo do some work\n";
   templateEcfFileForWait += "%ECF_CLIENT_EXE_PATH% --wait=\"../family1/FRED eq complete and ../family1/b eq complete and ../family2/aa eq complete and ../family2/bb eq complete\"\n";
   templateEcfFileForWait += "\n";
   templateEcfFileForWait += "%include <tail.h>\n";

   std::map<std::string,std::string> taskEcfFileMap;
   taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs,"wait"),templateEcfFileForWait));

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_wait_cmd_non_existant_paths.def"), taskEcfFileMap,1 /*timeout*/,  false/* don't wait for test to finish */);

   // wait for family1 and family2 to complete
   std::vector< std::pair<std::string,NState::State> > path_state_vec;
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_non_existant_paths/family1"), NState::COMPLETE) );
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_non_existant_paths/family2"), NState::COMPLETE) );
   wait_for_state(path_state_vec,10);

   // wait for 'wait' task to abort
   path_state_vec.clear();
   path_state_vec.push_back( std::make_pair( std::string("/test_wait_cmd_non_existant_paths/family0/wait"), NState::ABORTED) );
   wait_for_state(path_state_vec,10);

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

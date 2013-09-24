//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
//    This test will TEST:
//       o Alias creation and running
//       o Alias ordering
//       o Alias deletion
//    Will indirectly test the Alias memento's
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
#include "ClientInvoker.hpp"
#include "PrintStyle.hpp"
#include "ClientToServerCmd.hpp"
#include "DefsStructureParser.hpp"
#include "AssertTimer.hpp"
#include "TestVerification.hpp"
#include "Str.hpp"
#include "NOrder.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSingleSuite  )

template <typename T>
static std::vector<std::string> toStrVec(const std::vector<T>& vec)
{
   std::vector<std::string> retVec; retVec.reserve(vec.size());
   BOOST_FOREACH(T s, vec) { retVec.push_back(s->name()); }
   return retVec;
}

std::string toString(const std::vector<std::string>& c)
{
   std::stringstream ss;
   std::copy (c.begin(), c.end(), std::ostream_iterator <std::string> (ss, ", "));
   return ss.str();
}

void wait_for_alias_to_complete(ClientInvoker& theClient, const std::string& alias_path)
{
   AssertTimer assertTimer(10,false); // Bomb out after 10 seconds, fall back if test fail
   while(1) {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      node_ptr alias = defs->findAbsNode(alias_path);
      BOOST_REQUIRE_MESSAGE(alias.get(), "Could not locate created alias at path " << alias_path << "\n" << theClient.errorMsg());
      if (alias->state() == NState::COMPLETE) break;
      sleep(2);
   }
}

BOOST_AUTO_TEST_CASE( test_alias )
{
   DurationTimer timer;
   cout << "Test:: ...test_alias "<< flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in ecf includes
   //                     Allows test to run without requiring installation
   Defs theDefs;
   task_ptr task_a;
   {
      suite_ptr suite = theDefs.add_suite( "test_alias" ) ;
      suite->add_variable("SLEEPTIME","0");
      task_a = suite->add_task("task_a");
      task_a->addMeter( Meter("meter",0,20,20) );
      task_a->addEvent( Event(1,"event") );
      task_a->addLabel( Label("task_a_label","Label1") );
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness( false /*doLogFileVerification*/, true/* standardVerification*/);
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_alias.def"));

   // After the task has completed create a Alias
   ClientInvoker theClient ;
   theClient.set_throw_on_error(false);

   // Get the ecf script from the current task and use it to create alias
   BOOST_REQUIRE_MESSAGE( theClient.file(task_a->absNodePath(),CFileCmd::toString(CFileCmd::ECF),"10000") == 0, "Expected to retreive script\n" << theClient.errorMsg());
   std::string script = theClient.get_string();
   BOOST_CHECK_MESSAGE( !script.empty(),"script for task " << task_a->absNodePath() << " is empty" );


   // TEST Alias CREATION and running =============================================================================
   // Split the file into line
   std::vector<std::string> script_lines;
   Str::split(script,script_lines,"\n");

   NameValueVec used_variables;
   int result = theClient.edit_script_submit(task_a->absNodePath(),used_variables,script_lines,true/*create alias*/,true/*run alias*/);
   BOOST_REQUIRE_MESSAGE(result == 0, "Expected alias creation and run to succeed\n" << theClient.errorMsg());
   std::string alias0_path = task_a->absNodePath() + "/alias0";

   // Wait for alias to complete.
   wait_for_alias_to_complete(theClient,alias0_path);

//#ifdef DEBUG
//   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
//   defs_ptr defs = theClient.defs();
//   PrintStyle::setStyle(PrintStyle::STATE);
//   std::cout << *defs;
//#endif


   // TEST ORDERING:: Create another alias, but don't run it. =============================================================================
   result = theClient.edit_script_submit(task_a->absNodePath(),used_variables,script_lines,true/*create alias*/,false/*run alias*/);
   BOOST_REQUIRE_MESSAGE(result == 0, "Expected alias creation succeed\n" << theClient.errorMsg());
   {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      std::vector<alias_ptr> aliases;
      defs->get_all_aliases(aliases);
      BOOST_REQUIRE_MESSAGE(aliases.size() == 2, "Expected 2 aliases\n" << theClient.errorMsg());
   }

   BOOST_REQUIRE_MESSAGE(theClient.order(alias0_path, NOrder::toString(NOrder::DOWN)) == 0, "Expected order NOrder::DOWN to succeed\n" << theClient.errorMsg());
   {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      Task* task = defs->findAbsNode(task_a->absNodePath())->isTask();
      BOOST_REQUIRE_MESSAGE( task,"expected to find Task");

      std::vector<std::string> expected;
      expected.push_back("alias1");
      expected.push_back("alias0");
      BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::DOWN expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
   }

   BOOST_REQUIRE_MESSAGE(theClient.order(alias0_path, NOrder::toString(NOrder::UP)) == 0, "Expected order NOrder::UP to succeed\n" << theClient.errorMsg());
   {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      Task* task = defs->findAbsNode(task_a->absNodePath())->isTask();
      BOOST_REQUIRE_MESSAGE( task,"expected to find Task");

      std::vector<std::string> expected;
      expected.push_back("alias0");
      expected.push_back("alias1");
      BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::UP expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
   }

   BOOST_REQUIRE_MESSAGE(theClient.order(alias0_path, NOrder::toString(NOrder::ORDER)) == 0, "Expected order NOrder::ORDER to succeed\n" << theClient.errorMsg());
   {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      Task* task = defs->findAbsNode(task_a->absNodePath())->isTask();
      BOOST_REQUIRE_MESSAGE( task,"expected to find Task");

      std::vector<std::string> expected;
      expected.push_back("alias1");
      expected.push_back("alias0");
      BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ORDER expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
   }

   BOOST_REQUIRE_MESSAGE(theClient.order(alias0_path, NOrder::toString(NOrder::ALPHA)) == 0, "Expected order NOrder::ALPHA to succeed\n" << theClient.errorMsg());
   {
      BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
      defs_ptr defs = theClient.defs();
      Task* task = defs->findAbsNode(task_a->absNodePath())->isTask();
      BOOST_REQUIRE_MESSAGE( task,"expected to find Task");

      std::vector<std::string> expected;
      expected.push_back("alias0");
      expected.push_back("alias1");
      BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ALPHA expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
   }


   // TEST Alias DELETION =============================================================================
   std::string alias1_path = task_a->absNodePath() + "/alias1";
   BOOST_REQUIRE_MESSAGE(theClient.delete_node(alias0_path) == 0, "delete alias0 failed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.delete_node(alias1_path) == 0, "delete alias1 failed\n" << theClient.errorMsg());

   // Get the defs from the server
   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Could not get the defs from server\n" << theClient.errorMsg());
   defs_ptr defs = theClient.defs();
   std::vector<alias_ptr> aliases;
   defs->get_all_aliases(aliases);
   BOOST_REQUIRE_MESSAGE(aliases.empty(), "Alias deletion falied\n" << theClient.errorMsg());

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

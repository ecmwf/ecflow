#define BOOST_TEST_MODULE TestClient
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #54 $ 
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
#include <fstream>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "PrintStyle.hpp"
#include "WhiteListFile.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"
#include "System.hpp"             // kill singleton for valgrind

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// Note: To test HPUX->Linux, invoke serve on (Linux/HPUX) and the client cmds on other system
//       On the client side set ECF_NODE to machine name. To allow further testing if ECF_NODE
//       is specified then *don't* shutdown the server
// ************************************************************************************

BOOST_AUTO_TEST_CASE( test_client_lifecyle )
{
   // *******************************************************************************************
   // This test will *ONLY* work when testing with new server invocation, since it relies
   // on disabling job generation. Hence ignore test if ECF_NODE has been defined
   // *******************************************************************************************
   std::string host = ClientEnvironment::hostSpecified();
   if (!host.empty()) {
      // Server allready started, since we cant disable job generation ignore this test
      std::cout << "Client:: ...test_client_lifecycle, ignoring test when ECF_NODE specified..." << endl;
      return;
   }

   // This will remove check pt and backup file before server start, to avoid the server from loading previous test data
   // ** NOTE: We disable job generation in the server **/
   InvokeServer invokeServer("Client:: ...test_client_lifecycle",SCPort::next(),true /*disable job generation in server*/);

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0 server not started, or connection refused\n" << theClient.errorMsg());

   // load the defs into the server
   {
      std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "Load defs failed \n" << theClient.errorMsg());
   }
   {
      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"Server returned a NULL defs");
      BOOST_REQUIRE_MESSAGE( serverDefs->suiteVec().size() >= 1,"  no suite ?");
   }


   // Now go through and simulate client request to change Node tree state.
   // This is **highly** dependent on lifecycle.txt
   //	suite suite1
   //	  family family1
   //	   	task a
   //	        event 1 myEvent
   //	        meter myMeter 0 100
   //	   	task b
   //	   		trigger a == complete
   //	   endfamily
   //	   family family2
   //	   		task aa
   //	   			trigger ../family1/a:myMeter >= 20 and ../family1/a:myEvent
   //	   		task bb
   //	   			trigger ../family1/a:myMeter >= 50 || ../family1/a:myEvent
   //	    endfamily
   //	endsuite

   string suite1_family1_a = "suite1/family1/a";
   string suite1_family1_b = "suite1/family1/b";
   string suite1_family2_aa = "suite1/family2/aa";
   string suite1_family2_bb = "suite1/family2/bb";

   {
      // Begin will set all states to queued and then start job submission placing
      // any submiited jobs into the active state. Note server setup has disabled job generation
      BOOST_REQUIRE_MESSAGE(theClient.begin("suite1") == 0,CtsApi::begin("suite1") << " failed should return 0\n" << theClient.errorMsg());

      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      node_ptr node = serverDefs->findAbsNode(suite1_family1_a);
      BOOST_REQUIRE_MESSAGE( node->state() == NState::ACTIVE, "Node expected NState::ACTIVE, but found to be " << NState::toString(node->state()));
   }

   //**********************************************************************
   // Create a request to set the event on Node suite1/family1/a
   // This should place suite1_family2_bb immediately into submitted state
   // Since we at least one node in submitted, suite should be in SUBMITTED state
   {
      theClient.taskPath(suite1_family1_a);
      theClient.set_jobs_password(Submittable::DUMMY_JOBS_PASSWORD());
      BOOST_REQUIRE_MESSAGE(theClient.eventTask("myEvent") == 0,TaskApi::event("myEvent") << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      // PrintStyle style(PrintStyle::STATE);
      // 		cerr << *serverDefs.get() << "\n";
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"get command failed to get node tree from server");

      node_ptr node = serverDefs->findAbsNode(suite1_family1_a);
      BOOST_REQUIRE_MESSAGE( node->state() == NState::ACTIVE, "Node expected NState::ACTIVE, but found to be " << NState::toString(node->state()));
      node_ptr nodeb = serverDefs->findAbsNode(suite1_family2_bb);
      BOOST_REQUIRE_MESSAGE( nodeb->state() == NState::ACTIVE, "Node expected NState::ACTIVE, but found to be " << NState::toString(nodeb->state()));
   }


   //**********************************************************************
   // Create a request to set the Meter on Node suite1/family1/a
   // This should force suite1_family2_aa immediately into submitted state
   // This should force suite1_family2_bb immediately into submitted state
   {
      theClient.taskPath(suite1_family1_a);
      {
         char* argv[] = { const_cast<char*>("ClientInvoker"),
                  const_cast<char*>("--meter=myMeter"),
                  const_cast<char*>("100")
         };
         BOOST_REQUIRE_MESSAGE(theClient.invoke(3,argv) == 0," should return 0\n" << theClient.errorMsg());
         BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());
      }

      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"get command failed to get node tree from server");

      node_ptr nodeaa = serverDefs->findAbsNode(suite1_family2_aa);
      BOOST_REQUIRE_MESSAGE( nodeaa->state() == NState::ACTIVE, "Node expected NState::ACTIVE, but found to be " << NState::toString(nodeaa->state()));

      node_ptr nodebb = serverDefs->findAbsNode(suite1_family2_bb);
      BOOST_REQUIRE_MESSAGE( nodebb->state() == NState::ACTIVE, "Node expected NState::ACTIVE, but found to be " << NState::toString(nodebb->state()));
   }

   //**********************************************************************
   // Create a request to complete task suite1/family1/a
   // This should force suite1_family1_b to complete
   {
      theClient.taskPath(suite1_family1_a);
      BOOST_REQUIRE_MESSAGE(theClient.completeTask() == 0,TaskApi::complete() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());

      // Resolve dependencies
      // We could either wait 60 second or
      // added a custom command that will force dependency evaluation
      BOOST_REQUIRE_MESSAGE( theClient.forceDependencyEval() == 0,"--force-dep-eval failed should return 0\n" << theClient.errorMsg());

      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"get command failed to get node tree from server");

      node_ptr node = serverDefs->findAbsNode(suite1_family1_b);
      BOOST_REQUIRE_MESSAGE( node->state() == NState::ACTIVE,  "Expected NState::ACTIVE, but found " << NState::toString(node->state()));
   }

   //********************************************************************************
   // Complete the remaining tasks. Should really call init first, but what the eck.
   {
      theClient.taskPath(suite1_family1_b);
      BOOST_REQUIRE_MESSAGE(theClient.completeTask() == 0,TaskApi::complete() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());

      theClient.taskPath(suite1_family2_aa);
      BOOST_REQUIRE_MESSAGE(theClient.completeTask() == 0,TaskApi::complete() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());

      theClient.taskPath(suite1_family2_bb);
      BOOST_REQUIRE_MESSAGE(theClient.completeTask() == 0,TaskApi::complete() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.forceDependencyEval() == 0,CtsApi::forceDependencyEval() << " failed should return 0\n" << theClient.errorMsg());
   }

   {
      // Get the node tree back from the server, and check its node state
      // All node state should be complete. + check meter/event value was set properly
      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0\n" << theClient.errorMsg());

      defs_ptr serverDefs =  theClient.defs();
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"get command failed to get node tree from server");

      std::string metername = "myMeter";
      int meterValue = 100;
      node_ptr node = serverDefs->findAbsNode(suite1_family1_a);
      const Meter& theMeter = node->findMeter(metername);
      BOOST_REQUIRE_MESSAGE( !theMeter.empty(), "Could not find the meter");
      BOOST_REQUIRE_MESSAGE( theMeter.value() == meterValue , "Expected meter value " << meterValue << " but found " << theMeter.value());
      {
         std::string errorMsg; BOOST_REQUIRE_MESSAGE( serverDefs->checkInvariants(errorMsg), errorMsg);
      }

      std::string eventname = "myEvent";
      const Event& theEvent = node->findEventByNameOrNumber(eventname);
      BOOST_REQUIRE_MESSAGE( !theEvent.empty(), "Could not find the event myEvent");
      BOOST_REQUIRE_MESSAGE( theEvent.value(), "The event was not set");

      const std::vector<suite_ptr>& suiteVec = serverDefs->suiteVec();
      suite_ptr suite = suiteVec.back();
      BOOST_REQUIRE_MESSAGE( suite->state() == NState::COMPLETE,
               "Suite expected NState::COMPLETE, but found to be " << NState::toString(suite->state()));
   }

   {
      // Check that a log file was created, by asking the server for it.
      BOOST_REQUIRE_MESSAGE( theClient.getLog() == 0, "get log failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(!theClient.get_string().empty(),"The log file returned from the server is empty!!");

      // Clear the log
      BOOST_REQUIRE_MESSAGE(theClient.flushLog() == 0,CtsApi::flushLog() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.clearLog() == 0,CtsApi::clearLog() << " failed should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   }

   /// Destroy singleton's to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()


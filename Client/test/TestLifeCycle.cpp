//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #50 $ 
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

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite)

BOOST_AUTO_TEST_CASE( test_node_tree_lifecycle )
{
	cout << "Client:: ...test_node_tree_lifecycle" << endl;

   std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");

	Defs defs;
	std::string errorMsg,warningMsg;
	bool parse = defs.restore(path,errorMsg,warningMsg);
	if (!parse)  std::cerr << errorMsg;
	BOOST_CHECK(parse);

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

	// get the suite, before we do anything initial state should be UNKNOWN
	const std::vector<suite_ptr>& suiteVec = defs.suiteVec();
	suite_ptr suite = suiteVec.back();
	BOOST_CHECK_MESSAGE( suite->state() == NState::UNKNOWN," Initial suite state should be NState::UNKNOWN");

	string suite1_family1_a = "suite1/family1/a";
	string suite1_family1_b = "suite1/family1/b";
	string suite1_family2_aa = "suite1/family2/aa";
	string suite1_family2_bb = "suite1/family2/bb";


	// Pick the suite that is allowed to resolve dependencies
	// The suite must be loaded and be in state UNKNOWN or COMPLETE
 	TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("suite1")));


	// ***********************************************************************
	// Create a request to initialise Node: suite1/family1/a
	{
		TestHelper::invokeRequest(&defs,Cmd_ptr( new InitCmd(suite1_family1_a,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)),false);

		node_ptr node = defs.findAbsNode(suite1_family1_a);
		BOOST_CHECK_MESSAGE( node, "Could not find node");
		BOOST_CHECK_MESSAGE( node->state() == NState::ACTIVE, "Init request should place node in NState::ACTIVE");
		BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE, "Suite should be in NState::ACTIVE after init cmd");
	 	std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);
	}

	//**********************************************************************
	// Create a request to set the event on Node suite1/family1/a
	// This should force suite1_family2_bb immediately into submitted/active state
	{
 		std::string eventname = "myEvent";
 		TestHelper::invokeRequest(&defs,Cmd_ptr( new EventCmd(suite1_family1_a,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,eventname)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)));
 		node_ptr node = defs.findAbsNode(suite1_family1_a);

		const Event& theEvent = node->findEventByNameOrNumber(eventname);
		BOOST_CHECK_MESSAGE( !theEvent.empty(), "Could not find the event myEvent");
		BOOST_CHECK_MESSAGE( theEvent.value(), "The event was not set");
	 	std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);

 		// cerr << "Defs " << defs << "\n";
 		BOOST_CHECK_MESSAGE( node->state() == NState::ACTIVE, "State should be NState::ACTIVE but found to be " << NState::toString(node->state()));
		BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE, "Suite should be in NState::ACTIVE but found to be " << NState::toString(suite->state()));

		node_ptr nodebb = defs.findAbsNode(suite1_family2_bb);
 		BOOST_CHECK_MESSAGE( nodebb->state() == NState::ACTIVE, "State should be NState::ACTIVE but found to be " << NState::toString(nodebb->state()));
	}

	//*******************************************************************************
	// Resolve dependencies. After previous event suite1_family2_bb should have
	//                       been put into ACTIVE state.
	//                       ** Tests trigger/AST OR functionality **
	//                       Since we at least one node in active, suite should
	//                       be in avtive state
	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
		BOOST_CHECK_MESSAGE(jobs.generate(jobsParam),jobsParam.getErrorMsg());
		BOOST_FOREACH(Submittable* t, jobsParam.submitted() ) {
			BOOST_CHECK_MESSAGE( t->state() == NState::SUBMITTED, "jobSubmission should change Node state");
 		}

		{
			node_ptr node = defs.findAbsNode(suite1_family1_a);
			BOOST_CHECK_MESSAGE( node->state() == NState::ACTIVE, "resolve dependencies should change Node state here");
		}
		node_ptr node = defs.findAbsNode(suite1_family2_bb);
		BOOST_CHECK_MESSAGE( node, "Could Not find Node " << suite1_family2_bb );
		BOOST_CHECK_MESSAGE( node->state() == NState::ACTIVE, "resolve dependencies should change Node state");
		BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE, "Suite expected NState::ACTIVE, but found to be " << NState::toString(suite->state()));
	 	std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);
	}

	//**********************************************************************
	// Create a request to set the Meter on Node suite1/family1/a
	// This should immediately change suite1_family2_aa into submitted state
	{
 		std::string metername = "myMeter";
		int meterValue = 100;
		TestHelper::invokeRequest(&defs, Cmd_ptr( new MeterCmd(suite1_family1_a,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,metername,meterValue)));
      TestHelper::invokeRequest(&defs, Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)));

		node_ptr node = defs.findAbsNode(suite1_family1_a);
 		BOOST_CHECK_MESSAGE( node->state() == NState::ACTIVE,
 		                     "Expected Node '" << node->absNodePath() << "' to be NState::ACTIVE, but found " << NState::toString(node->state()) << "\n");
		BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE,
		                     "Suite expected NState::ACTIVE, but found to be " << NState::toString(suite->state()));

		const Meter& theMeter = node->findMeter(metername);
		BOOST_CHECK_MESSAGE( !theMeter.empty(), "Could not find the meter");
		BOOST_CHECK_MESSAGE( theMeter.value() == meterValue , "Meter value not set");
		std::string errorMsg; BOOST_REQUIRE_MESSAGE( defs.checkInvariants(errorMsg), errorMsg);

		node_ptr nodeaa = defs.findAbsNode(suite1_family2_aa);
		BOOST_CHECK_MESSAGE( nodeaa, "Could Not find Node " << suite1_family2_aa );
		BOOST_CHECK_MESSAGE( nodeaa->state() == NState::ACTIVE, "resolve dependencies should change Node state");
	}

	//**********************************************************************
	// Create a request to complete task suite1/family1/a
	// A is complete, which means evaluation dependencies should force:
	//      suite1/family1/b   :->to be submitted.
 	// since the complete command does an immediate job submission afterwards
	{
		TestHelper::invokeRequest(&defs,Cmd_ptr( new CompleteCmd(suite1_family1_a,Submittable::DUMMY_JOBS_PASSWORD())));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)));

		node_ptr node = defs.findAbsNode(suite1_family1_a);
		BOOST_CHECK_MESSAGE( node, "Could not find node");
		BOOST_CHECK_MESSAGE( node->state() == NState::COMPLETE, "Complete request should place node in NState::COMPLETE");
		BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE, "Expected NState::ACTIVE, but found to be " << NState::toString(suite->state()));


		node_ptr nodeb = defs.findAbsNode(suite1_family1_b);
		BOOST_CHECK_MESSAGE( nodeb, "Could not find node");
		BOOST_CHECK_MESSAGE( nodeb->state() == NState::ACTIVE, "Expected NState::ACTIVE, but found to be " << NState::toString(nodeb->state()));

	 	std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);
  	}

	//*******************************************************************************
	// Job submission, should not send any jobs for submission.
 	//         This will evaluate dependencies( ie day,date, trigger ast)
 	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
		BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 task to submit but found " << jobsParam.submitted().size());
 	}

	//********************************************************************************
	// Complete the remaining tasks
	{
		TestHelper::invokeRequest(&defs , Cmd_ptr( new CompleteCmd("suite1/family1/b",Submittable::DUMMY_JOBS_PASSWORD())));
		TestHelper::invokeRequest(&defs , Cmd_ptr( new CompleteCmd(suite1_family2_aa,Submittable::DUMMY_JOBS_PASSWORD())));
		TestHelper::invokeRequest(&defs , Cmd_ptr( new CompleteCmd(suite1_family2_bb,Submittable::DUMMY_JOBS_PASSWORD())));

		BOOST_CHECK_MESSAGE( suite->state() == NState::COMPLETE, "Suite should be in NState::COMPLETE state");
	 	std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);
 	}
   cout << "Client:: ...-END\n";
}

BOOST_AUTO_TEST_SUITE_END()


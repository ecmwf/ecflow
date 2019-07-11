#define BOOST_TEST_MODULE TestBase
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #37 $ 
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

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "ExprAst.hpp"
#include "TestHelper.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_resolve_dependencies )
{
	cout << "Base:: ...test_resolve_dependencies\n";

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create the defs file
	//	suite suite
	//	   family f
	//	   		task t
	//	   			meter step 0 240 120
	//	   		task tt
	//	   			complete t:step ge 120
	//	   			trigger t == complete
	//	   endfamily
	//	endsuite
	Defs defs;
	std::string metername = "step";
	std::string suitename = "suite";
	std::string familyname = "f";
	{
		suite_ptr suite = defs.add_suite( suitename );
      family_ptr fam = suite->add_family( familyname );

		task_ptr task = fam->add_task( "t" );
		task->addMeter( Meter(metername,0,240,120) );

      task_ptr task_tt = fam->add_task( "tt" );
		task_tt->add_complete(  "t:step ge 120");
		task_tt->add_trigger(   "t == complete" );

		std::string errorMsg;
		BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg), errorMsg);
	}


	// Ensure initial state is unknown
	string suite_f_t = "/suite/f/t";
	string suite_f_tt = "/suite/f/tt";
	node_ptr node_t = defs.findAbsNode(suite_f_t);
	node_ptr node_tt = defs.findAbsNode(suite_f_tt);
	suite_ptr suite = defs.findSuite(suitename);
	family_ptr fam = suite->findFamily(familyname);
 	BOOST_CHECK_MESSAGE( suite->state() == NState::UNKNOWN, "expected state NState::UNKNOWN, but found to be " << NState::toString(suite->state()));
	BOOST_CHECK_MESSAGE( node_t->state() == NState::UNKNOWN, "expected state NState::UNKNOWN, but found to be " << NState::toString(node_t->state()));
	BOOST_CHECK_MESSAGE( node_tt->state() == NState::UNKNOWN, "expected state NState::UNKNOWN, but found to be " << NState::toString(node_tt->state()));
 	BOOST_CHECK_MESSAGE( fam->state() == NState::UNKNOWN, "expected state NState::UNKNOWN, but found to be " << NState::toString(fam->state()));


	// ***********************************************************************
	// Create a request to begin suite
	// make sure chosen suite can begin to resolve dependencies.
	// beginning the suite will :
 	//     1/ set all children to the QUEUED state
  	//     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
  	{
		TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd(suitename)));
 	 	BOOST_CHECK_MESSAGE( suite->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(suite->state()));
	 	BOOST_CHECK_MESSAGE( fam->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(fam->state()));
		BOOST_CHECK_MESSAGE( node_t->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(node_t->state()));
		BOOST_CHECK_MESSAGE( node_tt->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(node_tt->state()));
  	}

	//*******************************************************************************
	// Resolve dependencies.
 	//	   		task t
	//	   			meter step 0 240 120       EXPECTED to be sumbitted
 	//
	//	   		task tt
	//	   			complete t:step ge 120     Expected to HOLD, since we ain't done nothing yet
	//	   			trigger t == complete
	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
 		BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 task to submit but found " << jobsParam.submitted().size());
		BOOST_CHECK_MESSAGE( node_t->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(node_t->state()));
 		BOOST_CHECK_MESSAGE( node_tt->state() == NState::QUEUED,"expected state NState::QUEUED, but found to be " << NState::toString(node_tt->state()));
	}

	//**********************************************************************
	// Create a request to set the Meter node t. This should force node tt
	// to complete immediately
	{
		int meterValue = 120;
		TestHelper::invokeRequest(&defs, Cmd_ptr( new MeterCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,metername,meterValue)));
      TestHelper::invokeRequest(&defs, Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)));
		const Meter& theMeter = node_t->findMeter(metername);
		BOOST_CHECK_MESSAGE( !theMeter.empty(), "Could not find the meter ");
		BOOST_CHECK_MESSAGE( theMeter.value() == meterValue , "Meter value not set");

		BOOST_CHECK_MESSAGE( node_t->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(node_t->state()));
 		BOOST_CHECK_MESSAGE( node_tt->state() == NState::COMPLETE,"expected state NState::COMPLETE, but found to be " << NState::toString(node_tt->state()));

	}

	{
		std::string errorMsg;
		BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg), errorMsg);
	}

	/// Destroy System singleton to avoid valgrind from complaining
	System::destroy();
}

BOOST_AUTO_TEST_CASE( test_trigger_after_delete )
{
  	cout << "Base:: ...test_trigger_after_delete\n";
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Create the defs file
	//	suite suite1    # the limit we want delete
	//     family f
	//     		task t0
  	//          	defstatus complete
	//     		task t1
  	//             event event
  	//          task t2
  	//             meter meter 0 100 100
  	//          task t3
  	//             edit user_var 1
 	//          task t4
  	//             repeat integer repeat_var 0 10 2
	//     endfamily
	//	endsuite
	//	suite suite2
 	//	   family f
 	//	   		task t0
  	//				trigger /suite1/f/t0 == complete
	//	   		task t1
  	//				trigger /suite1/f/t1:event == set
	//	   		task t2
  	//				trigger /suite1/f/t2:meter == 10
	//	   		task t3
  	//				trigger /suite1/f/t2:user_var == 1
	//	   		task t4
  	//				trigger /suite1/f/t2:repeat_var == 2
    //	   endfamily
	//	endsuite
	//
	// In this test case all triggers in suite2 will evalate true, we then delete suite1 and
  	// all the triggers should evaluate false; This is used to test the shared ptr in
  	// the expression, which hold the reference nodes. When the reference nodes are deleted
  	// then the expression should not evaluate
 	Defs defs;
	{
		suite_ptr suite = Suite::create("suite1" );
		family_ptr f = Family::create("f" );
		task_ptr t0 = Task::create("t0" );
		task_ptr t1 = Task::create("t1" );
		task_ptr t2 = Task::create("t2" );
		task_ptr t3 = Task::create("t3" );
		task_ptr t4 = Task::create("t4" );
		t0->addDefStatus(DState::COMPLETE);
		t1->addEvent(Event(0,"event",true));
		t2->addMeter(Meter("meter",0,100,100));
		t3->addVariable(Variable("user_var","1"));
		t4->addRepeat(RepeatInteger("repeat_var",0,10,2));

		f->addTask(t0);
		f->addTask(t1);
		f->addTask(t2);
		f->addTask(t3);
		f->addTask(t4);
		suite->addFamily(f);
		defs.addSuite( suite );
	}
 	{
		suite_ptr suite = Suite::create("suite2" );
		family_ptr f = Family::create("f" );
		task_ptr t0 = Task::create("t0" );
		task_ptr t1 = Task::create("t1" );
		task_ptr t2 = Task::create("t2" );
		task_ptr t3 = Task::create("t3" );
		task_ptr t4 = Task::create("t4" );
		t0->add_trigger("/suite1/f/t0 == complete");
		t1->add_trigger("/suite1/f/t1:event == set");
		t2->add_trigger("/suite1/f/t2:meter == 10");
		t3->add_trigger("/suite1/f/t3:user_var == 1");
		t4->add_trigger("/suite1/f/t4:repeat_var == 2");

		f->addTask(t0);
		f->addTask(t1);
		f->addTask(t2);
		f->addTask(t3);
		f->addTask(t4);
		suite->addFamily(f);
		defs.addSuite( suite );
	}

 	// begin. This will reset all attributes
 	defs.beginAll();

 	// setup attrbutes in suite1 so that evalaution will succeed in suite 2
 	// *** this must be in its own scope otherwise the shared_ptr will keep the node alive
 	{
 		node_ptr t1 =  defs.findAbsNode("/suite1/f/t1"); t1->changeEvent("event","set");
 		node_ptr t2 =  defs.findAbsNode("/suite1/f/t2"); t2->changeMeter("meter",10);
 		node_ptr t4 =  defs.findAbsNode("/suite1/f/t4"); t4->changeRepeat("2");
 		// cout << defs;
 	}

 	// evalate the triggers in suite2
 	{
		node_ptr suite2 =  defs.findAbsNode("/suite2");
		std::vector<task_ptr> suite2_tasks;
		suite2->get_all_tasks(suite2_tasks);
		BOOST_REQUIRE_MESSAGE(suite2_tasks.size() == 5, "Expected 5 tasks on suite2 but found " << suite2_tasks.size());

		for(auto & suite2_task : suite2_tasks) {
			BOOST_REQUIRE_MESSAGE( suite2_task->triggerAst()->evaluate(), "Expected task " << suite2_task->absNodePath() << " to evaluate");
		}
 	}


	// Ok Now delete suite1,
	// *** It is extremely important that shared_ptr for '/suite1' is in its own
	// *** scope, otherwise it will keep the 'suite' live, and NOT delete the limits
	{
		node_ptr suite1 =  defs.findAbsNode("/suite1");
		BOOST_REQUIRE_MESSAGE( suite1.get(), "Could not find the suite we want to delete?");
		BOOST_REQUIRE_MESSAGE( defs.deleteChild(suite1.get()), "Deletion failed?");
		BOOST_REQUIRE_MESSAGE( !defs.findAbsNode("/suite1").get(), "Deletion failed?");
	}

	// revaluate the triggers in suite2. This should fail, since we have delete suite1
 	{
		node_ptr suite2 =  defs.findAbsNode("/suite2");
		std::vector<task_ptr> suite2_tasks;
		suite2->get_all_tasks(suite2_tasks);
		BOOST_REQUIRE_MESSAGE(suite2_tasks.size() == 5, "Expected 5 tasks on suite2 but found " << suite2_tasks.size());

		for(auto & suite2_task : suite2_tasks) {
			BOOST_REQUIRE_MESSAGE( !suite2_task->triggerAst()->evaluate(), "Expected task " << suite2_task->absNodePath() << " to fail evaluation");
		}
 	}

	/// Destroy System singleton to avoid valgrind from complaining
	System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()


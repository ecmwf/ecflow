/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_defstatus )
{
	cout << "ANode:: ...test_defstatus\n";

	// Create a defs file corresponding to:
 	//suite suite1
 	//	family f1
	//   	task t1
	//   	task t2
 	//	family f2
	//   	task t1
	//   	task t2
 	Defs theDefs;
 	suite_ptr suite =  theDefs.add_suite(  "suite1"  );
 	family_ptr f1 =  suite->add_family( "f1" );
 	task_ptr t1 =  f1->add_task( "t1" );
 	task_ptr t2 =  f1->add_task( "t2" );

 	family_ptr f2 =  suite->add_family( "f2" );
 	task_ptr f2_t1 =  f2->add_task( "t1" );
 	task_ptr f2_t2 =  f2->add_task( "t2" );

 	// Get all nodes and tasks for ease of test
 	vector<Node*> nodes;
 	vector<Task*> tasks;
 	theDefs.getAllNodes(nodes);
 	theDefs.getAllTasks(tasks);

 	/// It should be noted that once a suite has begun, it stays begun, however for test purposes we had
 	/// added ability to reset the begin state.

 	/// Test 1: with no defstatus. All nodes should be set to NState::QUEUED
 	theDefs.beginAll();
 	BOOST_FOREACH(Node* n,nodes) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }

   theDefs.requeue();
   BOOST_FOREACH(Node* n,nodes) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }


 	/// Test 2: with defstatus on suite. With complete the status should have been propagated down
 	suite->addDefStatus(DState::COMPLETE);
 	theDefs.reset_begin(); // for test purposes only
 	theDefs.beginAll();
 	BOOST_FOREACH(Node* n,nodes) {
 		BOOST_CHECK_MESSAGE(n->state() == NState::COMPLETE,"Expected complete but found " << NState::toString(n->state()));
  	}

   theDefs.requeue();
   BOOST_FOREACH(Node* n,nodes) {
      BOOST_CHECK_MESSAGE(n->state() == NState::COMPLETE,"Expected complete but found " << NState::toString(n->state()));
   }

 	/// Test 3: defstatus of family f1 and f2. The parent node(suite) should reflect status of children
 	/// Also setting defstatus complete on a family should have propagated it downwards to tasks
 	suite->addDefStatus(DState::default_state()); // reset defstatus
 	f1->addDefStatus(DState::COMPLETE);
 	f2->addDefStatus(DState::COMPLETE);
   theDefs.reset_begin(); // for test purposes only
 	theDefs.beginAll();
   BOOST_FOREACH(Node* n,nodes) {
       BOOST_CHECK_MESSAGE(n->state() == NState::COMPLETE,"Expected complete but found " << NState::toString(n->state()));
   }

   theDefs.requeue();
   BOOST_FOREACH(Node* n,nodes) {
      BOOST_CHECK_MESSAGE(n->state() == NState::COMPLETE,"Expected complete but found " << NState::toString(n->state()));
   }


	// Suspend is really a user interaction the real state suould be queued
   f1->addDefStatus(DState::default_state());       // reset defstatus
   f2->addDefStatus(DState::default_state());       // reset defstatus
   suite->addDefStatus(DState::SUSPENDED);          // reset defstatus
   theDefs.reset_begin(); // for test purposes only
   theDefs.beginAll();
   BOOST_FOREACH(Node* n,nodes) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }

   theDefs.requeue();
   BOOST_FOREACH(Node* n,nodes) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }
}

BOOST_AUTO_TEST_CASE( test_ECFLOW_139 )
{
   cout << "ANode:: ...test_ECFLOW_139\n";

   // Create a defs file corresponding to:
   //suite suite1
   // family f1
   //    task t1; defstatus suspended
   //    task t2; defstatus suspended
   // family f2
   //    task t1; defstatus suspended
   //    task t2; defstatus suspended
   Defs theDefs;
   suite_ptr suite =  theDefs.add_suite(  "suite1"  );
   family_ptr f1 =  suite->add_family( "f1" );
   task_ptr t1 =  f1->add_task( "t1" );
   task_ptr t2 =  f1->add_task( "t2" );
   t1->addDefStatus(DState::SUSPENDED);
   t2->addDefStatus(DState::SUSPENDED);

   family_ptr f2 =  suite->add_family( "f2" );
   task_ptr f2_t1 =  f2->add_task( "t1" );
   task_ptr f2_t2 =  f2->add_task( "t2" );
   f2_t1->addDefStatus(DState::SUSPENDED);
   f2_t2->addDefStatus(DState::SUSPENDED);

   // Get all nodes and tasks for ease of test
   vector<Task*> tasks;
   theDefs.getAllTasks(tasks);

   /// It should be noted that once a suite has begun, it stays begun, however for test purposes we had
   /// added ability to reset the begin state.

   /// Test 1: Check NODE state All nodes should be set to NState::QUEUED
   theDefs.beginAll();
   BOOST_FOREACH(Task* n,tasks) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }

   /// Check: DSTATE
   BOOST_FOREACH(Task* n,tasks) { BOOST_CHECK_MESSAGE(n->dstate() == DState::SUSPENDED,"Expected suspended but found " << DState::toString(n->dstate())); }

   theDefs.requeue();
   BOOST_FOREACH(Task* n,tasks) { BOOST_CHECK_MESSAGE(n->state() == NState::QUEUED,"Expected queued but found " << NState::toString(n->state())); }
   BOOST_FOREACH(Task* n,tasks) { BOOST_CHECK_MESSAGE(n->dstate() == DState::SUSPENDED,"Expected suspended but found " << DState::toString(n->dstate())); }
}

BOOST_AUTO_TEST_SUITE_END()

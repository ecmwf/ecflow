//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include <string>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "System.hpp"
#include "PrintStyle.hpp"
#include "Limit.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_add_limit )
{
	cout << "Base:: ...test_add_limit\n";
	suite_ptr suite = Suite::create("suite");
	suite->addLimit(Limit("fast",1)) ;        // " Adding limit first time should be ok");
	BOOST_REQUIRE_THROW(suite->addLimit(Limit("fast",1)), std::runtime_error); //" Adding same limit second time should fail");

	suite->addInLimit(InLimit("fast","/suite")); //" Adding in-limit first time should be ok");
 	BOOST_REQUIRE_THROW(suite->addInLimit(InLimit("fast","/suite")), std::runtime_error); // " Adding in-limit second time should fail");
}

BOOST_AUTO_TEST_CASE( test_limit_increment )
{
   cout << "Base:: ...test_limit_increment\n";

   // Test than when a job is submitted multiple times, it should only consume 1 token
   //
   // Create the defs file
   // suite suite
   //     limit fast 10
   //    family f
   //          inlimit /suite:fast
   //          task t1
   //    endfamily
   // endsuite
   Defs defs;
   std::string limitName = "fast";
   std::string pathToLimit = "/suite";
   std::string suitename = "suite";
   suite_ptr s = defs.add_suite(suitename);
   family_ptr f = s->add_family("f");
   task_ptr  t1 = f->add_task("t1");
   {
      f->addInLimit(InLimit(limitName,pathToLimit));
      s->addLimit(Limit(limitName,10));
   }
   // cerr << defs << "\n";


   {
      // Resolve dependencies:: BEFORE calling beginCmd, should be a NO OP
      JobsParam jobsParam; // create jobs = false, spawn jobs = false
      Jobs jobs(&defs);
      BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
      BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 tasks to submit but found " << jobsParam.submitted().size());
   }

   {
      // Create a request to begin suite
      // make sure chosen suite can begin to resolve dependencies.
      // beginning the suite will:
      //     1/ set all children to the QUEUED state
      //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
      //
      //  Resolve dependencies. Only one task should be submitted due to Limit
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd(suitename)));
      BOOST_CHECK_MESSAGE( s->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s->state()));
      BOOST_CHECK_MESSAGE( f->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f->state()));
      BOOST_CHECK_MESSAGE( t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));
   }

   {
      // Resolve dependencies. No task should submit since we have reached the limit
      JobsParam jobsParam;  // create jobs = false, spawn jobs = false
      Jobs jobs(&defs);
      BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
      BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 task to submit but found " << jobsParam.submitted().size());
   }

   {
      // Try resubmitting the an active task, it should still only consume 1 token in the limit
      BOOST_CHECK_MESSAGE( t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));

      // Hack the state, to allow job to re- resubmitted
      t1->setStateOnly(NState::QUEUED);

      JobsParam jobsParam;  // create jobs = false, spawn jobs = false
      Jobs jobs(&defs);
      BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
      BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 1, "Expected 1 task to submit but found " << jobsParam.submitted().size());

      // Limit should still have only consumed one token
      limit_ptr limit = s->find_limit(limitName);
      BOOST_CHECK_MESSAGE( limit.get(), "Limit not found");
      BOOST_CHECK_MESSAGE( limit->value() == 1, "Expected limit of value 1, but found " << limit->value());
      BOOST_CHECK_MESSAGE( limit->paths().size() == 1, "Expected Only 1 task path in limit but found " << limit->paths().size());
   }
}


//===============================================================================
// This TEST is used to test limit and inLimit.
// Both examples taken from the documentation
BOOST_AUTO_TEST_CASE( test_limit )
{
	cout << "Base:: ...test_limit\n";

	///////////////////////////////////////////////////////////////////////////
	// Create the defs file
	//	suite suite
	//     limit fast 1
	//	   family f
	//          inlimit /suite:fast
	//	   		task t1
 	//	   		task t2
 	//	   endfamily
	//	endsuite
	Defs defs;
 	std::string limitName = "fast";
 	std::string pathToLimit = "/suite";
	std::string suitename = "suite";
 	family_ptr f = Family::create("f");
	task_ptr  t1 = Task::create("t1");
	task_ptr  t2 = Task::create("t2");
	suite_ptr s = Suite::create(suitename);
 	{
 		f->addTask(  t1 );
		f->addTask(  t2);
		f->addInLimit(InLimit(limitName,pathToLimit));

 		s->addFamily(f);
		s->addLimit(Limit(limitName,1));

		defs.addSuite( s );
	}
  	// cerr << defs << "\n";


	//*******************************************************************************
	// Resolve dependencies:: BEFORE calling beginCmd, should be a NO OP
  	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
  	 	BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 tasks to submit but found " << jobsParam.submitted().size());
   	}

	// ***********************************************************************
	// Create a request to begin suite
	// make sure chosen suite can begin to resolve dependencies.
	// beginning the suite will:
  	//     1/ set all children to the QUEUED state
  	//     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
  	//
	//  Resolve dependencies. Only one task should be submitted due to Limit
  	//
 	{
 		TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd(suitename)));
 	 	BOOST_CHECK_MESSAGE( s->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s->state()));
	 	BOOST_CHECK_MESSAGE( f->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f->state()));
		BOOST_CHECK_MESSAGE( t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));
		BOOST_CHECK_MESSAGE( t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(t2->state()));
  	}

	//*******************************************************************************
	// Resolve dependencies. No task should submit since we have reached the limit
	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
 		BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 task to submit but found " << jobsParam.submitted().size());
 	}

 	{
		std::string errorMsg;
		BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg), errorMsg);
 	}
}


BOOST_AUTO_TEST_CASE( test_limit1 )
{
	cout << "Base:: ...test_limit1\n";
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create the defs file
	//	suite suite
	//     limit disk 50
	//	   family f
	//          inlimit /suite:disk 20
	//	   		task t1
 	//	   		task t2
 	//	   		task t3
 	//	   endfamily
	//	endsuite
	//
	//  Test that only 2 Tasks can run
	Defs defs;
 	std::string limitName = "disk";
 	std::string pathToLimit = "/suite";
	std::string suitename = "suite";
 	family_ptr f = Family::create("f");
	task_ptr  t1 = Task::create("t1");
	task_ptr  t2 = Task::create("t2");
	task_ptr  t3 = Task::create("t3");
	suite_ptr s =  Suite::create(suitename);
 	{
 		f->addTask(  t1 );
		f->addTask(  t2 );
		f->addTask(  t3 );
		f->addInLimit(InLimit(limitName,pathToLimit,20));

 		s->addFamily(f);
		s->addLimit(Limit(limitName,50));

		defs.addSuite( s );
	}
 	// cerr << defs << "\n";

	//*******************************************************************************
	// Resolve dependencies:: BEFORE calling beginCmd, should be a NO OP
  	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
 	 	BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 tasks to submit but found " << jobsParam.submitted().size());
   	}

	// ***********************************************************************
	// Create a request to begin suite
	// make sure chosen suite can begin to resolve dependencies.
	// beginning the suite will:
  	//     1/ set all children to the QUEUED state
  	//     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
  	//
	//  Resolve dependencies. Only TWO task should be active due to Limit
 	{
 		TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd(suitename)));
 	 	BOOST_CHECK_MESSAGE( s->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s->state()));
	 	BOOST_CHECK_MESSAGE( f->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f->state()));
		BOOST_CHECK_MESSAGE( t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));
		BOOST_CHECK_MESSAGE( t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(t2->state()));
		BOOST_CHECK_MESSAGE( t3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(t3->state()));
  	}

	//*******************************************************************************
	// Resolve dependencies. No task should submit since we have reached the limit
	{
		JobsParam jobsParam; // create jobs = false, spawn jobs = false
		Jobs jobs(&defs);
 		BOOST_CHECK_MESSAGE( jobs.generate(jobsParam),jobsParam.getErrorMsg());
 		BOOST_CHECK_MESSAGE( jobsParam.submitted().size() == 0, "Expected 0 task to submit but found " << jobsParam.submitted().size());
		if (jobsParam.submitted().size() != 0 ) {
			BOOST_FOREACH(Submittable* t, jobsParam.submitted()) { cerr << "Submittable " << t->absNodePath() << "\n";}
		}
 	}

 	{
		std::string errorMsg;
		BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg), errorMsg);
 	}
}


BOOST_AUTO_TEST_CASE( test_limit_references_after_delete )
{
	/// In-limit have a reference to a limit. This limit can be on another node. If that node is deleted
	/// The limits are also deleted, hence we need to ensure in-limit reference to limits that are being
	/// deleted are cleared. Currently we use shared_ptr to achieve this
 	cout << "Base:: ...test_limit_references_after_delete\n";

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Create the defs file
	//	suite suiteLimits    # the limit we want delete
	//     limit limit1  1
	//     limit limit2  1
	//     limit limit3  1
	//	endsuite
	//
	//	suite suite
 	//	   family f
	//          inlimit /suiteLimits:limit1    # check limits references cleared after suiteLimits is deleted
	//	   		task t1
  	//	   endfamily
	//	   family f1
	//          inlimit /suiteLimits:limit2
	//	   		task t1
  	//	   endfamily
	//	   family f2
	//          inlimit /suiteLimits:limit3
	//	   		task t1
  	//	   endfamily
	//	endsuite
	//
	// In this test case we will delete the suiteLimits, and then test to ensure the the in-limits
	// in suite have been reset.
	Defs defs;
	{
		suite_ptr suite = Suite::create("suiteLimits" );
 		suite->addLimit(Limit("limit1",1));
 		suite->addLimit(Limit("limit2",1));
 		suite->addLimit(Limit("limit3",1));
		defs.addSuite( suite );
	}

	InLimit in_limit1("limit1","/suiteLimits");
	InLimit in_limit2("limit2","/suiteLimits");
	InLimit in_limit3("limit3","/suiteLimits");
	{
		suite_ptr suite = Suite::create("suite");

		family_ptr fam = Family::create("f");
		fam->addTask( Task::create("t1") );
  		fam->addInLimit(in_limit1);
		suite->addFamily(fam);

		family_ptr fam2 = Family::create("f1");
		fam2->addTask(  Task::create("t1") );
 		fam2->addInLimit(in_limit2);
		suite->addFamily(fam2);

		family_ptr fam3 = Family::create("f2");
		fam3->addTask(  Task::create("t1") );
  		fam3->addInLimit(in_limit3);
		suite->addFamily(fam3);

 		defs.addSuite( suite );
	}


	// First check that in-limit reference have been setup
	node_ptr f = defs.findAbsNode("/suite/f");
	node_ptr f1 = defs.findAbsNode("/suite/f1");
	node_ptr f2 = defs.findAbsNode("/suite/f2");
	BOOST_REQUIRE_MESSAGE( f.get() && f1.get() && f2.get(), "Failed to find family than contain the inlimits");


	BOOST_REQUIRE_MESSAGE( f->findInLimitByNameAndPath(in_limit1) &&
	                       f1->findInLimitByNameAndPath(in_limit2) &&
	                       f2->findInLimitByNameAndPath(in_limit3) , "Failed to find in-limits on the families" << defs);

	// Now check they all reference the limits
	BOOST_REQUIRE_MESSAGE( f->findLimitViaInLimit(in_limit1), "Inlimit does not reference its limit. Post process failed ?");
	BOOST_REQUIRE_MESSAGE( f1->findLimitViaInLimit(in_limit2), "Inlimit does not reference its limit. Post process failed ?");
	BOOST_REQUIRE_MESSAGE( f2->findLimitViaInLimit(in_limit3), "Inlimit does not reference its limit. Post process failed ?");

	// Ok Now delete suiteLimits, as this is where the limits reside
	// *** It is extremely important that shared_ptr for '/suiteLimit' is in its own
	// *** scope, otherwise it will keep the 'suite' live, and NOT delete the limits
	{
		node_ptr suiteLimits =  defs.findAbsNode("/suiteLimits");
		BOOST_REQUIRE_MESSAGE( suiteLimits.get(), "Could not find the suite we want to delete?");
		BOOST_REQUIRE_MESSAGE( defs.deleteChild(suiteLimits.get()), "Deletion failed?");
		BOOST_REQUIRE_MESSAGE( !defs.findAbsNode("/suiteLimits").get(), "Deletion failed?");
	}

	BOOST_REQUIRE_MESSAGE( !f->findLimitViaInLimit(in_limit1), "In-limit still references limit that has been deleted?");
	BOOST_REQUIRE_MESSAGE( !f1->findLimitViaInLimit(in_limit2), "In-limit still references limit that has been deleted?");
	BOOST_REQUIRE_MESSAGE( !f2->findLimitViaInLimit(in_limit3), "In-limit still references limit that has been deleted?");
}

BOOST_AUTO_TEST_CASE( test_limits_after_force_cmd )
{
   cout << "Base:: ...test_limits_after_force_cmd\n";

   // Create the following defs
   // suite s1
   //   limit A 10
   //   family f1
   //     inlimit A
   //     task t1
   //     task t2
   //   family f2
   //     inlimit A
   //     task t1
   //     task t2

   // When all the tasks are running/submitted state, limit A should have consumed 4 tokens
   // However if we how force family f2 to be complete, then the limit should be reset back to 2 tokens.
   // Repeating for family f1, should result with a Limit with no tokens
   Defs defs;
   suite_ptr s1 =  defs.add_suite("s1");
   s1->addLimit(Limit("A",10));
   family_ptr f1 = s1->add_family("f1");
   f1->addInLimit( InLimit("A","/s1"));
   task_ptr f1_t1 = f1->add_task("t1");
   task_ptr f1_t2 = f1->add_task("t2");
   family_ptr f2 = s1->add_family("f2");
   f2->addInLimit( InLimit("A","/s1"));
   task_ptr f2_t1 = f2->add_task("t1");
   task_ptr f2_t2 = f2->add_task("t2");
   //cout << defs;

   // Create a request to begin suite
   // make sure chosen suite can begin to resolve dependencies.
   // beginning the suite will:
   //     1/ set all children to the QUEUED state
   //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
   //
   //  Resolve dependencies. All tasks are within the limit and hence should start
   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("/s1")));
      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t2->state()));

      limit_ptr the_limit = s1->find_limit("A");
      BOOST_CHECK_MESSAGE( the_limit, "Could not find limit");
      BOOST_CHECK_MESSAGE( the_limit->value() == 4,"Expected limit value to be 4 but found " << the_limit->value());
   }

   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(f2->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t2->state()));

      limit_ptr the_limit = s1->find_limit("A");
      BOOST_CHECK_MESSAGE( the_limit, "Could not find limit");
      BOOST_CHECK_MESSAGE( the_limit->value() == 2,"Expected limit value to be 2 but found " << the_limit->value());
   }

   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(f1->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( s1->state() == NState::COMPLETE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::COMPLETE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::COMPLETE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t2->state()));

      limit_ptr the_limit = s1->find_limit("A");
      BOOST_CHECK_MESSAGE( the_limit, "Could not find limit");
      BOOST_CHECK_MESSAGE( the_limit->value() == 0,"Expected limit value to be 0 but found " << the_limit->value());
   }
}

BOOST_AUTO_TEST_CASE( test_limits_after_requeue_family_ECFLOW_196 )
{
   cout << "Base:: ...test_limits_after_requeue_family_ECFLOW_196\n";

   // This test is used to ensure that, requeue causes node to release tokens held by the Limits

   // Create the following defs
   // suite s1
   //   limit A 10
   //   family f1
   //     inlimit A
   //     task t1
   //     task t2
   //   family f2
   //     inlimit A
   //     task t1
   //     task t2

   // When all the tasks are running/submitted state, limit A should have consumed 4 tokens
   // However if we how force family f2 to be requeud, then the limit should be reset back to 2 tokens.
   Defs defs;
   suite_ptr s1 =  defs.add_suite("s1");
   s1->addLimit(Limit("A",10));
   family_ptr f1 = s1->add_family("f1");
   f1->addInLimit( InLimit("A","/s1"));
   task_ptr f1_t1 = f1->add_task("t1");
   task_ptr f1_t2 = f1->add_task("t2");
   family_ptr f2 = s1->add_family("f2");
   f2->addInLimit( InLimit("A","/s1"));
   task_ptr f2_t1 = f2->add_task("t1");
   task_ptr f2_t2 = f2->add_task("t2");
   //cout << defs;


   // Create a request to begin suite
   // make sure chosen suite can begin to resolve dependencies.
   // beginning the suite will:
   //     1/ set all children to the QUEUED state
   //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
   //
   //  Resolve dependencies. All tasks are within the limit and hence should start
   int expected_limit_value = 4;
   limit_ptr the_limit = s1->find_limit("A");
   BOOST_CHECK_MESSAGE( the_limit, "Could not find limit");
   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("/s1")));
      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t2->state()));

      BOOST_CHECK_MESSAGE( the_limit->value() == expected_limit_value,"Expected limit value to be 4 but found " << the_limit->value());
   }

   {
      expected_limit_value = 2;
      // We now want to re-queue node f2, and check to see that limit tokens are released after the re-queue
      // However, the Mock server, will call resolve dependencies after re-queue command , and place nodes *back* into active
      // state, to *STOP* this, we will add a trigger, so that the dependencies will not resolve, and hence nodes stay queued
      f2->add_trigger("1 == 0");
      TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd(f2->absNodePath(),RequeueNodeCmd::FORCE)));

      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));

      BOOST_CHECK_MESSAGE( the_limit->value() == expected_limit_value,"Expected limit value to be 2 but found " << the_limit->value());
   }

   {
      expected_limit_value = 0;
      // We now want to re-queue node f1, and check to see that limit tokens are released after the re-queue
      // However, the Mock server, will call resolve dependencies after re-queue command , and place nodes *back* into active
      // state, to *STOP* this, we will add a trigger, so that the dependencies will not resolve, and hence nodes stay queued
      f1->add_trigger("1 == 0");
      TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd(f1->absNodePath(),RequeueNodeCmd::FORCE)));

      BOOST_CHECK_MESSAGE( s1->state() == NState::QUEUED, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::QUEUED, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::QUEUED, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::QUEUED, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));

      BOOST_CHECK_MESSAGE( the_limit->value() == expected_limit_value,"Expected limit value to be 0 but found " << the_limit->value());
   }
}

BOOST_AUTO_TEST_CASE( test_limits_after_requeue_task_ECFLOW_196 )
{
   cout << "Base:: ...test_limits_after_requeue_task_ECFLOW_196\n";

   // This test is used to ensure that, requeue causes node to release tokens held by the Limits

   // Create the following def, i.e using internal and external limits
   // suite s0
   //   limit X 5
   // suite s1
   //   limit A 10
   //   limit B 5
   //   family f1
   //     inlimit A
   //     inlimit B
   //     inlimit X /s0/
   //     task t1
   //     task t2
   //   family f2
   //     inlimit A
   //     inlimit B
   //     inlimit X /s0/
   //     task t1
   //     task t2

   // When all the tasks are running/submitted state, limit A should have consumed 4 tokens
   // However if we how force task to reque, then the token should be released
   Defs defs;
   suite_ptr s0 =  defs.add_suite("s0");
   s0->addLimit(Limit("X",10));

   suite_ptr s1 =  defs.add_suite("s1");
   s1->addLimit(Limit("A",10));
   s1->addLimit(Limit("B",5));
   family_ptr f1 = s1->add_family("f1");
   f1->addInLimit( InLimit("A","/s1"));
   f1->addInLimit( InLimit("B","/s1"));
   f1->addInLimit( InLimit("X","/s0"));
   task_ptr f1_t1 = f1->add_task("t1");
   task_ptr f1_t2 = f1->add_task("t2");
   family_ptr f2 = s1->add_family("f2");
   f2->addInLimit( InLimit("A","/s1"));
   f2->addInLimit( InLimit("B","/s1"));
   f2->addInLimit( InLimit("X","/s0"));
   task_ptr f2_t1 = f2->add_task("t1");
   task_ptr f2_t2 = f2->add_task("t2");

   std::vector<task_ptr> tasks;
   tasks.push_back(f1_t1); tasks.push_back(f1_t2);
   tasks.push_back(f2_t1); tasks.push_back(f2_t2);
   //cout << defs;

   limit_ptr the_A_limit = s1->find_limit("A");
   limit_ptr the_B_limit = s1->find_limit("B");
   limit_ptr the_X_limit = s0->find_limit("X");
   BOOST_CHECK_MESSAGE( the_A_limit && the_B_limit && the_X_limit, "Could not find limits");

   // Create a request to begin suite
   // make sure chosen suite can begin to resolve dependencies.
   // beginning the suite will:
   //     1/ set all children to the QUEUED state
   //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
   //
   //  Resolve dependencies. All tasks are within the limit and hence should start
   int expected_limit_value = 4;
   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("/s1")));
      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t2->state()));

      BOOST_CHECK_MESSAGE( the_A_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_A_limit->value());
      BOOST_CHECK_MESSAGE( the_B_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_B_limit->value());
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());
   }

   {
      // We now want to re-queue node tasks, and check to see that limit tokens are released after the re-queue
      // However, the Mock server, will call resolve dependencies after re-queue command , and place nodes *back* into active
      // state, to *STOP* this, we will add a trigger, so that the dependencies will not resolve, and hence nodes stay queued
      f1->add_trigger("1 == 0");
      f2->add_trigger("1 == 0");

      for(auto & task : tasks) {
         TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd(task->absNodePath(),RequeueNodeCmd::FORCE)));
         expected_limit_value--;
         BOOST_CHECK_MESSAGE( the_A_limit->value() == expected_limit_value,"Expected limit value " << expected_limit_value << " but found " << the_A_limit->value());
         BOOST_CHECK_MESSAGE( the_B_limit->value() == expected_limit_value,"Expected limit value " << expected_limit_value << " but found " << the_B_limit->value());
         BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value " << expected_limit_value << " but found " << the_X_limit->value());
      }
   }
}

BOOST_AUTO_TEST_CASE( test_inlimit_with_family_ECFLOW_878 )
{
   cout << "Base:: ...test_inlimit_with_family_ECFLOW_878\n";

   // This test places a limit on the families. Should ignore the tasks
   // With this test only 1 family can start at a time
   //
   // Create the following def. with inlimit -n, we limit node only
   // suite s0
   //   limit X 1   # Only allow one family at a time to start
   // suite s1
   //   family f1
   //     inlimit -n X
   //     task t1
   //     task t2
   //   family f2
   //     inlimit -n X
   //     task t1
   //     task t2
   //   family f3
   //     inlimit -n X
   //     task t1
   //     task t2

   Defs defs;
   suite_ptr s0 =  defs.add_suite("s0");
   s0->addLimit(Limit("X",1));

   suite_ptr s1 =  defs.add_suite("s1");
   family_ptr f1 = s1->add_family("f1");
   f1->addInLimit(InLimit("X","/s0",1,true));
   task_ptr f1_t1 = f1->add_task("t1");
   task_ptr f1_t2 = f1->add_task("t2");
   family_ptr f2 = s1->add_family("f2");
   f2->addInLimit(InLimit("X","/s0",1,true));
   task_ptr f2_t1 = f2->add_task("t1");
   task_ptr f2_t2 = f2->add_task("t2");
   family_ptr f3 = s1->add_family("f3");
   f3->addInLimit(InLimit("X","/s0",1,true));
   task_ptr f3_t1 = f3->add_task("t1");
   task_ptr f3_t2 = f3->add_task("t2");

   //cout << defs;
   limit_ptr the_X_limit = s0->find_limit("X");
   BOOST_CHECK_MESSAGE( the_X_limit, "Could not find limits");

   // Create a request to begin suite
   // make sure chosen suite can begin to resolve dependencies.
   // beginning the suite will:
   //     1/ set all children to the QUEUED state
   //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
   //
   //  Resolve dependencies. Only 2 task should start
   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("/s1")));

//      PrintStyle::setStyle(PrintStyle::MIGRATE);
//      cout << defs;

      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // should only have 1 path, and it should be path to family f1
      const std::set<std::string>& x_limit_paths =  the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f1->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f1->absNodePath() );

//      // why f2_t1 is queued
//      cout << "why ============================================";
//      std::vector<std::string> vec;
//      f2_t1->bottom_up_why(vec);
//      for(size_t i =0; i < vec.size(); i++) {
//         cout << "why:" << i << " " << vec[i] << "\n";
//      }
   }

   // Now set tasks f1_t1, f1_t2 to complete, this should release the limit, so that next family can run
   {
      std::vector<std::string> task_paths;
      task_paths.push_back(f1_t1->absNodePath());
      task_paths.push_back(f1_t2->absNodePath());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(task_paths,"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( f1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // Paths should be empty
      // should only have 1 path, and it should be path to family f1
      const std::set<std::string>& x_limit_paths = the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f2->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f2->absNodePath() );
   }

   // Now set tasks f2_t1, f2_t2 to complete, this should release the limit, so that next family can run
   {
      std::vector<std::string> task_paths;
      task_paths.push_back(f2_t1->absNodePath());
      task_paths.push_back(f2_t2->absNodePath());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(task_paths,"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( f1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // should only have 1 path, and it should be path to family f3
      const std::set<std::string>& x_limit_paths = the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f3->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f3->absNodePath() );
   }

   // Now set tasks f3_t1, f3_t2 to complete, this should release the limit
   {
      std::vector<std::string> task_paths;
      task_paths.push_back(f3_t1->absNodePath());
      task_paths.push_back(f3_t2->absNodePath());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(task_paths,"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( f1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 0;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // Paths should be empty
      const std::set<std::string>& x_limit_paths = the_X_limit->paths();
      BOOST_CHECK_MESSAGE( the_X_limit->paths().empty() , "Expected no paths, but found" <<  x_limit_paths.size());
   }

//   PrintStyle::setStyle(PrintStyle::MIGRATE);
//   cout << defs;
}

BOOST_AUTO_TEST_CASE( test_inlimit_ECFLOW_878 )
{
   cout << "Base:: ...test_inlimit_ECFLOW_878\n";

   // This test places a limit on the families. Should ignore the tasks
   // With this test only 1 family can start at a time.
   // However we *ALSO* want to constrain the tasks, to only 1 task at a time
   //
   // Create the following def. with inlimit -n, we limit node only
   // suite s0
   //   limit X 1   # Only allow one family at a time to start
   //   limit T 1   # Only allow one task at a time to start
   // suite s1
   //   family f1
   //     inlimit -n X
   //     inlimit T
   //     task t1
   //     task t2
   //   family f2
   //     inlimit -n X
   //     inlimit T
   //     task t1
   //     task t2
   //   family f3
   //     inlimit -n X
   //     inlimit T
   //     task t1
   //     task t2

   Defs defs;
   suite_ptr s0 =  defs.add_suite("s0");
   s0->addLimit(Limit("X",1));
   s0->addLimit(Limit("T",1));

   suite_ptr s1 =  defs.add_suite("s1");
   family_ptr f1 = s1->add_family("f1");
   f1->addInLimit(InLimit("X","/s0",1,true));
   f1->addInLimit(InLimit("T","/s0",1));
   task_ptr f1_t1 = f1->add_task("t1");
   task_ptr f1_t2 = f1->add_task("t2");
   family_ptr f2 = s1->add_family("f2");
   f2->addInLimit(InLimit("X","/s0",1,true));
   f2->addInLimit(InLimit("T","/s0",1));
   task_ptr f2_t1 = f2->add_task("t1");
   task_ptr f2_t2 = f2->add_task("t2");
   family_ptr f3 = s1->add_family("f3");
   f3->addInLimit(InLimit("X","/s0",1,true));
   f3->addInLimit(InLimit("T","/s0",1));
   task_ptr f3_t1 = f3->add_task("t1");
   task_ptr f3_t2 = f3->add_task("t2");

   //cout << defs;
   limit_ptr the_X_limit = s0->find_limit("X");
   limit_ptr the_T_limit = s0->find_limit("T");
   BOOST_CHECK_MESSAGE( the_X_limit && the_T_limit, "Could not find limits");

   // Create a request to begin suite
   // make sure chosen suite can begin to resolve dependencies.
   // beginning the suite will:
   //     1/ set all children to the QUEUED state
   //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
   //
   //  Resolve dependencies. Only 1 task should start
   {
      TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("/s1")));

      //PrintStyle::setStyle(PrintStyle::MIGRATE);
      //cout << defs;

      BOOST_CHECK_MESSAGE( s1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(s1->state()));
      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // should only have 1 path, and it should be path to family f1
      const std::set<std::string>& x_limit_paths =  the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f1->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f1->absNodePath() );

      const std::set<std::string>& t_limit_paths =  the_T_limit->paths();
      BOOST_CHECK_MESSAGE( t_limit_paths.size()==1 , "Expected one path in limit but found " <<  t_limit_paths.size());

//      // why f2_t1 is queued
//      cout << "why ============================================";
//      std::vector<std::string> vec;
//      f2_t1->bottom_up_why(vec);
//      for(size_t i =0; i < vec.size(); i++) {
//         cout << "why:" << i << " " << vec[i] << "\n";
//      }
   }

   // Now set tasks f1_t1 complete, this should release  t1_t2
   {
      std::vector<std::string> task_paths; task_paths.push_back(f1_t1->absNodePath());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(task_paths,"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( f1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // should only have 1 path, and it should be path to family f1
      const std::set<std::string>& x_limit_paths =  the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f1->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f1->absNodePath() );

      const std::set<std::string>& t_limit_paths =  the_T_limit->paths();
      BOOST_CHECK_MESSAGE( t_limit_paths.size()==1 , "Expected one path in limit but found " <<  t_limit_paths.size());
   }

   // Now set tasks f1_t2 complete, this should release  f2_1
   {
      std::vector<std::string> task_paths; task_paths.push_back(f1_t2->absNodePath());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new ForceCmd(task_paths,"complete",true /*recursive */, false /* set Repeat to last value */)));

      BOOST_CHECK_MESSAGE( f1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1->state()));
      BOOST_CHECK_MESSAGE( f1_t1->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t1->state()));
      BOOST_CHECK_MESSAGE( f1_t2->state() == NState::COMPLETE, "expected state NState::COMPLETE, but found to be " << NState::toString(f1_t2->state()));
      BOOST_CHECK_MESSAGE( f2->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2->state()));
      BOOST_CHECK_MESSAGE( f2_t1->state() == NState::ACTIVE, "expected state NState::ACTIVE, but found to be " << NState::toString(f2_t1->state()));
      BOOST_CHECK_MESSAGE( f2_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f2_t2->state()));
      BOOST_CHECK_MESSAGE( f3->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3->state()));
      BOOST_CHECK_MESSAGE( f3_t1->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t1->state()));
      BOOST_CHECK_MESSAGE( f3_t2->state() == NState::QUEUED, "expected state NState::QUEUED, but found to be " << NState::toString(f3_t2->state()));

      int expected_limit_value = 1;
      BOOST_CHECK_MESSAGE( the_X_limit->value() == expected_limit_value,"Expected limit value to be " << expected_limit_value << " but found " << the_X_limit->value());

      // should only have 1 path, and it should be path to family f2
      const std::set<std::string>& x_limit_paths =  the_X_limit->paths();
      BOOST_CHECK_MESSAGE( x_limit_paths.size()==1 , "Expected one path in limit but found " <<  x_limit_paths.size());
      BOOST_CHECK_MESSAGE( x_limit_paths.find(f2->absNodePath()) != x_limit_paths.end(), "Expected to find path " << f2->absNodePath() );

      const std::set<std::string>& t_limit_paths =  the_T_limit->paths();
      BOOST_CHECK_MESSAGE( t_limit_paths.size()==1 , "Expected one path in limit but found " <<  t_limit_paths.size());
   }

   //PrintStyle::setStyle(PrintStyle::MIGRATE);
   //cout << defs;

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

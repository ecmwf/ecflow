//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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


	/// Destroy System singleton to avoid valgrind from complaining
	System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

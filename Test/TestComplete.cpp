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
// Description :
//============================================================================
#include "ServerTestHarness.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
//
// This test shows the use of repeat and complete. this shows how
// we can repeat a family without waiting for all the children to
// complete. See page 65 of user manual
//
// As protection against user, a family should not complete
// if any of its children are still in ACTIVE or SUBMIITED state.
//
// **************************************************************************
// Note: When we use a complete expression. Node resolution will set complete
//      hierarchically
// **************************************************************************
BOOST_AUTO_TEST_CASE( test_complete )
{
   // Added since in 4.6.0 no longer sets state if it has not changed.
   if (getenv("ECF_DISABLE_TEST_FOR_OLD_SERVERS")) {
      std::cout << "\n    Disable test_complete for old server ,re-enable when 4.6.0 is minimum version\n";
      return;
   }

	DurationTimer timer;
	cout << "Test:: ...test_complete " << flush;


   TestClean clean_at_start_and_end;

	// Create the defs file corresponding to the text below
	// ECF_HOME variable is automatically added by the test harness.
	// ECF_INCLUDE variable is automatically added by the test harness.
	// SLEEPTIME variable is automatically added by the test harness.
	// ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
	//                     This is substituted in sms includes
	//                     Allows test to run without requiring installation

 	//# Note: we have to use relative paths, since these tests are relocatable
	//	suite test_complete
	//	  family family
	//      repeat integer YMD 0 1
	//	   	complete ./family/check:nofiles  # repeat family with waiting for children to complete
	//	   	task check
	//	   		event 1 nofiles
	//	    task t1
	//        trigger check==complete
	//      task t2
	//         trigger t2 == complete   #  never runs
	//	    endfamily
	//	endsuite
 	std::string eventName = "nofiles";
  	Defs theDefs;
 	{
      suite_ptr suite = theDefs.add_suite("test_complete");
      family_ptr fam = suite->add_family("family");
 		fam->addRepeat( RepeatInteger("VAR",0,1,1));  // repeat family 2 times
 		fam->add_complete( "/test_complete/family/check:nofiles" );
		fam->addVerify( VerifyAttr(NState::COMPLETE,2) );       // family should complete 2 times

      task_ptr task_check = fam->add_task("check");
 		task_check->addEvent( Event(1,eventName) );
 		task_check->addVerify( VerifyAttr(NState::COMPLETE,2) );

      task_ptr task_t1 = fam->add_task("t1");
		task_t1->add_trigger( "check == complete");
		task_t1->addVerify( VerifyAttr(NState::COMPLETE,2) );

      task_ptr task_t2 = fam->add_task("t2");
		task_t2->add_trigger( "t2 == complete" );
		task_t2->addVerify( VerifyAttr(NState::COMPLETE,2) );
 	}

 	// The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
 	serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_complete.def") );

	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_complete_does_not_hold )
{
   DurationTimer timer;
   cout << "Test:: ...test_complete_does_not_hold " << flush;

   /// This test shows that a complete expression should not hold a node
   /// A complete should complete a node, and not hold it

   // suite test_complete
   //   family family
   //     complete 1 == 0            # impossible expression that never evaluates
   //     task t1                    # task t1 should still run
   //     task t2
   //         complete 1 == 0        # impossible expression that never evaluates
   //         trigger t1 == complete # task t2 should still run
   //     endfamily
   // endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_complete_does_not_hold");
      family_ptr fam = suite->add_family("family");
      fam->add_complete( "1 == 0");
      task_ptr t1 = fam->add_task("t1");
      t1->addVerify( VerifyAttr(NState::COMPLETE,1) );
      task_ptr t2 = fam->add_task("t2");
      t2->add_trigger( "t1 == complete");
      t2->add_complete( "1 == 0");
      t2->addVerify( VerifyAttr(NState::COMPLETE,1) );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_complete_does_not_hold.def") );

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_complete_with_empty_family )
{
   DurationTimer timer;
   cout << "Test:: ...test_complete_with_empty_family " << flush;

   // Test a family with a complete expression and that has no children

   // suite test_complete_with_empty_family
   //   family empty
   //      complete /test_complete_with_empty_family/family/t1 == complete and
   //               /test_complete_with_empty_family/family/t2 == complete
   //   family family
   //       task t1
   //       task t2
   //     endfamily
   // endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_complete_with_empty_family");
      family_ptr empty = suite->add_family("empty");
      empty->add_complete( "/test_complete_with_empty_family/family/t1 == complete and /test_complete_with_empty_family/family/t2 == complete");
      empty->addVerify( VerifyAttr(NState::COMPLETE,1) );
      family_ptr fam = suite->add_family("family");
      fam->add_task("t1")->addVerify( VerifyAttr(NState::COMPLETE,1) );
      fam->add_task("t2")->addVerify( VerifyAttr(NState::COMPLETE,1) );
   }

   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_complete_with_empty_family.def") );

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

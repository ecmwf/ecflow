
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #17 $ 
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
#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "PrintStyle.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( TestSuite )

BOOST_AUTO_TEST_CASE( test_requeue_node )
{
	DurationTimer timer;
	cout << "Test:: ...test_requeue_node " << flush;
   TestClean clean_at_start_and_end;

	// Create the defs file corresponding to the text below
	// ECF_HOME variable is automatically added by the test harness.
	// ECF_INCLUDE variable is automatically added by the test harness.
	// SLEEPTIME variable is automatically added by the test harness.
	// ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
	//                     This is substituted in sms includes
	//                     Allows test to run without requiring installation

 	//# Note: we have to use relative paths, since these tests are relocatable
	//suite test_repeat_integer
	//	repeat integer VAR 0 1 1          # run at 0, 1    2 times
	//	edit SLEEPTIME 1
	//	edit ECF_INCLUDE $ECF_HOME/includes
	//	family family
	//	    repeat integer VAR 0 2 1     # run at 0, 1     2 times
	//   	task t<n>
	//      ....
 	//  	endfamily
	//endsuite

	// Each task/job should be run *4* times, according to the repeats Mimics nested loops
  	Defs theDefs;
 	{
      suite_ptr suite = theDefs.add_suite(  "test_reque" );
 		suite->addRepeat( RepeatInteger("VAR",0,1,1)); // repeat suite 2 times
 		suite->addVerify( VerifyAttr(NState::COMPLETE,2) );
      family_ptr fam = suite->add_family("family" );
 		fam->addRepeat( RepeatInteger("VAR",0,1,1));    // repeat family 2 times
		fam->addVerify( VerifyAttr(NState::COMPLETE,4) );
		int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
  		for(int i=0; i < taskSize; i++) {
  		   task_ptr task = fam->add_task( "t" +   boost::lexical_cast<std::string>(i));
  			task->addVerify( VerifyAttr(NState::COMPLETE,4) );      // task should complete 4 times
 		}
 	}

 	// The test harness will create corresponding directory structure
 	// and populate with standard sms files.
   ServerTestHarness serverTestHarness;
 	serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_reque.def") );

 	// Now re-queue the whole suite
	TestFixture::client().set_throw_on_error( true );
   TestFixture::client().requeue("/test_reque");

   // Wait for test to finish
 	int timeout = 30;
	bool verifyAttrInServer = false;
	defs_ptr serverDefs = serverTestHarness.testWaiter(theDefs,timeout,verifyAttrInServer);
	BOOST_REQUIRE_MESSAGE(serverDefs.get()," Failed to return server after re-queue");

//	cout << "Printing Defs \n";
//	std::cout << *serverDefs.get();

	// since we requed each task should have completed 8 times
	std::vector<Task*> taskVec;
	serverDefs->getAllTasks(taskVec);
	BOOST_FOREACH(Task* t, taskVec) {
 		BOOST_FOREACH(const VerifyAttr& v, t->verifys()) {
 			if (v.state() == NState::COMPLETE) {
 				BOOST_CHECK_MESSAGE(v.actual() == 8,"Expected task " << t->absNodePath() << " to complete 8 times, due to reque, but it completed only " << v.actual() << " times\n");
 			}
 		}
	}

	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

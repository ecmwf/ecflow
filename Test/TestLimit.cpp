//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #25 $ 
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
// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE( test_limit )
{
	DurationTimer timer;
	cout << "Test:: ...test_limit "<< flush;
   TestClean clean_at_start_and_end;

	// Create the defs file corresponding to the text below
	// ECF_HOME variable is automatically added by the test harness.
	// ECF_INCLUDE variable is automatically added by the test harness.
	// SLEEPTIME variable is automatically added by the test harness.
	// ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
	//                     This is substituted in sms includes
	//                     Allows test to run without requiring installation

	//# Test the ecf file can be found via ECF_SCRIPT
	//# Note: we have to use relative paths, since these tests are relocatable
 	// Create the defs file
	//	suite test_limit
	//     limit disk 50
	//     limit fast 1
	//	   edit ECF_HOME data/ECF_HOME    # added by test harness
	//	   edit SLEEPTIME 1
	//	   edit ECF_INCLUDE $ECF_HOME/includes
	//	   family family
	//          inlimit /suite1:fast
	//	   		task t1
 	//	   		task t2
 	//	   		task t3
 	//	   endfamily
	//	   family family2
	//          inlimit /suite1:disk 20
	//	   		task t1
 	//	   		task t2
 	//	   		task t3
 	//	   endfamily
	//	endsuite

  	Defs theDefs;
 	{
 		std::string suiteName = "test_limit";
  		std::string pathToLimit = "/" + suiteName;

      suite_ptr suite = theDefs.add_suite( suiteName );
		suite->addLimit(Limit("fast",1));
		suite->addLimit(Limit("disk",50));

      family_ptr fam = suite->add_family("family");
		fam->addInLimit(InLimit("fast",pathToLimit));
		fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
		int taskSize = 3;
 		for(int i=0; i < taskSize; i++) {
 		   task_ptr task = fam->add_task( "t" + boost::lexical_cast<std::string>(i) );
 			task->addVerify( VerifyAttr(NState::COMPLETE,1) );
 		}

      family_ptr fam2 = suite->add_family("family2");
		fam2->addInLimit(InLimit("disk",pathToLimit,20));
		fam2->addVerify( VerifyAttr(NState::COMPLETE,1) );
 		for(int i=0; i < taskSize; i++) {
 		   task_ptr task = fam2->add_task( "t" + boost::lexical_cast<std::string>(i) );
 			task->addVerify( VerifyAttr(NState::COMPLETE,1) );
  		}
 	}


 	// The test harness will create corresponding directory structure
 	// and populate with standard ecf files.
 	ServerTestHarness serverTestHarness;
 	serverTestHarness.run(theDefs,  ServerTestHarness::testDataDefsLocation( "test_limit.def" ));

	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

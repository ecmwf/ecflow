
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
BOOST_AUTO_TEST_CASE( test_repeat_integer )
{
	DurationTimer timer;
	cout << "Test:: ...test_repeat_integer " << flush;
   TestClean clean_at_start_and_end;

   // ********************************************************************************
   // IMPORTANT: A family will only complete when it has reached the end of the repeats
   // *********************************************************************************

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

	// Each task/job should be run *4* times, according to the repeats
	// Mimics nested loops
	int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
  	Defs theDefs;
 	{
		std::auto_ptr< Suite > suite( new Suite( "test_repeat_integer" ) );
 		std::auto_ptr< Family > fam( new Family( "family" ) );
 		fam->addRepeat( RepeatInteger("VAR",0,1,1));    // repeat family 2 times
		fam->addVerify( VerifyAttr(NState::COMPLETE,2) );
  		for(int i=0; i < taskSize; i++) {
  			std::auto_ptr< Task > task( new Task( "t" + boost::lexical_cast<std::string>(i) ) );
  			task->addVerify( VerifyAttr(NState::COMPLETE,4) );      // task should complete 4 times
  			fam->addTask( task );
 		}
 		suite->addRepeat( RepeatInteger("VAR",0,1,1)); // repeat suite 2 times
 		suite->addFamily( fam );
 		suite->addVerify( VerifyAttr(NState::COMPLETE,1) );
		theDefs.addSuite( suite );
 	}


 	// The test harness will create corresponding directory structure
 	// and populate with standard sms files.
  	ServerTestHarness serverTestHarness(false/*do log file verification*/);
 	serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_integer.def"));

	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_repeat_date )
{
   DurationTimer timer;
   cout << "Test:: ...test_repeat_date " << flush;
   TestClean clean_at_start_and_end;

   // ********************************************************************************
   // IMPORTANT: A family will only complete when it has reached the end of the repeats
   // *********************************************************************************
   //suite test_repeat_date
   // family family
   //    repeat date DATE 20110630 20110632
   //    task t<n>
   //      ....
   //    endfamily
   //endsuite
   int taskSize = 2;
   Defs theDefs;  {
      suite_ptr suite = theDefs.add_suite("test_repeat_date");
      family_ptr fam = suite->add_family("family" );
      fam->addRepeat( RepeatDate("DATE",20110630,20110704));
      fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
      for(int i=0; i < taskSize; i++) {
         task_ptr task = fam->add_task( "t" + boost::lexical_cast<std::string>(i) );
         task->addVerify( VerifyAttr(NState::COMPLETE,5) );
      }
      suite->addVerify( VerifyAttr(NState::COMPLETE,1) );
   }

   // The test harness will create corresponding directory structure
   // and populate with standard sms files.
   ServerTestHarness serverTestHarness(false/*do log file verification*/);
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_date.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}


BOOST_AUTO_TEST_CASE( test_repeat_enumerator )
{
   DurationTimer timer;
   cout << "Test:: ...test_repeat_enumerator " << flush;
   TestClean clean_at_start_and_end;

   // ********************************************************************************
   // IMPORTANT: A family will only complete when it has reached the end of the repeats
   // *********************************************************************************
   //   suite test_repeat_enumerator
   //     family top
   //       family plot
   //         family iasi_plots
   //           repeat enumerated month "200801" "200802"
   //           task t1
   //         endfamily
   //       endfamily
   //     endfamily
   //   endsuite

   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite(  "test_repeat_enumerator" );
      family_ptr top = suite->add_family("top");
      family_ptr plot = top->add_family("plot");
      family_ptr iasi_plots = plot->add_family("iasi_plots");
      vector<string> months; months.reserve(12); months.push_back("200801"); months.push_back("200802");
      iasi_plots->addRepeat(RepeatEnumerated("month",months));
      task_ptr t1 = iasi_plots->add_task("t1");
      t1->addVerify( VerifyAttr(NState::COMPLETE,2) );
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files.
   ServerTestHarness serverTestHarness(false/*do log file verification*/);
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_repeat_enumerator.def") );

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_repeat_defstatus )
{
	DurationTimer timer;
	cout << "Test:: ...test_repeat_defstatus " << flush;
   TestClean clean_at_start_and_end;

	// TEST SHOULD COMPLETE STRAIGHT AWAY SINCE WE HAVE A DEFSTATUS COMPLETE
	// since the complete state is set on all children of suite.
	// Create the defs file corresponding to the text below
 	//# Note: we have to use relative paths, since these tests are relocatable
	//suite test_repeat_defstatus
	// defstatus complete
	//	repeat integer VAR 0 1 1          # run at 0, 1    2 times
	//	edit SLEEPTIME 1
	//	edit ECF_INCLUDE $ECF_HOME/includes
	//	family family
	//	    repeat integer VAR 0 2 1     # run at 0, 1     2 times
	//   	task t<n>
	//      ....
 	//  	endfamily
	//endsuite
	int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
  	Defs theDefs;
 	{
		std::auto_ptr< Suite > suite( new Suite( "test_repeat_defstatus" ) );
		suite->addDefStatus(DState::COMPLETE);

 		std::auto_ptr< Family > fam( new Family( "family" ) );
 		fam->addRepeat( RepeatInteger("VAR",0,1,1));    // repeat family 2 times
		fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
  		for(int i=0; i < taskSize; i++) {
  			std::auto_ptr< Task > task( new Task( "t" + boost::lexical_cast<std::string>(i) ) );
  			task->addVerify( VerifyAttr(NState::COMPLETE,1) );      // task should complete 4 times
  			fam->addTask( task );
 		}
 		suite->addRepeat( RepeatInteger("VAR",0,1,1)); // repeat suite 2 times
 		suite->addFamily( fam );
 		suite->addVerify( VerifyAttr(NState::COMPLETE,1) );
		theDefs.addSuite( suite );
 	}

   ServerTestHarness serverTestHarness(false/*do log file verification*/);
 	serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_repeat_defstatus.def"));
	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}


BOOST_AUTO_TEST_SUITE_END()

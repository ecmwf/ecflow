//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #39 $ 
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
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "Host.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE( test_server_job_submission )
{
	DurationTimer timer;
	cout << "Test:: ...test_server_job_submission "<< flush;
   TestClean clean_at_start_and_end;

 	//# Note: we have to use relative paths, since these tests are relocatable
	//suite test_server_job_submission
	//	edit SLEEPTIME 1
	//	edit ECF_INCLUDE $ECF_HOME/includes
	//	family family
	//   	task t<n>
	//      ....
 	//  	endfamily
	//endsuite
  	Defs theDefs;
 	{
      suite_ptr suite = theDefs.add_suite( "test_server_job_submission" );
      family_ptr fam = suite->add_family("family" );
	 	fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
	 	int taskSize = 3; // on linux 1024 tasks take ~4 seconds for job submission
  		for(int i=0; i < taskSize; i++) {
         task_ptr task = fam->add_task( "t" +   boost::lexical_cast<std::string>(i));
  	 		task->addVerify( VerifyAttr(NState::COMPLETE,1) );
 		}
 	}

 	// The test harness will create corresponding directory structure and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
	serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_server_job_submission.def"));

	cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_restore_defs_from_check_pt )
{
   DurationTimer timer;
   cout << "Test:: ...test_restore_defs_from_check_pt "<< flush;
   TestClean clean_at_start_and_end;
   // **********************************************************************
   // In order for this test to work, we need to disable the automatic
   // check pointing done by the server. Otherwise it will interfere with
   // this test. This is re-enabled at the end of the test
   // ************************************************************************
   Defs theDefs; {
      suite_ptr suite = Suite::create( "test_restore_defs_from_check_pt" );
      suite->addTask( Task::create( "t1" ) );
      theDefs.addSuite( suite );
   }
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_restore_defs_from_check_pt.def"));

   TestFixture::client().set_throw_on_error(false);

   // Disable server check pointing
   BOOST_REQUIRE_MESSAGE(TestFixture::client().checkPtDefs(ecf::CheckPt::NEVER) == 0,CtsApi::checkPtDefs(ecf::CheckPt::NEVER) << " failed should return 0\n" << TestFixture::client().errorMsg());

   Host h;
   std::string check_pt_file_name = h.ecf_checkpt_file(TestFixture::port());

   // make sure we have a valid definition with at least one suite before start
   BOOST_REQUIRE_MESSAGE( TestFixture::client().getDefs() == 0, "Expected getDefs() to succeed\n");
   BOOST_REQUIRE_MESSAGE( TestFixture::client().defs()->suiteVec().size() == 1, "Expected 1 suite at the start\n");

   // Check pt, make sure file exist on disk and has a non zero file size\n";
   BOOST_REQUIRE_MESSAGE( TestFixture::client().checkPtDefs() == 0,"Expected Check-pt to succeed.\n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE( fs::exists(check_pt_file_name),CtsApi::checkPtDefs() << " failed file(" << check_pt_file_name << ") not saved");
   BOOST_REQUIRE_MESSAGE( fs::file_size(check_pt_file_name) !=0,"Expected check point file(" << check_pt_file_name << ") to have file size > 0");

   // Make sure restore from check pt, only works if server is halted and there are no suites\n";
   BOOST_REQUIRE_MESSAGE( TestFixture::client().restoreDefsFromCheckPt() == 1,"Expected restoreDefsFromCheckPt to *FAIL* since server not halted\n");
   BOOST_REQUIRE_MESSAGE( TestFixture::client().haltServer() == 0,"Expected halt server to succeed\n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE( TestFixture::client().restoreDefsFromCheckPt() == 1,"Expected restoreDefsFromCheckPt to *FAIL* since definition not deleted yet\n");
   BOOST_REQUIRE_MESSAGE( TestFixture::client().delete_all() == 0,"Expected delete all nodes to succeed\n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE( TestFixture::client().getDefs() == 0, "Expected getDefs() to succeed, i.e expected empty defs\n");
   BOOST_REQUIRE_MESSAGE( TestFixture::client().defs()->suiteVec().empty(), "Expected no suites, after delete_all()\n");

   // make sure check pt file still exists, sanity test\n";
   BOOST_REQUIRE_MESSAGE( fs::exists(check_pt_file_name),CtsApi::checkPtDefs() << " failed file(" << check_pt_file_name << ") not saved");
   BOOST_REQUIRE_MESSAGE( fs::file_size(check_pt_file_name) !=0,"Expected check point file(" << check_pt_file_name << ") to have file size > 0");

   // make sure restore actually worked, by retrieving the definition and counting the suites\n";
   BOOST_REQUIRE_MESSAGE( TestFixture::client().restoreDefsFromCheckPt() == 0,"Expected restoreDefsFromCheckPt to *SUCCEED*, since server halted and defs deleted \n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE( TestFixture::client().getDefs() == 0, "Get defs failed should return 0\n" << TestFixture::client().errorMsg());
   BOOST_REQUIRE_MESSAGE( TestFixture::client().defs()->suiteVec().size() == 1, "Restore from check pt did not work. expected 1 suite but found " << TestFixture::client().defs()->suiteVec().size() );

   // restore default check pointing, for test that follow
   BOOST_REQUIRE_MESSAGE(TestFixture::client().checkPtDefs(ecf::CheckPt::ON_TIME,120) == 0,CtsApi::checkPtDefs(ecf::CheckPt::ON_TIME) << " failed should return 0\n" << TestFixture::client().errorMsg());

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

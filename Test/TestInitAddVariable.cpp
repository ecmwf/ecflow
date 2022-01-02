//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #39 $
//
// Copyright 2009- ECMWF.
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

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "VerifyAttr.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( TestSuite )

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE( test_init_add_variable )
{
   // Added since in 5.2.0 (only 5.2.0 server supports this behaviour)
   if (getenv("ECF_DISABLE_TEST_FOR_OLD_SERVERS")) {
      std::cout << "\n    Disable test_init_add_variable for old server , re-enable when 5.2.0 is minimum version\n";
      return;
   }

   DurationTimer timer;
   cout << "Test:: ...test_init_add_variable "<< flush;
   TestClean clean_at_start_and_end;

   //# Note: we have to use relative paths, since these tests are relocatable
   //suite test_server_job_submission
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   // edit INIT_ADD_VARIABLES --add "NAME1=1" "NAME2=2"
   // edit COMPLETE_DEL_VARIABLES --remove NAME1 NAME2
   // family family
   //    task t1     # can't place trigger on t1, otherwise we have dead lock. Variables added at ACTIVE, but trigger expression resolved at QUEUE time
   //    task t2
   //       trigger t1:NAME1 == 1 and t1:NAME2 == 2
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite( "test_init_add_variable" );
      suite->add_variable("INIT_ADD_VARIABLES","--add NAME1=1 NAME2=2");
      suite->add_variable("COMPLETE_DEL_VARIABLES","--remove NAME1 NAME2");
      family_ptr fam = suite->add_family("family" );
      fam->addVerify( VerifyAttr(NState::COMPLETE,1) );
      task_ptr t1 = fam->add_task( "t1" );
      t1->addVerify( VerifyAttr(NState::COMPLETE,1) );
      t1->add_variable("SLEEPTIME",boost::lexical_cast<std::string>(TestFixture::job_submission_interval()));

      task_ptr t2 = fam->add_task( "t2" );
      t2->add_trigger("t1 == active and ./t1:NAME1 == 1 and ./t1:NAME2 == 2");
      t2->addVerify( VerifyAttr(NState::COMPLETE,1) );

      theDefs.auto_add_externs(); // since NAME1 and NAME2 not defined
   }
   //cout << theDefs;

   // The test harness will create corresponding directory structure and populate with standard ecf files.
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_init_add_variable.def"));


   TestFixture::client().sync_local();
   BOOST_CHECK_MESSAGE( TestFixture::client().defs(),"Expected defs");

   {
      node_ptr t1 = TestFixture::client().defs()->findAbsNode("/test_init_add_variable/family/t1");
      BOOST_CHECK_MESSAGE( t1,"Expected task to be found");
      const Variable& v1 = t1->findVariable("NAME1");
      const Variable& v2 = t1->findVariable("NAME2");
      BOOST_CHECK_MESSAGE( v1.empty() && v2.empty(),"Expected not to find variables NAME1 and NAME2");
   }
   {
      node_ptr t2 = TestFixture::client().defs()->findAbsNode("/test_init_add_variable/family/t2");
      BOOST_CHECK_MESSAGE( t2,"Expected task to be found");
      const Variable& v1 = t2->findVariable("NAME1");
      const Variable& v2 = t2->findVariable("NAME2");
      BOOST_CHECK_MESSAGE( v1.empty() && v2.empty(),"Expected not to find variables NAME1 and NAME2");
   }

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #25 $
//
// Copyright 2009-2016 ECMWF.
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
#include "File.hpp"
#include "DurationTimer.hpp"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )

BOOST_AUTO_TEST_CASE( test_ECF_SCRIPT_CMD )
{
   DurationTimer timer;
   cout << "Test:: ...test_ECF_SCRIPT_CMD " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocatable
   // suite test_ECF_SCRIPT_CMD
   //   family family
   //     task check
   //         edit ECF_SCRIPT_CMD "cat $ECF_HOME/test_ECF_SCRIPT_CMD/family/check.ecf"
   //     task t1
   //         trigger check == complete
   //         edit ECF_SCRIPT_CMD "cat $ECF_HOME/test_ECF_SCRIPT_CMD/family/t1.ecf"
   //   endfamily
   // endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_ECF_SCRIPT_CMD");
      family_ptr fam = suite->add_family("family");
      fam->addVerify( VerifyAttr(NState::COMPLETE,1) );

      task_ptr task_check = fam->add_task("check");
      task_check->addVerify( VerifyAttr(NState::COMPLETE,1) );
      task_check->add_variable("ECF_SCRIPT_CMD","cat " + TestFixture::smshome() + task_check->absNodePath() + File::ECF_EXTN());

      task_ptr task_t1 = fam->add_task("t1");
      task_t1->add_trigger( "check == complete");
      task_t1->addVerify( VerifyAttr(NState::COMPLETE,1) );
      task_t1->add_variable("ECF_SCRIPT_CMD","cat " + TestFixture::smshome() + task_t1->absNodePath() + File::ECF_EXTN());
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_ECF_SCRIPT_CMD.def") );

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This is used to INVOKE a SINGLE test. Easier for debugging
//============================================================================
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "ClientToServerCmd.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite  )

BOOST_AUTO_TEST_CASE( test_file_cmd )
{
   DurationTimer timer;
   cout << "Test:: ...test_file_cmd " << flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocate-able
   //suite test_file_cmd
   // edit SLEEPTIME 1
   // edit ECF_INCLUDE $ECF_HOME/includes
   // family family
   //    task t1
   //    task t2
   //  endfamily
   //endsuite

   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_file_cmd");
      suite->addVerify( VerifyAttr(NState::COMPLETE,1) );
      family_ptr fam = suite->add_family("family");
      int taskSize = 2; // on linux 1024 tasks take ~4 seconds for job submission
      for(int i=0; i < taskSize; i++) {
         task_ptr task = fam->add_task("t" +  boost::lexical_cast<std::string>(i));
         task->addVerify( VerifyAttr(NState::COMPLETE,1) );
      }
   }

   // The test harness will create corresponding directory structure
   // and populate with standard ecf files AND will generate man files for node containers
   ServerTestHarness serverTestHarness(false/*do log file verification*/);
   serverTestHarness.generateManFileForNodeContainers();
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_file_cmd.def"));

   // Now invoke the file command to extract <ecffile,job file,job output, manual >
   // If the requests succeeded the client needs to create a temporary file, and populate it with
   // the contents of the string returned from the server
   TestFixture::client().set_throw_on_error(false);

   std::vector<Node*> nodeVec;
   theDefs.getAllNodes(nodeVec);

   BOOST_FOREACH(Node* node, nodeVec) {

      string nodePath = node->absNodePath();
      std::vector<CFileCmd::File_t> fileTypesVec = CFileCmd::fileTypesVec();
      for(size_t i = 0; i < fileTypesVec.size(); i++) {

         string file_type = CFileCmd::toString(fileTypesVec[i]);
         std::vector<std::string> theArgs = CtsApi::file(nodePath,file_type,"10000");
         std::string args; for(size_t x = 0; x < theArgs.size(); x++) { args += theArgs[x]; args += " "; }

         int theResult =  TestFixture::client().file(nodePath,file_type,"10000");
//         cout << "nodePath = " << nodePath << " fileType = " << file_type << " args passed = " << args << " pass = " << theResult << "\n";

         /// Expect KILL and STAT file types to fail, i.e since we have not called those commands
         if (fileTypesVec[i] == CFileCmd::KILL || fileTypesVec[i] == CFileCmd::STAT) {
            BOOST_CHECK_MESSAGE(  theResult == 1,args << " Expected " << CFileCmd::toString(fileTypesVec[i]) << " to fail");
            continue;
         }

         if (node->isSubmittable() || fileTypesVec[i] == CFileCmd::MANUAL) {
            // For suite and families only manual is valid
            BOOST_CHECK_MESSAGE(  theResult == 0,args << " failed for Node should return 0.\n" << TestFixture::client().errorMsg());
            BOOST_CHECK_MESSAGE(  !TestFixture::client().get_string().empty()," file contents empty for  " << args );
         }
         else {
            // Should fail
            BOOST_CHECK_MESSAGE( theResult != 0, args << " Expected failure for node " << node->debugNodePath() << " with file= " << CFileCmd::toString(fileTypesVec[i]) );
            if (theResult == 0 ) {
               std::cout << "Found file contents:''\n";
               std::cout << TestFixture::client().get_string();
               std::cout << "''\n";
            }
         }
      }
   }

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()


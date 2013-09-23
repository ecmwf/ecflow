#define BOOST_TEST_MODULE TestSmsEcf
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
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
#include <iostream>
#include <fstream>
#include <sys/stat.h> // for chmod

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "TestHelper.hpp"
#include "PrintStyle.hpp"
#include "InvokeServer.hpp"
#include "Str.hpp"

#include "EcfSmsCompare.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

/// The test file for the old sms file are generated using:
// # sms_start
// # cdp
// #                          // login into the server. This may be automatic
// # play -r /test test.def
// #                          // Wait for the suite to finish. Can observer with xcdp
// # status ; ls -NRvdV /test > test.hybrid.status

BOOST_AUTO_TEST_SUITE( TestSmsEcf )

static int called = 0;

static void reportProgress(const defs_ptr& serverDefs)
{
   // Give progress on task. Don't report the same task state more than once
   // Don't use pointer since they will not be unique. i.e a new set is retrieved from the server each time
   static set<std::string> completedTasks;
   static set<std::string> submittedTasks;
   static set<std::string> activeTasks;
   std::vector<Task*> tasks;
   serverDefs->getAllTasks(tasks);
   BOOST_FOREACH(Task* t, tasks) {
      if (t->state() == NState::SUBMITTED ) {
         if (submittedTasks.find(t->absNodePath()) != submittedTasks.end()) continue;
         submittedTasks.insert(t->absNodePath());
         BOOST_TEST_MESSAGE("..." << t->debugNodePath() << " submitted");
      }
      else if (t->state() == NState::ACTIVE ) {
         if (activeTasks.find(t->absNodePath()) != activeTasks.end()) continue;
         activeTasks.insert(t->absNodePath());
         BOOST_TEST_MESSAGE("......" << t->debugNodePath() << " active");
      }
      else if (t->state() == NState::COMPLETE ) {
         if (completedTasks.find(t->absNodePath()) != completedTasks.end()) continue;
         completedTasks.insert(t->absNodePath());
         BOOST_TEST_MESSAGE("........." << t->debugNodePath() << " complete");
      }
      else if (t->state() == NState::ABORTED ) {
         BOOST_TEST_MESSAGE("... " << t->debugNodePath() << " " << NState::toString(t->state()) << " " << t->abortedReason());
      }
      else {
         if (called > 8) {
            std::vector<std::string> reasonWhy;
            t->bottom_up_why(reasonWhy);
            for(size_t i = 0; i < reasonWhy.size(); i++) {
               if (i == 0) BOOST_TEST_MESSAGE("... waiting because: " <<  reasonWhy[i]);
               else        BOOST_TEST_MESSAGE("                     " <<  reasonWhy[i]);
            }
         }
      }
   }
   called++;
}

static bool terminateTest(const defs_ptr& serverDefs, const std::vector< std::pair<std::string,NState::State > >& terminationCriteria)
{
   reportProgress(serverDefs);

   if (terminationCriteria.empty()) {

      // Use default, ie test completes when all suites are complete.
      // Hence test.def should only contain one suite at a time
      size_t completeSuiteCnt = 0;
      BOOST_FOREACH(suite_ptr s, serverDefs->suiteVec()) { if (s->state() == NState::COMPLETE) completeSuiteCnt++;}
      return (serverDefs->suiteVec().size() == completeSuiteCnt);
   }

   for(size_t i = 0; i < terminationCriteria.size(); i++) {
      node_ptr node = serverDefs->findAbsNode(terminationCriteria[i].first);
      BOOST_REQUIRE_MESSAGE(node.get(),"Invalid node specified for termination test. Path " << terminationCriteria[i].first << " can not be found" );
      if (node->state() != terminationCriteria[i].second) return false;
   }
   return true;
}

static void doTest(const std::string& suiteName,
         const std::string& defsFile,
         const std::string& comparisonFile,
         const std::vector< std::pair<std::string,NState::State > >& terminationCriteria)
{
   // Bomb out early if the comparison file does not exist
   BOOST_REQUIRE_MESSAGE(fs::exists(comparisonFile),comparisonFile << " does not exist");

   // make sure user can locate that data on different platforms by modifying
   // location of ECF_HOME/ ECF_INCLUDES in the defs file.
   std::string filePath;
   {
      filePath = File::test_data("TestEcfSms/test/data/","TestEcfSms") + defsFile;
      std::string smshome = File::test_data("TestEcfSms/test/data","TestEcfSms");
      std::string smsinslude = File::test_data("TestEcfSms/test/data/includes","TestEcfSms");

      BOOST_TEST_MESSAGE("...Modifying " << filePath << " replacing ECF_HOME and ECF_INCLUDE");

      // Hard coded. Replace ECF_HOME/ ECF_INCLUDES to avoid hard paths, allowing test to
      // run on multiple platforms.
      int count = 0;
      std::vector<std::string> lines;
      BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(filePath,lines),"Could not open file " << filePath);
      for(size_t i = 0; i < lines.size(); i++) {
         if (lines[i].empty()) continue;
         std::vector< std::string > lineTokens;
         Str::split(lines[i], lineTokens);
         if (lineTokens.size() != 3) continue;
         if (lineTokens[0].find("edit") != std::string::npos) {
            if (lineTokens[1].find(Str::ECF_HOME()) != std::string::npos) {
               if (Str::replace(lines[i],lineTokens[2],smshome)) count++;
            }
            if (lineTokens[1].find(Str::ECF_INCLUDE()) != std::string::npos) {
               if (Str::replace(lines[i],lineTokens[2],smsinslude)) count++;
            }
         }
         if (count == 2) break;
      }
      BOOST_REQUIRE_MESSAGE(count == 2,"Expected to replace ECF_HOME/ECF_INCLUDE but replaced " << count << " times");

      // rewrite file with the changes
      std::string errorMsg;
      BOOST_REQUIRE_MESSAGE(File::create(filePath,  lines,  errorMsg), errorMsg);
   }

   // Invoke the server in the background, and terminate server at the end of the test
   // This will remove check pt and backup file before server start, to avoid the server from loading previous test data
   InvokeServer invokeServer("...Invoking server",ClientEnvironment::portSpecified());

   // We need to replace smsinit,smscomplete with the path to ecf client.
   // The job submission will look for ECF_CLIENT, if it exists we do the replacement
   std::string clientPath =  File::find_ecf_client_path();
   BOOST_REQUIRE_MESSAGE( !clientPath.empty(), "Could not find path to client executable");
   BOOST_REQUIRE_MESSAGE( fs::exists(clientPath), clientPath << " does not exist");
   std::vector<std::pair<std::string,std::string> > serverEnv; serverEnv.reserve(1);
   serverEnv.push_back( std::make_pair(std::string("ECF_CLIENT"),clientPath) );

   BOOST_TEST_MESSAGE("...start server polling on port " << invokeServer.port());
   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   theClient.setEnv(serverEnv);
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0," should return 0 server not started, or connection refused\n" << theClient.errorMsg());

   BOOST_TEST_MESSAGE("...load the defs  " << filePath << " into the server");
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(filePath) == 0, "load defs failed \n" << theClient.errorMsg());

   BOOST_TEST_MESSAGE("...Begin the suite " << CtsApi::begin(suiteName));
   BOOST_REQUIRE_MESSAGE( theClient.begin(suiteName) == 0, CtsApi::begin(suiteName) << " failed should return 0\n" << theClient.errorMsg());

   BOOST_TEST_MESSAGE("...wait for the test to finish");
   int sleepTime = 10;
   while( 1 ) {
      sleep(sleepTime);   // avoid calling get excessively, by using sleep

      BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0 " << theClient.errorMsg());
      defs_ptr serverDefs =  theClient.defs();
      BOOST_REQUIRE_MESSAGE( serverDefs.get(),"get command failed to get node tree from server");

#ifdef DEBUG_DEFS
      cout << "Printing Defs \n";
      PrintStyle style(PrintStyle::STATE);
      std::cout << *serverDefs.get();
#endif

      if (terminateTest(serverDefs,terminationCriteria)) {

         // We have completed, test the state of the node tree against the old ECF_ run.
         // We look for the old state in a file named <suite>.status
         EcfSmsCompare theComparer(comparisonFile);
         string localErrorMessage;
         BOOST_REQUIRE_MESSAGE(theComparer.run(serverDefs,localErrorMessage),localErrorMessage);
         break;
      }
   }
}

BOOST_AUTO_TEST_CASE( test_compare_sms_with_ecf_real )
{
   // Use --log_level=message to see BOOST_TEST_MESSAGE output
   BOOST_TEST_MESSAGE("TestSmsEcf::...  test_compare_sms_with_ecf_real");
   called = 0;

   // *************************************************************************
   // IMPORTANT: The code below ONLY work because the server is started LOCALLY
   // and hence has same NOTION of the CWD, if the server is started in a seperate
   // process that does not match this CWD then it will NOT be able to locate
   // data (i.e ECF_ jobs file, etc).
   // ***************************************************************************

   std::string comparisonFile = File::test_data("TestEcfSms/test/data/test.real.status","TestEcfSms");
   std::string test_real_def = File::test_data("TestEcfSms/test/data/test_real.def","TestEcfSms");
   std::string test_real_def_copy = File::test_data("TestEcfSms/test/data/test_real.def_copy","TestEcfSms");
   fs::remove(test_real_def.c_str());

   // We need to copy since we will be modifying the defs file
   // cant use def directly since this is readonly in the perforce repository
   fs::copy_file(test_real_def_copy,test_real_def);
   BOOST_CHECK_MESSAGE(chmod( test_real_def.c_str(), 0777 ) == 0," Coud not make file " << test_real_def << " writeable ");

   std::string suiteName = "test";
   std::string defsFile = "test_real.def";

   // Family /test/f2 is queued due to time dependency
   std::vector< std::pair<std::string,NState::State > > terminationCriteria;
   terminationCriteria.push_back( std::make_pair(string("/test/f1"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f2"), NState::QUEUED));
   terminationCriteria.push_back( std::make_pair(string("/test/f2/t4"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f3"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f4"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f5"), NState::COMPLETE));

   doTest(suiteName, defsFile, comparisonFile, terminationCriteria );

   // remove file that was copied
   fs::remove(test_real_def.c_str());
}

BOOST_AUTO_TEST_CASE( test_compare_sms_with_ecf_hybrid )
{
   BOOST_TEST_MESSAGE("TestSmsEcf::...  test_compare_sms_with_ecf_hybrid");
   called = 0;

   // *************************************************************************
   // IMPORTANT: The code below ONLY work because the server is started LOCALLY
   // and hence has same NOTION of the CWD, if the server is started in a seperate
   // process that does not match this CWD then it will NOT be able to locate
   // data (i.e ECF_ jobs file, etc).
   // ***************************************************************************

   std::string comparisonFile = File::test_data("TestEcfSms/test/data/test.hybrid.status","TestEcfSms");
   std::string hybrid_def = File::test_data("TestEcfSms/test/data/test_hybrid.def","TestEcfSms");
   std::string hybrid_def_copy = File::test_data("TestEcfSms/test/data/test_hybrid.def_copy","TestEcfSms");
   fs::remove(hybrid_def.c_str());
   fs::copy_file(hybrid_def_copy,hybrid_def);
   BOOST_CHECK_MESSAGE(chmod( hybrid_def.c_str(), 0777 ) == 0," Could not make file " << hybrid_def << " writeable ");

   std::string suiteName = "test";
   std::string defsFile = "test_hybrid.def";

   // Family /test/f2 is queued due to time dependency
   std::vector< std::pair<std::string,NState::State > > terminationCriteria;
   terminationCriteria.push_back( std::make_pair(string("/test/f1"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f2"), NState::QUEUED));
   terminationCriteria.push_back( std::make_pair(string("/test/f2/t4"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f3"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f4"), NState::COMPLETE));
   terminationCriteria.push_back( std::make_pair(string("/test/f5"), NState::COMPLETE));

   doTest(suiteName, defsFile, comparisonFile, terminationCriteria );

   // remove file that was copied.
   fs::remove(hybrid_def.c_str());
}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #127 $ 
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
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestVerification.hpp"
#include "TestFixture.hpp"
#include "LogVerification.hpp"
#include "DurationTimer.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "AssertTimer.hpp"
#include "TestHelper.hpp"
#include "PrintStyle.hpp"
#include "WhyCmd.hpp"
#include "Ecf.hpp"
#include "ClientInvoker.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

//#define DEBUG_TEST_HARNESS 1
//#define DEBUG_DEFS 1
//#define DEBUG_DIFF 1

ServerTestHarness::ServerTestHarness( bool doLogFileVerification, bool standardVerification   )
: doLogFileVerification_(doLogFileVerification),
  standardVerification_(standardVerification),
  generateManFileForNodeContainers_(false),
  check_task_duration_less_than_server_poll_(true),
  add_default_sleep_time_(true),
  serverUpdateCalendarCount_(0)
{}

void ServerTestHarness::run(
         Defs& theClientDefs,
         const std::string& defs_filename,
         const std::map<std::string,
         std::string>& customTaskSmsMap,
         int timeout,
         bool waitForTestCompletion)
{
   defs_filename_ = defs_filename;

   /// RUN the TEST
   defs_ptr serverDefs = doRun(theClientDefs,customTaskSmsMap,timeout,waitForTestCompletion);

   /// CHECK the test
   if ( doLogFileVerification_ ) {

      // Locate log file for analysis and comparison
      string newLog = TestVerification::locate_log_file(defs_filename);

      if ( standardVerification_ ) {
         /// The standard test verification will check in the log file that each task goes through the
         /// normal life cycle changes, and then compares the state changes to a golden reference log file
         TestVerification::standardVerification(newLog,theClientDefs);
      }
      else {
         // Does not follow the normal life cycle changes
         // hence we will only compare with golden reference log file.
         std::string errorMsg;
         BOOST_CHECK_MESSAGE(LogVerification::compareNodeStates(newLog,ServerTestHarness::goldenTestDataLocation(newLog),errorMsg),errorMsg);
      }
   }
}

void  ServerTestHarness::run(Defs& defs,const std::string& defs_filename, int timeout, bool waitForTestCompletion)
{
   std::map<std::string,std::string> customTaskSmsMap;
   run(defs,defs_filename,customTaskSmsMap,timeout,waitForTestCompletion);
}

struct null_deleter { void operator()(void const *) const{} };

defs_ptr
ServerTestHarness::doRun(Defs& theClientDefs, const std::map<std::string,std::string>& customTaskSmsMap,int timeout,bool waitForTestCompletion)
{
#ifdef DEBUG_TEST_HARNESS
   cout << "ServerTestHarness::doRun " << defs_filename_ << " timeout=" << timeout << "  waitForTestCompletion = " << waitForTestCompletion << "\n";
#endif
   BOOST_REQUIRE_MESSAGE(!theClientDefs.suiteVec().empty(), "No suite defined");


#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Get the client exe. This can be client exe on another platform hence cant run here\n";
#endif
   std::string theClientExePath = TestFixture::theClientExePath();
   BOOST_REQUIRE_MESSAGE(!theClientExePath.empty()," The client program could not be found");


#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Create ECF directory structure corresponding to Node tree\n";
#endif
   // Automatically add variable ECF_HOME to each suite, before saving to disk
   // This is needed to locate ECF_FILE includes
   std::string ecf_home = testDataLocation( defs_filename_ );

   // Most test just define one suite, however the mega.def has up to 26 suites
   // If set this can be used to choose which suite to begin.
   std::string suiteName;

   // Create the client early, since we need to determine if enviroment variable
   // ECF_ALLOW_NEW_CLIENT_OLD_SERVER was set, if it was, we need to update the generated .ecf scripts
   // to embed this variable, this will allow  *new child commands* to talk to old servers
   int allow_new_client_old_server = 0;
   ClientInvoker theClient ;
   if (theClient.allow_new_client_old_server() != 0) {
      // need export ECF_ALLOW_NEW_CLIENT_OLD_SERVER
      allow_new_client_old_server = theClient.allow_new_client_old_server();
   }


   // ECF_CLIENT_EXE_PATH allows dependence on client exe without installation
   // Allow user to add SLEEPTIME, otherwise add a default
   int customSmsCnt = 0;
   int taskSmsMapSize = static_cast<int>(customTaskSmsMap.size());
   BOOST_FOREACH(suite_ptr s, theClientDefs.suiteVec()) {
      if (allow_new_client_old_server != 0) {
         std::string value = "export ECF_ALLOW_NEW_CLIENT_OLD_SERVER=" + boost::lexical_cast<std::string>(allow_new_client_old_server);
         s->addVariable( Variable( "ECF_ALLOW_NEW_CLIENT_OLD_SERVER",  value ) );
      }
      if (s->findVariable(Str::ECF_HOME()).empty())         s->addVariable( Variable( Str::ECF_HOME(),  ecf_home ) );
      if (s->findVariable("ECF_CLIENT_EXE_PATH").empty()) s->addVariable( Variable( "ECF_CLIENT_EXE_PATH",  theClientExePath ) );
      if (s->findVariable(Str::ECF_INCLUDE()).empty())      s->addVariable( Variable( Str::ECF_INCLUDE(), TestFixture::includes()  ) );
      if (s->findVariable("SLEEPTIME").empty())           s->addVariable( Variable( "SLEEPTIME", "1" ) );

      if (check_task_duration_less_than_server_poll_) {
         if (s->findVariable("CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL").empty()) s->addVariable( Variable( "CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL", "_any_" ) );
      }
      suiteName = s->name();

      // recursively create directory structure from ECF_HOME and populate tasks with sms files
      createDirAndEcfFiles(s.get(),ecf_home,customTaskSmsMap,customSmsCnt);
   }
   BOOST_REQUIRE_MESSAGE( customSmsCnt == taskSmsMapSize,"customSmsCnt:" << customSmsCnt << " does not match " << taskSmsMapSize << " createDirAndEcfFiles did not create all sms file corresponding to tasks");


   // If the defs has more than one suite, then start them all.
   if ( theClientDefs.suiteVec().size() != 1) {
      suiteName.clear();
#ifdef DEBUG_TEST_HARNESS
      cout << "   ServerTestHarness::doRun: defs has " << theClientDefs.suiteVec().size() << " suites hence will begin all of them\n";
#endif
   }


   bool load_defs_from_disk = true;
   {
#ifdef DEBUG_TEST_HARNESS
      cout << "   ServerTestHarness::doRun: Save the Defs file to disk. " << defs_filename_ << "\n";
#endif
      std::ofstream theClientDefsFile( defs_filename_.c_str() );
      if ( theClientDefsFile.fail()) {
         // The file is *not on disk*, just use in memory defs
         load_defs_from_disk = false;
      }
      else {
         PrintStyle::setStyle(PrintStyle::DEFS); // needed for output
         theClientDefsFile << theClientDefs;
         PrintStyle::setStyle(PrintStyle::STATE); // From now on show state
      }
   }

   // Set the location of the new log file. Close the current log file and create new log file
   // The defs file name should have been set to the test location
   // The log file is CLEARED so that previous run is ignored
   std::string new_log_file_path = defs_filename_ + "_log";
#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::new_log_file_path = " << new_log_file_path << "\n";
#endif
   theClient.new_log( new_log_file_path );
   theClient.clearLog();


#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Delete all nodes in server. Using force to allow as many tests as possible\n";
#endif
   BOOST_REQUIRE_MESSAGE(theClient.delete_all(true/*force, even if it creates zombies*/) == 0,CtsApi::to_string(CtsApi::delete_node()) << " failed should return 0. Should Delete ALL existing defs in the server\n" << theClient.errorMsg());


#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Load new defs in the server\n";
#endif
   if ( load_defs_from_disk ) {
      BOOST_REQUIRE_MESSAGE(theClient.loadDefs( defs_filename_ ) == 0,"load defs failed should return 0. Should Load defs file " << defs_filename_ << " into the server from current working directory\n" << theClient.errorMsg());
   }
   else {
      // load expects a defs_ptr
      defs_ptr defs(&theClientDefs,null_deleter());
      BOOST_REQUIRE_MESSAGE(theClient.load( defs ) == 0,"load defs failed should return 0. Should Load defs file " << defs_filename_ << " into the server\n" << theClient.errorMsg());
   }

#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Restart server \n";
#endif
   BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,CtsApi::restartServer() << " failed should return 0. Should restart the server via a client command\n" << theClient.errorMsg());


#ifdef DEBUG_TEST_HARNESS
   cout << "   ServerTestHarness::doRun: Calling begin on suite " << suiteName << "\n";
#endif
   BOOST_REQUIRE_MESSAGE(theClient.begin( suiteName ) == 0,CtsApi::begin( suiteName ) << " failed should return 0. Should Begin the suite " <<  suiteName << "\n" << theClient.errorMsg());


   if (waitForTestCompletion) {
#ifdef DEBUG_TEST_HARNESS
      cout << "   ServerTestHarness::doRun: Waiting for test to finish\n";
#endif
      return testWaiter(theClient,theClientDefs,timeout,true /* test against verification attributes on defs */);
   }
   return defs_ptr();
}


static void test_invariants(defs_ptr the_defs, const std::string& title) {
   std::string errorMsg;
   BOOST_CHECK_MESSAGE( the_defs->checkInvariants(errorMsg),title << " Invariants failed " << errorMsg);
}

defs_ptr
ServerTestHarness::testWaiter(const ClientInvoker& theClient,
         const Defs& theClientDefs,
         int timeout,
         bool verifyAttr)
{
#ifdef DEBUG_TEST_HARNESS
   cout << "ServerTestHarness::testWaiter \n";
#endif

#ifdef DEBUG_DIFF
   std::string dump_defs_filename = "test.def";
   int counter = 0;
#endif

   BOOST_REQUIRE_MESSAGE(theClient.client_handle() == 0,"Client handle expected to be zero but found " << theClient.client_handle());

   /// This function will test sync'ing. by getting the sync and full defs, then comparing them
   AssertTimer assertTimer(timeout,false); // Bomb out after n seconds, fall back if test fail

   // test the incremental changes
   defs_ptr incremental_defs;
   defs_ptr full_defs;

   int sleepTime = (theClientDefs.suiteVec().size() == 1) ? TestFixture::job_submission_interval() : 10;
   int sleep_fudgeFactor = TestFixture::job_submission_interval();

   // How do we terminate this test?
   // The test succeeds if the suite state is complete,
   while( 1 ) {
  		//std::cout << "sleepTime = " << sleepTime << "\n";
      sleep(sleepTime);    // avoid calling get excessively, by using sleep
      DurationTimer timer; // automatically adjust sleep time

      bool server_changed = false;
      if (!full_defs.get()) {
         server_changed = true;
         // ********************************************************************************
         // Calling getDefs will also force a flush of the log file. This is required since we
         // copy the log file locally on the shared file system for cross platform testing
         // ******************************************************************************
         BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0 " << theClient.errorMsg());
         full_defs =  theClient.defs();
         incremental_defs = theClient.defs();
         BOOST_REQUIRE_MESSAGE( full_defs.get(),"get command failed to get node tree from server");
         BOOST_REQUIRE_MESSAGE( incremental_defs.get(),"get command failed to get node tree from server");

         // Ensure that when suite was loaded in server, that others suites were discarded
         BOOST_CHECK_MESSAGE(theClientDefs.suiteVec().size() == full_defs->suiteVec().size(),"mismatch in client suite count " << theClientDefs.suiteVec().size() << " and server " << full_defs->suiteVec().size());
         test_invariants(full_defs,"First time for getting full defs");
      }
      else {
         BOOST_REQUIRE_MESSAGE(theClient.news(full_defs) == 0, "news failed should return 0 " << theClient.errorMsg());
         server_changed = theClient.get_news();
         //			std::cout << "server_changed = " << server_changed << "\n";
         if ( server_changed ) {

            // Get the incremental changes **FIRST** and compare this with the full defs later on
            BOOST_REQUIRE_MESSAGE(theClient.sync(incremental_defs) == 0, "sync failed should return 0 " << theClient.errorMsg());
            BOOST_CHECK_MESSAGE(theClient.in_sync(), "in_sync expected to return true, when we have changes in server");
            test_invariants(incremental_defs,"After syncing with incremental defs.");

            // get the FULL defs
            BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,CtsApi::get() << " failed should return 0 " << theClient.errorMsg());
            full_defs =  theClient.defs();
            BOOST_REQUIRE_MESSAGE( full_defs.get(),"get command failed to get node tree from server");
            test_invariants(full_defs,"After getting the full defs.");

            // **** NOTE ****: There could have been state change between the calls:
            // **************: 	  theClient.sync(incremental_defs)
            // **************:    theClient.getDefs()
            // **************: hence we can only compare, incremental and full defs if
            // **************: state and modification numbers are the same:
            BOOST_REQUIRE_MESSAGE(incremental_defs.get() != full_defs.get()," Expected two different defs trees ");
            if ( incremental_defs->state_change_no()  == full_defs->state_change_no() &&
                     incremental_defs->modify_change_no() == full_defs->modify_change_no()) {
               Ecf::set_debug_equality(true);  // only has affect in DEBUG build
               BOOST_CHECK_MESSAGE( *full_defs == *incremental_defs,
                        "Full and incremental defs should be the same. (state_change_no,modify_change_no) ("
                        << incremental_defs->state_change_no() << "," << incremental_defs->modify_change_no() << ")"
               );
               Ecf::set_debug_equality(false); // only has affect in DEBUG build
            }
            else {
               // std::cout << "***** ServerTestHarness::testWaiter: State change between getting incremental and full defs\n";
            }
         }
      }

      int hasAutoCancel = 0;
      size_t completeSuiteCnt = 0;
      if (server_changed) {

#ifdef DEBUG_DIFF
         {
            PrintStyle::Type_t st = PrintStyle::getStyle();
            PrintStyle::setStyle(PrintStyle::STATE);
            counter++;
            std::string filename = dump_defs_filename + boost::lexical_cast<std::string>(counter);
            std::ofstream theFile( filename.c_str() );

            std::vector<Task*> tasks;
            full_defs->getAllTasks(tasks);
            int unknown = 0;
            int complete = 0;
            int queued = 0;
            int aborted = 0;
            int submitted = 0;
            int active = 0;
            size_t password_size = 0;
            size_t process_id_size = 0;
            for(size_t i =0; i < tasks.size(); i++) {
               switch (tasks[i]->state()) {
                  case NState::UNKNOWN: unknown++; break;
                  case NState::COMPLETE: complete++; break;
                  case NState::QUEUED: queued++; break;
                  case NState::ABORTED: aborted++; break;
                  case NState::SUBMITTED: submitted++; break;
                  case NState::ACTIVE: active++; break;
               }
               password_size += tasks[i]->jobsPassword().size();
               process_id_size += tasks[i]->process_or_remote_id().size();
            }
            theFile << "task password_size(" << password_size << ") process_id_size(" << process_id_size << ")   total = " << (password_size + process_id_size) << endl;
            theFile << "unknown: " << unknown << "\n";
            theFile << "complete: " << complete << "\n";
            theFile << "queued: " << queued << "\n";
            theFile << "aborted: " << aborted << "\n";
            theFile << "submitted: " << submitted << "\n";
            theFile << "active: " << active << "\n";
            theFile << *full_defs.get();
            PrintStyle::setStyle(st);
         }
#endif
         std::string errorMsg;
         BOOST_REQUIRE_MESSAGE( full_defs->checkInvariants(errorMsg),"Invariants failed " << errorMsg);

         // record the number of times that the server updated the calendar. Allow debug of time dependencies
         serverUpdateCalendarCount_ = full_defs->updateCalendarCount();

         BOOST_FOREACH(suite_ptr s, full_defs->suiteVec()) {
            if (s->state() == NState::COMPLETE) completeSuiteCnt++;
            if (s->hasAutoCancel()) hasAutoCancel++;
         }

#ifdef DEBUG_DEFS
         cout << "Printing Defs \n";
         std::cout << *full_defs.get();
         cout << "completeSuiteCnt = " << completeSuiteCnt
                  << " full_defs->suiteVec().size() = " <<  full_defs->suiteVec().size()
                  << " hasAutoCancel = " << hasAutoCancel << "\n";
#endif
         if ( (full_defs->suiteVec().size() == completeSuiteCnt)  && (hasAutoCancel == 0)) {

            if ( verifyAttr ) {
               // Do verification of expected state changes
               string localErrorMessage;
               BOOST_REQUIRE_MESSAGE(full_defs->verification(localErrorMessage),localErrorMessage << "\n" << *full_defs.get());
            }
            return full_defs;
         }
      }

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         // Give clues why we are not finishing on time, by using Why and by dumping out node tree
         cout << "Test time " << assertTimer.duration() << " taking longer than time constraint of " << assertTimer.timeConstraint() << " aborting\n";
         cout << "completeSuiteCnt = " << completeSuiteCnt << " full_defs->suiteVec().size() = " <<  full_defs->suiteVec().size() << " hasAutoCancel = " << hasAutoCancel << "\n";
         std::cout << "update-calendar-count(" << serverUpdateCalendarCount_ << ")\n";
         std::cout << "WHY:\n";
         WhyCmd reason(full_defs, "" /* do a top down why */ );
         std::cout << reason.why() << "\n";
      }
      BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),*full_defs);
      if ( assertTimer.duration() >= assertTimer.timeConstraint()) break; // fix warning on AIX

      // auto adjust sleep time.
      sleepTime = timer.duration() + sleep_fudgeFactor;
   }
   return defs_ptr();
}


void ServerTestHarness::createDirAndEcfFiles(
         NodeContainer* nc,
         const std::string& smshome,
         const std::map<std::string,std::string>& customTaskSmsMap,
         int& customSmsCnt) const
{
   std::string directory =  smshome + nc->absNodePath();
#ifdef DEBUG_TEST_HARNESS
   cout << "creating directory  " << directory << "\n";
#endif

   if ( !fs::exists( smshome ) )    fs::create_directory( smshome );
   if ( !fs::exists( directory ) )  fs::create_directory( directory );

   if ( generateManFileForNodeContainers_ ) {
      std::string manFileName = smshome + nc->absNodePath() + File::MAN_EXTN();
      std::ofstream theManFile( manFileName.c_str() );
      theManFile << "%manual\n";
      theManFile << "This file auto generated by ServerTestHarness::createDirAndEcfFiles for all Node Containers\n";
      theManFile << "%end\n";
   }

   BOOST_FOREACH(node_ptr n, nc->nodeVec()) {

      Task* t = n->isTask();
      if (t) {
         std::string ecf_file =  smshome + t->absNodePath() + File::ECF_EXTN();
#ifdef DEBUG_TEST_HARNESS
         cout << "creating ecf file  " << ecf_file << "\n";
#endif
         // Create ECF file with default template or custom  file.
         std::ofstream theEcfFile( ecf_file.c_str() );
         std::map<std::string,std::string>::const_iterator it = customTaskSmsMap.find(t->absNodePath());
         if (it == customTaskSmsMap.end()) theEcfFile << getDefaultTemplateEcfFile(t);
         else  {
            theEcfFile << (*it).second;
            customSmsCnt++;
         }
      }
      else {
         Family* f = n->isFamily();
         assert(f);
         createDirAndEcfFiles(f,smshome,customTaskSmsMap,customSmsCnt);
      }
   }
}


std::string ServerTestHarness::getDefaultTemplateEcfFile(Task* t) const
{
   std::string templateEcfFile;

   templateEcfFile += "%manual\n";
   templateEcfFile += "This is the default ecf script file used for testing\n";
   templateEcfFile += "%end\n";
   templateEcfFile += "%comment\n";
   templateEcfFile += "# Using angle brackets should use ECF_INCLUDE\n";
   templateEcfFile += "%end\n";
   templateEcfFile += "%include <head.h>\n";
   templateEcfFile += "\n";
   BOOST_FOREACH(const Event& e, t->events()) {
      templateEcfFile += "%ECF_CLIENT_EXE_PATH% --event=" + e.name_or_number() + "\n";
   }
   BOOST_FOREACH(const Meter& m, t->meters()) {
      templateEcfFile += "for i in";
      int min = m.min();
      int max = m.max();
      int delta = abs(max -min)/10;
      for(int i = min+delta; i <= max; i = i + delta) {
         templateEcfFile += " ";
         templateEcfFile += boost::lexical_cast<std::string>(i);
      }
      templateEcfFile += "\n";
      templateEcfFile += "do\n";
      templateEcfFile += "   %ECF_CLIENT_EXE_PATH% --meter=" + m.name() + " $i\n";
      templateEcfFile += "   sleep %SLEEPTIME%\n";
      templateEcfFile += "done\n";
   }
   /// labels require at least 2 arguments,
   BOOST_FOREACH(const Label& label, t->labels()) {
      if (!label.new_value().empty()) {
         templateEcfFile += "%ECF_CLIENT_EXE_PATH% --label=" + label.name() + " " + label.new_value() + "\n";
      }
      else if (!label.value().empty()) {
         templateEcfFile += "%ECF_CLIENT_EXE_PATH% --label=" + label.name() + " " + label.value() + "\n";
      }
   }
   if (add_default_sleep_time_ && t->events().empty() && t->meters().empty()) {
      templateEcfFile += "\n# SLEEPTIME is defined the Client Defs.def file. Test variable substitution\n";
      templateEcfFile += "sleep %SLEEPTIME%\n";
   }
   templateEcfFile += "\n%include <tail.h>\n";
   return templateEcfFile;
}

std::string ServerTestHarness::testDataDefsLocation( const std::string& defsFile)
{
   // DefsFile is of the form base_name.def"
   // We want to place the defs and log file in the same location as its test directory
   // 		test/data/ECF_HOME/base_name/base_name.def
   std::string testData = testDataLocation(defsFile) + "/" + defsFile;
   return testData;
}

std::string ServerTestHarness::testDataLocation( const std::string& defsFile)
{
   // DefsFile is of the form:
   //                           base_name.def
   //                           /tmp/path/base_name.def
   // We want to place the defs and log file in the same location as its test directory
   // 		test/data/ECF_HOME/base_name
   //
   std::string base_name = defsFile;
   size_t slash_pos = defsFile.rfind('/',defsFile.length());
   if ( slash_pos != std::string::npos) {
      base_name = base_name.erase(0,slash_pos+1);
   }

   size_t dot_pos = base_name.rfind('.',base_name.length());
   assert( dot_pos != std::string::npos); // missing '.'
   base_name =  base_name.substr(0,dot_pos);


   std::string testData = TestFixture::smshome() + "/" + base_name;
   return testData;
}

std::string ServerTestHarness::goldenTestDataLocation( const std::string& logFile )
{
   // file is of the form Test/data/my.def_log
   //                    OR data/my.def_log
   // convert to my.def_log
   std::string leafLogFile = logFile;
   size_t lastSlash = logFile.find_last_of("/");
   if ( lastSlash != std::string::npos) {
      leafLogFile = logFile.substr( lastSlash+1 );
   }

   std::string testData = File::test_data("Test/goldenData/","Test");
   testData += leafLogFile;
   return testData;
}

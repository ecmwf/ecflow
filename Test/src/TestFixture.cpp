//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #77 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : This Fixture facilitates the test of client/server on different platforms
//============================================================================

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdlib.h> // for getenv()
#include <fstream>  // for ofstream

#include "TestFixture.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "ClientEnvironment.hpp" // needed for static ClientEnvironment::hostSpecified(); ONLY
#include "File.hpp"
#include "TestHelper.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "PrintStyle.hpp"
#include "Host.hpp"
#include "Rtt.hpp"
#include "EcfPortLock.hpp"

#ifdef DEBUG
std::string rtt_filename = "rtt.dat";
#else
std::string rtt_filename = "rtt_d.dat";
#endif
std::string TestFixture::scratchSmsHome_ = "";
std::string TestFixture::host_;
std::string TestFixture::port_;
std::string TestFixture::test_dir_;
std::string TestFixture::project_test_dir_ = "Test";

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

// ************************************************************************************************
// For test purpose the server can be started:
// 	1/ By this test fixture
//  2/ Externally but on the same machine. by defining env variable: export ECF_NODE=localhost
// 	   WHY? To test for memory leak. (i.e. with valgrind)
//  3/ Externally but on a different platform. by defining env variable: export ECF_NODE=itanium
//     In this case we NEED to copy the test data, so that it is
//     accessible by the client AND server
//
//  When invoking the server Externally _MUST_ use   .
//			./Server/bin/gcc.<version>/debug/server --ecfinterval=2
//       	The --ecfinterval=2 is _important_ or test will take a long time
// ************************************************************************************************

// Uncomment to preserve the files for test
//#define DEBUG_HOST_SERVER 1
#define DEBUG_LOCAL_SERVER 1

TestFixture::TestFixture(const std::string& project_test_dir)
{
   init(project_test_dir);
}

TestFixture::TestFixture()
{
   init("Test");
}

ClientInvoker& TestFixture::client()
{
   static ClientInvoker theClient_;
   return theClient_;
}


void TestFixture::init(const std::string& project_test_dir)
{
   TestFixture::project_test_dir_ = project_test_dir;

   if ( !fs::exists( local_ecf_home() ) )  fs::create_directory( local_ecf_home() );

   // client side file for recording all ClientInvoker round trip times
   boost::filesystem::remove(rtt_filename);
   Rtt::create(rtt_filename);

   // ********************************************************
   // Note: Global fixture Constructor can not use BOOST macro
   // ********************************************************
   PrintStyle::setStyle(PrintStyle::STATE);

   // Let first see if we need do anything. If ECF_NODE is specified (ie the name
   // of the machine, which has the ecflow server), only then do we need to do anything.
   // The server must have access to the file system specified by ECF_HOME.
   // This becomes an issue when the server is on a different machine
   host_ = ClientEnvironment::hostSpecified();
   port_ = ClientEnvironment::portSpecified(); // returns ECF_PORT, otherwise Str::DEFAULT_PORT_NUMBER
   std::cout << "TestFixture::TestFixture()  jobSubmissionInterval = " << job_submission_interval() << "\n";
   if (!host_.empty() && host_ != Str::LOCALHOST()) {

      client().set_host_port(host_,port_);
      std::cout << "   EXTERNAL SERVER running on _ANOTHER_ PLATFORM, assuming " << host_ << ":" << port_ << " copying test data ...\n";

      // Must use a file system accessible from the server. Use $SCRATCH
      // Duplicate test data, required to scratch area and reset ECF_HOME
      char* scratchEnv = getenv("SCRATCH");
      assert(scratchEnv != NULL);
      std::string theSCRATCHArea(scratchEnv);
      assert(!theSCRATCHArea.empty());

      theSCRATCHArea += "/test_dir";
      test_dir_ = theSCRATCHArea;  // test_dir_ needed in destructor
      if (boost::filesystem::exists(test_dir_)) {
         boost::filesystem::remove_all(test_dir_);
      }
      theSCRATCHArea += "/ECF_HOME";
      scratchSmsHome_ = theSCRATCHArea;

      bool ok = File::createDirectories(theSCRATCHArea);
      BOOST_REQUIRE_MESSAGE(ok,"File::createDirectories(theSCRATCHArea) failed");
      ok = fs::exists(theSCRATCHArea); assert(ok);

      // Ensure that local includes data exists. This needs to be copied to SCRATCH
      ok = fs::exists(includes()); assert(ok);

      // Copy over the includes directory to the SCRATCH area.
      std::string scratchIncludes = test_dir_ + "/";
      std::string do_copy = "cp -r " + includes() + " " + scratchIncludes;
      if (system( do_copy.c_str() ) != 0)  assert(false);

      // clear log file
      clearLog();
   }
   else if (!host_.empty() && host_ == Str::LOCALHOST()) {

      client().set_host_port(host_,port_);
      std::cout << "   EXTERNAL SERVER running on _SAME_ PLATFORM. Assuming " << host_ << ":" << port_ << "\n";

      // log file may have been deleted, by previous tests. Create a new log file
      std::string the_log_file = TestFixture::pathToLogFile();
      if ( !fs::exists( the_log_file )) {
         std::cout << "   Log file " << the_log_file << " does NOT exist, attempting to recreate\n";
         std::cout << "   Creating new log(via remote server) file " << the_log_file  << "\n";
         client().new_log( the_log_file );
         client().logMsg("Created new log file. msg sent to force new log file to be written to disk");
      }
      else std::cout << "   Log file " << the_log_file << " already exist\n";
   }
   else {
      // For local host start by removing log file. Server invocation should create a new log file
      boost::filesystem::remove( boost::filesystem::path( pathToLogFile() ) );
      host_ = Str::LOCALHOST();

      // Create a unique port number, allowing debug and release to run at the same time
      // Note: linux64 and linux64intel, can run on same machine, on different workspace
      // Hence the lock file is not sufficient. Hence we will make a client server call.
      cout << "Find free port to start server, starting with port " << port_ << "\n";
      int the_port = boost::lexical_cast<int>(port_);
      while (!EcfPortLock::is_free(the_port)) the_port++;
      port_ = ClientInvoker::find_free_port(the_port,true /*show debug output */);
      EcfPortLock::create(port_);

      // host_ is empty update to localhost, **since** port may have changed, update ClinetInvoker
      client().set_host_port(host_,port_);

      // Remove the generated check point files, at start of test, otherwise server will load check point file
      Host h;
      boost::filesystem::remove(h.ecf_checkpt_file(port_));
      boost::filesystem::remove(h.ecf_backup_checkpt_file(port_));

      std::string theServerInvokePath = File::find_ecf_server_path();
      assert(!theServerInvokePath.empty() );
      theServerInvokePath += " --port=" +  port_ ;
      theServerInvokePath += " --ecfinterval=" + boost::lexical_cast<std::string>( job_submission_interval() );
      theServerInvokePath += "&";
      if ( system( theServerInvokePath.c_str() ) != 0)  assert(false); // " Server invoke failed "

      std::cout << "   ECF_NODE not specified, starting LOCAL " << theServerInvokePath << "\n";
   }

   /// Ping the server to see if its running
   /// Assume remote/local server started on the default port
   /// Either way, we wait for 60 seconds for server, for it to respond to pings
   /// This is important when server is started locally. We must wait for it to come alive.
   if (!client().wait_for_server_reply()) {
      cout << "Ping server on " << host_ << Str::COLON() << port_ << " failed. Is the server running ? " << client().errorMsg() << "\n";
      assert(false);
   }

   // Log file must exist, otherwise test will not work. Log file required for comparison
   if ( !fs::exists( TestFixture::pathToLogFile() )) {
      cout << "Log file " << TestFixture::pathToLogFile() << " does not exist\n";
      assert(false);
   }
}


TestFixture::~TestFixture()
{
	// Note: Global fixture Destructor can not use BOOST macro
  	std::cout << "TestFixture::~TestFixture() " << host_ << ":" << port_ << "\n";

   // destructors should not allow exception propagation
   try {
#ifndef DEBUG_HOST_SERVER
      if (!host_.empty() && boost::filesystem::exists(test_dir_)) {
         boost::filesystem::remove_all(test_dir_);
      }
#endif
#ifndef DEBUG_LOCAL_SERVER
      if (boost::filesystem::exists(local_ecf_home())) {
         boost::filesystem::remove_all(local_ecf_home());
      }
#endif

      // Print the server suites
      client().set_cli(true); // so server stats are written to standard out
      client().set_throw_on_error( false ); // destructors should not allow exception propagation
      if (client().suites() != 0) {
         std::cout << "TestFixture::~TestFixture(): ClientInvoker " << CtsApi::suites() << " failed: " << client().errorMsg() << "\n";
      }

      // Print the server stats
      if (client().stats() != 0) {
         std::cout << "TestFixture::~TestFixture(): ClientInvoker " << CtsApi::stats() << " failed: " << client().errorMsg() << "\n";
      }

      // Kill the server, as all suites are complete. will work for local or external
      if (client().terminateServer() != 0) {
         std::cout << "TestFixture::~TestFixture():  ClientInvoker " << CtsApi::terminateServer() << " failed: " << client().errorMsg() << "\n";
         EcfPortLock::remove( port_ );
         assert(false);
      }
      sleep(1); // allow time to update log file

      // Remove the generated check point files, at end of test
      Host host;
      boost::filesystem::remove(host.ecf_log_file(port_));
      boost::filesystem::remove(host.ecf_checkpt_file(port_));
      boost::filesystem::remove(host.ecf_backup_checkpt_file(port_));

      // remove the lock file
      EcfPortLock::remove( port_ );

      // destroy, so that we flush the rtt_filename
      Rtt::destroy();

      cout << "\nTiming: *NOTE*: The child commands *NOT* recorded. Since its a separate exe(ecflow_client), called via .ecf script\n";
      cout << Rtt::analyis(rtt_filename); // report round trip times
      boost::filesystem::remove(rtt_filename);
   }
 	catch (std::exception& ex) {
 	   std::cout << "TestFixture::~TestFixture() caught exception " << ex.what() << "\n";
 	}
 	catch (...) {
      std::cout << "TestFixture::~TestFixture() caught unknown exception\n";
 	}
}

int TestFixture::job_submission_interval()
{
   int jobSubmissionInterval = 3 ;
#if defined(HPUX) || defined(_AIX)
   jobSubmissionInterval += 3;
#endif
   return jobSubmissionInterval;
}

std::string TestFixture::smshome()
{
	if ( serverOnLocalMachine() )
		return local_ecf_home();
	return scratchSmsHome_;
}

bool TestFixture::serverOnLocalMachine() {
	return (host_.empty() || host_ == Str::LOCALHOST());
}

std::string TestFixture::theClientExePath()
{
	if ( serverOnLocalMachine() ) return File::find_ecf_client_path();

	char* client_path_p = getenv("ECF_CLIENT_EXE_PATH");
	if ( client_path_p == NULL) {

		// Try this before complaining
      std::string path = "/usr/local/apps/ecflow/current/bin/ecflow_client";
 		if ( fs::exists(path) ) return path;

		cout << "Please set ECF_CLIENT_EXE_PATH. This needs to be set to path to the client executable\n";
		cout << "The client must be the one that was built on the same platform as the server\n";
		assert(false);
   }
 	return string(client_path_p);
}

void TestFixture::clearLog() {

	// Can't remove log on remote server, just clear the log file
   client().clearLog();
}

std::string TestFixture::pathToLogFile()
{
	if ( serverOnLocalMachine() )  {
		Host host;
		return host.ecf_log_file(port_);
	}

	char* pathToRemoteLog_p = getenv("ECF_LOG");
	if ( pathToRemoteLog_p == NULL) {
		cout << "TestFixture::pathToLogFile(): assert failed\n";
		cout << "Please set ECF_LOG. This needs to be set to path to the log file\n";
		cout << "that can be seen by the client and server\n";
		assert(false);
  	}
	return std::string(pathToRemoteLog_p);
}

std::string TestFixture::local_ecf_home()
{
   std::string rel_path = project_test_dir_;
#ifdef DEBUG

#if defined(_AIX)
   rel_path += "/data/ECF_HOME_debug_aix";
#elif defined(HPUX)
   rel_path += "/data/ECF_HOME_debug_hpux";
#else
#if defined(__clang__)
   rel_path += "/data/ECF_HOME_debug_clang";
#elif defined(__INTEL_COMPILER)
   rel_path += "/data/ECF_HOME_debug_intel";
#elif defined(_CRAYC)
   rel_path += "/data/ECF_HOME_debug_cray";
#else
   rel_path += "/data/ECF_HOME_debug_gnu";
#endif
#endif

#else

#if defined(_AIX)
   rel_path += "/data/ECF_HOME_release_aix";
#elif defined(HPUX)
   rel_path += "/data/ECF_HOME_release_hpux";
#else
#if defined(__clang__)
   rel_path += "/data/ECF_HOME_release_clang";
#elif defined(__INTEL_COMPILER)
   rel_path += "/data/ECF_HOME_release_intel";
#elif defined(_CRAYC)
   rel_path += "/data/ECF_HOME_release_cray";
#else
   rel_path += "/data/ECF_HOME_release_gnu";
#endif
#endif


#endif

   // Allow post-fix to be added, to allow test to run in parallel
   char* theEnv = getenv("TEST_ECF_HOME_POSTFIX");
   if (theEnv) rel_path += std::string(theEnv);

   std::string absolute_path = File::test_data(rel_path,project_test_dir_);
   return absolute_path;
}

std::string TestFixture::includes()
{
   // Get to the root source directory
   std::string includes_path = File::root_source_dir();
   includes_path += "/";
   includes_path += project_test_dir_;
   includes_path += "/data/includes";
   //std::cout << "includes_path = " << includes_path << " ==============================================\n";
   return includes_path;
}


/// Given a task name like "a" find the find the first task matching that name
/// and returns is abs node path
std::string TestFixture::taskAbsNodePath(const Defs& theDefs, const std::string& taskName)
{
	std::vector<Task*> vec;
	theDefs.getAllTasks(vec);
	BOOST_FOREACH(Task* t , vec ) {
		if (t->name() == taskName) return t->absNodePath();
	}

	cout << "TestFixture::taskAbsNodePath: assert failed: Could not find task " << taskName << "\n";
	assert(false); // could not find the task ??
	return string();
}

int TestFixture::server_version()
{
   client().server_version();
   std::string the_server_version_str = TestFixture::client().get_string();
   //cout << "\nserver_version_str = " << the_server_version_str << "\n";
   BOOST_REQUIRE_MESSAGE( Str::replace_all(the_server_version_str,".",""),"failed to find '.' in server version string " << TestFixture::client().get_string());

   // Could 4.0.8rc1
   Str::replace_all(the_server_version_str,"rc1","");
   Str::replace_all(the_server_version_str,"rc2","");
   int the_server_version = boost::lexical_cast<int>(the_server_version_str);
   return the_server_version;
}

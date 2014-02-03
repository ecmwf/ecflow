#define BOOST_TEST_MODULE Server
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "boost/filesystem/operations.hpp"

#include "ServerEnvironment.hpp"
#include "Log.hpp"
#include "Host.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "File.hpp"
#include "CheckPt.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestServer )


BOOST_AUTO_TEST_CASE( test_server_environment_ecfinterval )
{
	cout << "Server:: ...test_server_environment_ecfinterval\n";

	// ecflow server interval is valid for range [1-60]
	for(int i=-10; i< 70; ++i)
	{
		string errorMsg;
		string argument = "--ecfinterval=" + boost::lexical_cast<std::string>(i);

		int argc = 2;
		char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>(argument.c_str()) };
		ServerEnvironment serverEnv(argc,argv);
		bool valid = serverEnv.valid(errorMsg);
		if (i > 0 && i < 61)  {
			BOOST_REQUIRE_MESSAGE(valid,"Server environment ecfinterval valid range is [1-60] " << errorMsg);
			BOOST_CHECK_MESSAGE(serverEnv.submitJobsInterval() == i,"Expected submit jobs interval of " << i << " but found " << serverEnv.submitJobsInterval());
 		}
		else   BOOST_CHECK_MESSAGE(!valid,"Server environment ecfinterval valid range is [1-60] " << errorMsg);
	}

	Host h;
	fs::remove(h.ecf_log_file(Str::DEFAULT_PORT_NUMBER()));
}

BOOST_AUTO_TEST_CASE( test_server_environment_port )
{
	cout << "Server:: ...test_server_environment_port\n";

	// The port numbers are divided into three ranges.\n";
	//  o the Well Known Ports, (require root permission)      0 -1023\n";
	//  o the Registered Ports,                             1024 -49151\n";
	//  o Dynamic and/or Private Ports.                    49151 -65535\n\n";
	//  Please set in the range 1024-49151 via argument or \n";
	Host h;
	int argc = 2;
	{
		std::string errorMsg;
		char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=0") };
		ServerEnvironment serverEnv(argc,argv);
		BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg)," Server environment not valid " << errorMsg);
		fs::remove(h.ecf_log_file("0"));
 	}
	{
		std::string errorMsg;
		char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=1000") };
		ServerEnvironment serverEnv(argc,argv);
		BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg)," Server environment not valid " << errorMsg);
		fs::remove(h.ecf_log_file("1000"));
 	}
	{
		std::string errorMsg;
		char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=49151") };
		ServerEnvironment serverEnv(argc,argv);
		BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg)," Server environment not valid " << errorMsg);
		fs::remove(h.ecf_log_file("49151"));
 	}

	{
		std::string errorMsg;
		char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=3144") };
		ServerEnvironment serverEnv(argc,argv);
		BOOST_CHECK_MESSAGE(serverEnv.valid(errorMsg)," Server environment not valid " << errorMsg);
		BOOST_CHECK_MESSAGE(serverEnv.port() == 3144,"Expected 3144 but found " << serverEnv.port());
		fs::remove(h.ecf_log_file("3144"));
	}
}

BOOST_AUTO_TEST_CASE( test_server_environment_log_file )
{
   // Regression test log file creation
   cout << "Server:: ...test_server_environment_log_file\n";

   int argc = 2;
   char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=3144") };
   ServerEnvironment serverEnv(argc,argv);

   BOOST_CHECK_MESSAGE(Log::instance(), "Log singleton not created");
   BOOST_CHECK_MESSAGE(fs::exists(Log::instance()->path()), "Log file not created");

   // Check that server variable ECF_LOG created and value is correct
   std::vector<std::pair<std::string,std::string> > server_vars;
   serverEnv.variables(server_vars);

   bool found_var = false;
   typedef std::pair<std::string, std::string> mpair;
   BOOST_FOREACH(const mpair& p, server_vars ) {
      if (Str::ECF_LOG() == p.first) {
         BOOST_CHECK_MESSAGE(p.second == Log::instance()->path(),"Expected " << Log::instance()->path() << " but found " << p.second);
         found_var = true;
         break;
      }
   }
   BOOST_CHECK_MESSAGE(found_var,"Failed to find server variable ECF_LOG");

   // tear down remove the log file created by ServerEnvironment
   Host h;
   fs::remove(h.ecf_log_file("3144"));

   /// Destroy Log singleton to avoid valgrind from complaining
   Log::destroy();
}

BOOST_AUTO_TEST_CASE( test_server_config_file )
{
   // Regression test to make sure the server environment variable don't get removed
   cout << "Server:: ...test_server_config_file\n";

   int argc = 1;
   char* argv[] = { const_cast<char*>("ServerEnvironment")  };

   ServerEnvironment serverEnv(argc,argv,File::test_data("Server/server_environment.cfg","Server"));

   // expected variables
   std::vector<std::string> expected_variables;
   expected_variables.push_back(  Str::ECF_HOME() );
   expected_variables.push_back( "ECF_LOG"  );
   expected_variables.push_back( "ECF_CHECK" );
   expected_variables.push_back( "ECF_CHECKOLD" );
   expected_variables.push_back( "ECF_JOB_CMD" );
   expected_variables.push_back( "ECF_KILL_CMD" );
   expected_variables.push_back( "ECF_STATUS_CMD" );
   expected_variables.push_back( "ECF_URL_CMD" );
   expected_variables.push_back( "ECF_URL_BASE" );
   expected_variables.push_back( "ECF_URL" );
   expected_variables.push_back( "ECF_MICRO" );
   expected_variables.push_back( "ECF_PID" );
   expected_variables.push_back( "ECF_VERSION" );
   expected_variables.push_back(  Str::ECF_PORT() );
   expected_variables.push_back( "ECF_NODE");

   std::vector<std::pair<std::string,std::string> > server_vars;
   serverEnv.variables(server_vars);
   BOOST_FOREACH(const std::string& expected_var, expected_variables) {

      bool found_var = false;
      typedef std::pair<std::string, std::string> mpair;
      BOOST_FOREACH(const mpair& p, server_vars ) {
         if (expected_var == p.first) { found_var = true; break; }
      }
      BOOST_CHECK_MESSAGE(found_var,"Failed to find server var " << expected_var);
   }

   // check other way, so that this test gets updated
   typedef std::pair<std::string, std::string> mpair;
   BOOST_FOREACH(const mpair& p, server_vars ) {
      bool found_var = false;
      BOOST_FOREACH(const std::string& expected_var, expected_variables) {
         if (expected_var == p.first) { found_var = true; break; }
      }
      BOOST_CHECK_MESSAGE(found_var,"Failed to update test for server var " << p.first);
   }

   // Check the values in the server config file, are the *SAME* as the defaults, when config is *NOT* present
   // Please note do *NOT* use quotes for the values, otherwise quotes get added.
   // WRONG: ECF_MICRODEF = "%"
   // RIGHT: ECF_MICRODEF = %
   // o IGNORE ECF_CHECK: We *ONLY check those value in the config, that should not be altered.
   //                     since we add root path, and append with host/port
   // o ignore ECF_CHECKMODE: not a server variable
   //
   typedef std::pair<std::string, std::string> mpair;
   BOOST_FOREACH(const mpair& p, server_vars ) {
      //std::cout << "server variables " << p.first << "  " << p.second << "\n";
      if (Str::ECF_HOME() == p.first) {
         BOOST_CHECK_MESSAGE(p.second == fs::current_path().string(),"for ECF_HOME expected " << fs::current_path().string() << " but found " << p.second);
         continue;
      }
      if (string("ECF_PORT") == p.first) {
         BOOST_CHECK_MESSAGE(p.second == Str::DEFAULT_PORT_NUMBER(),"for ECF_PORT expected " <<  Str::DEFAULT_PORT_NUMBER() << " but found " << p.second);
         continue;
      }
      if (string("ECF_CHECKINTERVAL") == p.first) {
         std::string expected = boost::lexical_cast<std::string>(CheckPt::default_interval());
         BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_CHECKINTERVAL expected " <<  CheckPt::default_interval() << " but found " << p.second);
         continue;
      }
      if (string("ECF_INTERVAL") == p.first) {
          std::string expected = "60";
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_INTERVAL expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_JOB_CMD") == p.first) {
          std::string expected = Ecf::JOB_CMD();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_JOB_CMD expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_KILL_CMD") == p.first) {
          std::string expected = Ecf::KILL_CMD();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_KILL_CMD expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_STATUS_CMD") == p.first) {
          std::string expected = Ecf::STATUS_CMD();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_STATUS_CMD expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_URL_CMD") == p.first) {
          std::string expected = Ecf::URL_CMD();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_URL_CMD expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_URL_BASE") == p.first) {
          std::string expected = Ecf::URL_BASE();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_URL_BASE expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_URL") == p.first) {
          std::string expected = Ecf::URL();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_URL expected " <<  expected << " but found " << p.second);
          continue;
      }
      if (string("ECF_MICRODEF") == p.first) {
          std::string expected = Ecf::MICRO();
          BOOST_CHECK_MESSAGE(p.second == expected,"for ECF_MICRODEF expected " <<  expected << " but found " << p.second);
          continue;
      }
   }

   // tear down remove the log file created by ServerEnvironment
   Host host;
   fs::remove(host.ecf_log_file(Str::DEFAULT_PORT_NUMBER()));
}


BOOST_AUTO_TEST_CASE( test_server_environment_variables )
{
   // Regression test to make sure the server environment variable don't get removed
   cout << "Server:: ...test_server_environment_variables\n";

   int argc = 2;
   char* argv[] = { const_cast<char*>("ServerEnvironment"),  const_cast<char*>("--port=3144") };
   ServerEnvironment serverEnv(argc,argv);

   // expected variables
   std::vector<std::string> expected_variables;
   expected_variables.push_back(  Str::ECF_HOME() );
   expected_variables.push_back( "ECF_LOG"  );
   expected_variables.push_back( "ECF_CHECK" );
   expected_variables.push_back( "ECF_CHECKOLD" );
   expected_variables.push_back( "ECF_JOB_CMD" );
   expected_variables.push_back( "ECF_KILL_CMD" );
   expected_variables.push_back( "ECF_STATUS_CMD" );
   expected_variables.push_back( "ECF_URL_CMD" );
   expected_variables.push_back( "ECF_URL_BASE" );
   expected_variables.push_back( "ECF_URL" );
   expected_variables.push_back( "ECF_MICRO" );
   expected_variables.push_back( "ECF_PID" );
   expected_variables.push_back( "ECF_VERSION" );
   expected_variables.push_back(  Str::ECF_PORT() );
   expected_variables.push_back( "ECF_NODE");

   std::vector<std::pair<std::string,std::string> > server_vars;
   serverEnv.variables(server_vars);
   BOOST_FOREACH(const std::string& expected_var, expected_variables) {

      bool found_var = false;
      typedef std::pair<std::string, std::string> mpair;
      BOOST_FOREACH(const mpair& p, server_vars ) {
         if (expected_var == p.first) { found_var = true; break; }
      }
      BOOST_CHECK_MESSAGE(found_var,"Failed to find server var " << expected_var);
   }

   // check other way, so that this test gets updated
   typedef std::pair<std::string, std::string> mpair;
   BOOST_FOREACH(const mpair& p, server_vars ) {
      bool found_var = false;
      BOOST_FOREACH(const std::string& expected_var, expected_variables) {
         if (expected_var == p.first) { found_var = true; break; }
      }
      BOOST_CHECK_MESSAGE(found_var,"Failed to update test for server var " << p.first);
   }

   // tear down remove the log file created by ServerEnvironment
   Host h;
   fs::remove(h.ecf_log_file("3144"));

   /// Destroy Log singleton to avoid valgrind from complaining
   Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #29 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <string>
#include <fstream>
#include <sys/stat.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "MyDefsFixture.hpp"
#include "Version.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// Override ECF_HOME. ECF_HOME is needed to locate to the .ecf files
//theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************
BOOST_AUTO_TEST_CASE( test_log_and_checkpt_write_errors )
{
   //cout << "->Create a directory from where we will start the server\n";
   std::string ecf_home = "test_log_and_checkpt_write_errors";
   fs::create_directory(ecf_home);

   //cout << "->change directory, before server start\n";
   BOOST_CHECK_MESSAGE( chdir(ecf_home.c_str()) == 0  , "Can't change directory to " << ecf_home << "  error: " << strerror(errno)  );
   //cout << "->current path = " << boost::filesystem::current_path() << "\n";

   {
      //cout << "->start the server\n";
      InvokeServer invokeServer("Client:: ...test_log_and_checkpt_write_errors",SCPort::next());
      BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );

      ClientInvoker theClient(invokeServer.host(),invokeServer.port());
      BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0 server not started, or connection refused\n" << theClient.errorMsg());

      //cout << "->load a defs file to the server\n";
      std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");
      BOOST_CHECK_MESSAGE(theClient.loadDefs(path) == 0,"load defs failed \n" << theClient.errorMsg());

      //cout << "->flush the log file, this will close the log file in the server\n";
      BOOST_CHECK_MESSAGE(theClient.flushLog() == 0,"flushLog failed \n" << theClient.errorMsg());

      //cout << "->remove write permission for " << ecf_home << "\n";
      BOOST_CHECK_MESSAGE( chdir("..") == 0  , "Can't change directory to parent  error: " << strerror(errno)  );
      BOOST_CHECK_MESSAGE( chmod(ecf_home.c_str(), strtol("0444" , 0, 8)) == 0 , "Can't chmod : " << strerror(errno)  );

      //cout << "->write the checkpoint file,  this should also try to re-open the log file and fail\n";
      BOOST_CHECK_MESSAGE(theClient.checkPtDefs() == 0,CtsApi::checkPtDefs() << " Checkpoint expected to pass\n" << theClient.errorMsg());

      //cout << "->get the defs from server, Check flags, check server variables\n";
      BOOST_CHECK_MESSAGE(theClient.sync_local() == 0,"sync_local failed \n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE(theClient.defs()->get_flag().is_set(Flag::CHECKPT_ERROR),"Expected Flag::CHECKPT_ERROR to be set");
      BOOST_CHECK_MESSAGE(theClient.defs()->get_flag().is_set(Flag::LOG_ERROR),    "Expected Flag::LOG_ERROR to be set");
      BOOST_CHECK_MESSAGE(theClient.defs()->server().variable_exists("ECF_CHECKPT_ERROR"), "Expected to find ECF_CHECKPT_ERROR as a server variable");
      BOOST_CHECK_MESSAGE(theClient.defs()->server().variable_exists("ECF_LOG_ERROR"), "Expected to find ECF_LOG_ERROR as a server variable");

      //cout << "->add write permission to " << ecf_home << "\n";
      BOOST_CHECK_MESSAGE( chmod(ecf_home.c_str(),strtol("0755" , 0, 8)) == 0 , "Can't chmod : " << strerror(errno)  );

      //cout << "->flush log file again\n";
      BOOST_CHECK_MESSAGE(theClient.flushLog() == 0,"flushLog failed \n" << theClient.errorMsg());

      //cout << "->clear the flags -> this should also delete the server user variables\n";
      BOOST_CHECK_MESSAGE(theClient.alter("/","clear_flag","log_error") == 0    ,"alter / clear_flag log_error : failed \n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE(theClient.alter("/","clear_flag","checkpt_error") == 0,"alter / clear_flag checkpt_error : failed \n" << theClient.errorMsg());

      //cout << "->checkpoint again\n";
      BOOST_CHECK_MESSAGE(theClient.checkPtDefs() == 0,CtsApi::checkPtDefs() << " Checkpoint expected to fail\n" << theClient.errorMsg());

      //cout << "->Get the defs, Check flags are cleared and server variables are deleted\n";
      BOOST_CHECK_MESSAGE(theClient.sync_local() == 0,"sync_local failed \n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE(!theClient.defs()->get_flag().is_set(Flag::CHECKPT_ERROR),"Expected Flag::CHECKPT_ERROR to be cleared");
      BOOST_CHECK_MESSAGE(!theClient.defs()->get_flag().is_set(Flag::LOG_ERROR),    "Expected Flag::LOG_ERROR to be cleared");
      BOOST_CHECK_MESSAGE(!theClient.defs()->server().variable_exists("ECF_CHECKPT_ERROR"), "Expected to NOT find ECF_CHECKPT_ERROR as a server variable");
      BOOST_CHECK_MESSAGE(!theClient.defs()->server().variable_exists("ECF_LOG_ERROR"),     "Expected to NOT find ECF_LOG_ERROR as a server variable");
   }

   //cout << "->remove created directory " << ecf_home << "\n";
   //cout << "->current path = " << boost::filesystem::current_path() << "\n";
   BOOST_CHECK_MESSAGE(File::removeDir( ecf_home ),"Failed to remove dir " << ecf_home << "  error: " << strerror(errno) );
}
BOOST_AUTO_TEST_SUITE_END()

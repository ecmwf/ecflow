//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
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

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )


static void wait_for_sigterm_in_server(ClientInvoker& theClient)
{
    int count = 0;
    while(1) {
       count++;
       sleep(1);
       BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Sync local failed\n" << theClient.errorMsg());
       if ( theClient.defs()->get_flag().is_set(ecf::Flag::ECF_SIGTERM) )  {
          // cout << "break after " << count << "s\n";
          break;
       }
       BOOST_REQUIRE_MESSAGE(count > 9,"Server never received SIGTERM after 9 seconds");
    }
}

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// This test will send a signal SIGTERM, i.e (via kill -15 pid) and check to ensure
// that a check point file is saved.
// ************************************************************************************
BOOST_AUTO_TEST_CASE( test_signal_SIGTERM )
{
   // This will remove check pt and backup file before server start, to avoid the server from loading previous test data
   InvokeServer invokeServer("Client:: ...test_signal_SIGTERM",SCPort::next());
   BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0 server not started, or connection refused\n" << theClient.errorMsg());

   std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");
   BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0,"load defs failed \n" << theClient.errorMsg());

   // Get the definition
   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Sync local failed\n" << theClient.errorMsg());

   // Get the process id of the server
   const std::string& ecf_pid = theClient.defs()->server().find_variable("ECF_PID");
   BOOST_REQUIRE_MESSAGE(!ecf_pid.empty(),"ECF_PID not set in the server");

   // Send a SIGTERM to the server and ensure that a check point file is created
   std::string sigterm = "kill -15 " + ecf_pid ;
   system(sigterm.c_str());
   wait_for_sigterm_in_server(theClient);

   // Clear sigterm flag
   BOOST_REQUIRE_MESSAGE( theClient.alter("/","clear_flag","sigterm") == 0,"--alter should return 0\n" << theClient.errorMsg());

   // We expect a check point file to be save to disk, but *no* backup
   BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_checkpt_file()),CtsApi::checkPtDefs() << " failed file(" << invokeServer.ecf_checkpt_file() << ") not saved");
   BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_checkpt_file()) !=0,"Expected check point file(" << invokeServer.ecf_checkpt_file() << "), to have file size > 0");
   if (ClientEnvironment::hostSpecified().empty()) {
      // This check only valid if server was invoked locally. Ignore for remote servers
      BOOST_REQUIRE_MESSAGE(!fs::exists(invokeServer.ecf_backup_checkpt_file()), "Backup check point file(" << invokeServer.ecf_backup_checkpt_file() << ")should not exist,for very first time.");
   }


   // Send a SIGTERM again. This time we expect the backup check point file to be created.
   system(sigterm.c_str());
   wait_for_sigterm_in_server(theClient);

   BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_checkpt_file()),CtsApi::checkPtDefs() << " failed No check pt file(" << invokeServer.ecf_checkpt_file() << ") saved");
   BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_checkpt_file()) !=0,"Expected check point file(" << invokeServer.ecf_checkpt_file() << ") to have file size > 0  ");
   BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_backup_checkpt_file()), "Expected backup check point file(" << invokeServer.ecf_backup_checkpt_file() << ") to be created");
   BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_backup_checkpt_file()) !=0,"Expected backup check point file(" << invokeServer.ecf_backup_checkpt_file() << "), to have file size > 0");
}

BOOST_AUTO_TEST_SUITE_END()


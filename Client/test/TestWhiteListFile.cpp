//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "WhiteListFile.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// Note: To test HPUX->Linux, invoke serve on (Linux/HPUX) and the client cmds on other system
//       On the client side set ECF_NODE to machine name. To allow further testing if ECF_NODE
//       is specified then *don't* shutdown the server
// ************************************************************************************

BOOST_AUTO_TEST_CASE( test_loading_of_white_list_file )
{
   Host the_host;
   std::string port = SCPort::next();
   std::string host = ClientEnvironment::hostSpecified();
   if (host.empty()) {

      // make sure NO whitelist file is present before the server is started.
      // This allows any user to send requests to the server
      // Only do this locally, as white list file on remote machine may not be accessible
      fs::remove(the_host.ecf_lists_file(port));
   }

   // This will remove check pt and backup file before server start, to avoid the server from loading previous test data
   InvokeServer invokeServer("Client:: ...test_loading_of_white_list_file",port);

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());

   // OK now test white list file functionality, but only if server was invoked locally
   if ( host.empty() ) {

      // The white list file should not exist, hence reload white list SHOULD fail.
      // i.e because we deleted it earlier
      BOOST_REQUIRE_THROW( theClient.reloadwsfile(), std::runtime_error);

      // Create a valid white list file; For the FIRST time:
      // **** IMPORTANT: if we had reloaded a white list file where the user has read access
      // **************: _FIRST_ it will not be possible reload white file with write access, afterwards
      // **************: since subsequent RELOAD command itself will require write access
      std::string errorMsg;
      BOOST_REQUIRE_MESSAGE(WhiteListFile::createWithWriteAccess(the_host.ecf_lists_file(port),errorMsg),errorMsg);

      //  Reload should pass, as its the _first_ time
      BOOST_REQUIRE_MESSAGE( theClient.reloadwsfile() == 0, CtsApi::reloadwsfile() << " should return 0\n" << theClient.errorMsg());

      // Invoking a client request that requires write access, should pass
      BOOST_CHECK_MESSAGE( theClient.shutdownServer() == 0,"should return 0\n" << theClient.errorMsg());

      // Invoking a client request that requires read access, should also pass
      BOOST_CHECK_MESSAGE( theClient.getDefs() == 0,"should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.sync_local() == 0,"should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.news_local() == 0,"should return 0\n" << theClient.errorMsg());

      // Remove the white list file. Comment out for debug
      fs::remove(the_host.ecf_lists_file(port));
   }
}

BOOST_AUTO_TEST_SUITE_END()


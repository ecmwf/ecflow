//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #42 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

BOOST_AUTO_TEST_CASE( test_client_handle_cmd )
{
   /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
   InvokeServer invokeServer("Client:: ...test_client_handle_cmd ",SCPort::next());
   BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );
   ClientInvoker theClient(invokeServer.host(), invokeServer.port());
   theClient.set_auto_sync(true);

   std::vector<std::string> suites = { "s1", "s2" };
   defs_ptr defs = Defs::create();
   for(auto s : suites) { defs->add_suite(s);}

   BOOST_REQUIRE_MESSAGE(theClient.load(defs) == 0,"load defs failed \n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == suites.size(),"load failed");

   BOOST_REQUIRE_MESSAGE(theClient.ch1_register(true,suites) == 0,"ch1_register failed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == suites.size(),"register group sync failed");
   BOOST_REQUIRE_MESSAGE(theClient.server_reply().client_handle() == 1," ch1_register failed expected handle 1");

   BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,"delete_all failed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == 0,"delete all failed");
   BOOST_REQUIRE_MESSAGE(theClient.server_reply().client_handle() == 0,"delete all failed expected handle 0 but found " << theClient.server_reply().client_handle());

   // repeat test without auto_sync
   theClient.set_auto_sync(false);
   BOOST_REQUIRE_MESSAGE(theClient.load(defs) == 0,"load defs failed \n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0,"sync_local failed \n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == suites.size(),"load failed");

   BOOST_REQUIRE_MESSAGE(theClient.ch1_register(true,suites) == 0,"ch1_register failed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0,"sync_local failed \n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == suites.size(),"register group sync failed");
   BOOST_REQUIRE_MESSAGE(theClient.server_reply().client_handle() == 1," ch1_register failed expected handle 1");

   BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,"delete_all failed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0,"sync_local failed \n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == 0,"delete all failed");
   BOOST_REQUIRE_MESSAGE(theClient.server_reply().client_handle() == 0,"delete all failed expected handle 0 but found " << theClient.server_reply().client_handle());

   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

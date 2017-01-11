//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #42 $
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
#include <string>
#include <iostream>
#include <fstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

BOOST_AUTO_TEST_CASE( test_flag_migrate )
{
   // This test relies on a NEW server invocation. Hence if ECF_HOST/remote server is used
   // the test will will invalid. hence ignore.
   if (!ClientEnvironment::hostSpecified().empty()) {
      cout << "Client:: ...test_flag_migrate: ignoring test when ECF_HOST specified\n";
      return;
   }

   /// Test that when a node is  ecf::Flag::MIGRATED, the children are not persisted for sync
   /// But nodes are still persisted for check-pointing
   InvokeServer invokeServer("Client:: ...test_flag_migrate",SCPort::next());

   std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(path,false,false/* check only*/) == 0,"Expected load to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   size_t expected_nodes = 0;
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      expected_nodes = all_nodes.size();
      BOOST_REQUIRE_MESSAGE(expected_nodes > 5  ,"Expected > 5 nodes but found " << all_nodes.size());
   }

   // delete defs
   BOOST_REQUIRE_MESSAGE( theClient.delete_all(true) == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 0,"Expected defs with 0 suite\n" << theClient.errorMsg());

   // reload the defs
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(path,false,false/* check only*/) == 0,"Expected load to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == expected_nodes  ,"Expected "  << expected_nodes << " nodes but found " << all_nodes.size());
   }

   // set suite as migrated. The children of the suite should not be persisted, but still check pointed
   BOOST_REQUIRE_MESSAGE( theClient.alter("/suite1","set_flag","migrated") == 0,"Expected alter to succeed\n" << theClient.errorMsg());

   // INCREMENTAL SYNC
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == 1  ,"Expected 1 nodes but found " << all_nodes.size());
   }

   // FULL sync
   theClient.reset();
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == 1  ,"Expected 1 nodes but found " << all_nodes.size());
   }

   // Ensure that checkpoint still persist the node, even if ecf::Flag::MIGRATED is set
   BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,CtsApi::checkPtDefs() << " failed should return 0\n" << theClient.errorMsg());
   {
      Defs defs;
      defs.restore_from_checkpt(invokeServer.ecf_checkpt_file()); // make a data model change
      std::vector<node_ptr> all_nodes; defs.get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == expected_nodes  ,"Expected "  << expected_nodes << " nodes but found " << all_nodes.size());
   }

   // Repeat test with recovered checkpoint. First delete defs in server.
   BOOST_REQUIRE_MESSAGE( theClient.delete_all(true) == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 0,"Expected defs with 0 suite\n" << theClient.errorMsg());

   // restore from checkpoint
   BOOST_REQUIRE_MESSAGE(theClient.restoreDefsFromCheckPt() == 0, "restore checkpoint failed should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());

   // Clear migrate flag
   BOOST_REQUIRE_MESSAGE( theClient.alter("/suite1","clear_flag","migrated") == 0,"Expected alter to succeed\n" << theClient.errorMsg());

   // ensure that recovered checkpoint still has the expected_nodes, with increment and *FULL* sync
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == expected_nodes  ,"Expected "  << expected_nodes << " nodes but found " << all_nodes.size());
   }

   // FULL sync
   theClient.reset();
   BOOST_REQUIRE_MESSAGE( theClient.sync_local() == 0,"Expected sync to succeed\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.defs() && theClient.defs()->suiteVec().size() == 1,"Expected defs with 1 suite\n" << theClient.errorMsg());
   {
      std::vector<node_ptr> all_nodes;
      theClient.defs()->get_all_nodes(all_nodes);
      BOOST_REQUIRE_MESSAGE(all_nodes.size() == expected_nodes  ,"Expected "  << expected_nodes << " nodes but found " << all_nodes.size());
   }
}

BOOST_AUTO_TEST_SUITE_END()

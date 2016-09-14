/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ServerToClientCmdContext.hpp"
#include "MyDefsFixture.hpp"
#include "Ecf.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_FIXTURE_TEST_SUITE( NodeTestSuite, MyDefsFixture )

// Allow for multiple archives
static void testPersistence(const Defs& fixtureDefs,ecf::Archive::Type at)
{
	std::string check_pt_file = "fixture_defs.check";
	fixtureDefs.save_as_checkpt(check_pt_file,at);

	Defs restoredDefs;
	restoredDefs.restore_from_checkpt(check_pt_file,at);

	bool theyCompare = (restoredDefs == fixtureDefs);
	if (!theyCompare) {

		std::cout << "Dump restored defs\n" << restoredDefs << "\n";
		std::cout << "Dump fixture defs\n" << fixtureDefs << "\n";

		BOOST_CHECK_MESSAGE(theyCompare,"restored defs file is not same as fixtureDefs defs file");
	}

	cout << " check pt file_size: " <<  fs::file_size(check_pt_file) << "\n";

 	// Uncomment if you want see what this file looks like
	fs::remove(check_pt_file);
}

#if defined(BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_binary )
{
	cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_binary";
	BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
	testPersistence(fixtureDefsFile(),ecf::Archive::BINARY);
}
#elif defined(PORTABLE_BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_portable_binary )
{
	cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_portable_binary";
	BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
	testPersistence(fixtureDefsFile(),ecf::Archive::PORTABLE_BINARY);
}
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_eos_portable_binary )
{
   cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_eos_portable_binary";
   BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
   testPersistence(fixtureDefsFile(),ecf::Archive::EOS_PORTABLE_BINARY);
}
#else
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_text )
{
   cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_text" ;
   BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
   testPersistence(fixtureDefsFile(),ecf::Archive::TEXT);
}
#endif


BOOST_AUTO_TEST_CASE( test_persistece_with_migrate_flag )
{
   Defs defs;
   suite_ptr s = defs.add_suite("test_persistece_with_migrate_flag");
   family_ptr f1 = s->add_family("f1");
   f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");

   // Remove host dependent variables from server state, so that we can run on other platforms
   defs.set_server().delete_server_variable("ECF_LOG");
   defs.set_server().delete_server_variable("ECF_CHECK");
   defs.set_server().delete_server_variable("ECF_CHECKOLD");

   std::string file_name = File::test_data("ANode/test/data/test_persistece_with_migrate_flag","ANode");
   {
      defs.save_as_checkpt(file_name);
      Defs restored_defs; restored_defs.restore_from_checkpt(file_name);

      Ecf::set_debug_equality(true);
      BOOST_CHECK_MESSAGE(defs == restored_defs, "Saved and restored defs not the same");
      Ecf::set_debug_equality(false);

      std::vector<node_ptr> all_nodes;
      restored_defs.get_all_nodes(all_nodes);
      BOOST_CHECK_MESSAGE(all_nodes.size()==4,"expected 4 nodes but found " << all_nodes.size());

      fs::remove(file_name);
   }

   {
      // this time we only expect 1 node
      s->flag().set(ecf::Flag::MIGRATED);

      {
         // Will not persist children of migrate node
         ServerToClientCmdContext cmd;
         defs.save_as_checkpt(file_name);
      }

      Defs restored_defs; restored_defs.restore_from_checkpt(file_name);

      std::vector<node_ptr> all_nodes;
      restored_defs.get_all_nodes(all_nodes);
      BOOST_CHECK_MESSAGE(all_nodes.size()==1,"expected 1 nodes but found " << all_nodes.size());

      fs::remove(file_name);
      s->flag().clear(ecf::Flag::MIGRATED);
   }

   {
      // this time we only expect 2 node
      f1->flag().set(ecf::Flag::MIGRATED);

      {
         // Will not persist children of migrate node
         ServerToClientCmdContext cmd;
         defs.save_as_checkpt(file_name);
      }

      Defs restored_defs; restored_defs.restore_from_checkpt(file_name);

      std::vector<node_ptr> all_nodes;
      restored_defs.get_all_nodes(all_nodes);
      BOOST_CHECK_MESSAGE(all_nodes.size()==2,"expected 2 nodes but found " << all_nodes.size());

      fs::remove(file_name);
      f1->flag().clear(ecf::Flag::MIGRATED);
   }

   {
      // this time we only expect 4 node
      t2->flag().set(ecf::Flag::MIGRATED);

      {
         // Will not persist children of migrate node
         ServerToClientCmdContext cmd;
         defs.save_as_checkpt(file_name);
      }

      Defs restored_defs; restored_defs.restore_from_checkpt(file_name);

      std::vector<node_ptr> all_nodes;
      restored_defs.get_all_nodes(all_nodes);
      BOOST_CHECK_MESSAGE(all_nodes.size()==4,"expected 4 nodes but found " << all_nodes.size());

      fs::remove(file_name);
   }
}
BOOST_AUTO_TEST_SUITE_END()

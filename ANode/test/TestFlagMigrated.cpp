/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Ecf.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_flag_migrated )
{
   cout << "ANode:: ...test_flag_migrated\n" ;
   // This will test that when the migrate flag is set
   // we do not persist the children of Suites and families.
   // However we should always persist when check-pointing

   Defs defs;
   suite_ptr s = defs.add_suite("test_flag_migrated");
   family_ptr f1 = s->add_family("f1");
   f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");
   size_t expected_all_nodes = 4;

   // Remove host dependent variables from server state, so that we can run on other platforms
   defs.set_server().delete_server_variable("ECF_LOG");
   defs.set_server().delete_server_variable("ECF_CHECK");
   defs.set_server().delete_server_variable("ECF_CHECKOLD");

   std::string file_name = File::test_data("ANode/test/data/test_flag_migrated","ANode");
   {
      defs.boost_save_as_checkpt(file_name);
      Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);

      DebugEquality debug_equality; // only as affect in DEBUG build
      BOOST_CHECK_MESSAGE(defs == restored_defs, "Saved and restored defs not the same");

      std::vector<node_ptr> all_nodes;
      restored_defs.get_all_nodes(all_nodes);
      BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());

      fs::remove(file_name);
   }

   {
      // this time we only expect 1 node
      s->flag().set(ecf::Flag::MIGRATED);

      {
         defs.boost_save_as_checkpt(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
      {
         defs.boost_save_as_filename(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==1,"expected 1 nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }

      s->flag().clear(ecf::Flag::MIGRATED);
   }

   {
      // this time we only expect 2 node
      f1->flag().set(ecf::Flag::MIGRATED);

      {
         defs.boost_save_as_checkpt(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
      {
         defs.boost_save_as_filename(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==2,"expected 2 nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }

      f1->flag().clear(ecf::Flag::MIGRATED);
   }

   {
      // this time expected_all_nodes
      t2->flag().set(ecf::Flag::MIGRATED);

      {
         defs.boost_save_as_checkpt(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
      {
         defs.boost_save_as_filename(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes;
         restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
   }
}

BOOST_AUTO_TEST_CASE( test_flag_migrated_with_reque )
{
   cout << "ANode:: ...test_flag_migrated_with_reque\n" ;
   // This will test the re-queue should clear the migrated flag, and cause children of
   // suite/families to appear once again.

   Defs defs;
   suite_ptr s = defs.add_suite("test_flag_migrated_with_reque");
   family_ptr f1 = s->add_family("f1");
   f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");
   size_t expected_all_nodes = 4;

   defs.beginAll();

   // Remove host dependent variables from server state, so that we can run on other platforms
   defs.set_server().delete_server_variable("ECF_LOG");
   defs.set_server().delete_server_variable("ECF_CHECK");
   defs.set_server().delete_server_variable("ECF_CHECKOLD");

   std::string file_name = File::test_data("ANode/test/data/test_flag_migrated_with_reque","ANode");
   {
      // this time we only expect 1 node
      s->flag().set(ecf::Flag::MIGRATED);

      {
         defs.boost_save_as_checkpt(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes; restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
      {
         defs.boost_save_as_filename(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes; restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==1,"expected 1 nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
   }

   {
      // Reque means migrate flag was cleared, hence we expect full set of nodes.
      defs.requeue(); // this should clear the migrate flag

      BOOST_CHECK_MESSAGE(!s->get_flag().is_set(ecf::Flag::MIGRATED),"Expected migrate flag to be cleared");

      {
         defs.boost_save_as_checkpt(file_name);
         Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
         std::vector<node_ptr> all_nodes; restored_defs.get_all_nodes(all_nodes);
         BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
         fs::remove(file_name);
      }
      {
          defs.boost_save_as_filename(file_name);
          Defs restored_defs; restored_defs.boost_restore_from_checkpt(file_name);
          std::vector<node_ptr> all_nodes; restored_defs.get_all_nodes(all_nodes);
          BOOST_CHECK_MESSAGE(all_nodes.size()==expected_all_nodes,"expected " << expected_all_nodes << " nodes but found " << all_nodes.size());
          fs::remove(file_name);
       }
   }
}
BOOST_AUTO_TEST_SUITE_END()

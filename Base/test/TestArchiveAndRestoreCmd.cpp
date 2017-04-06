//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #48 $
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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "TestHelper.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "System.hpp"
#include "Pid.hpp"
#include "File.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_archive_and_restore_suite )
{
   cout << "Base:: ...test_archive_and_restore_suite\n";

   // Please note: after archive we delete the children, hence any references to child refer to a different object

   // Create the defs file corresponding to the text below
   //suite suite
   //  edit ECF_HOME ....
   //  family f1
   //     task t1

   // We use Pid::unique_name, to allow multiple invocation of this test
   Defs theDefs;
   std::string ecf_home = File::test_data("Base/test","Base");
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);
   suite_ptr suite = theDefs.add_suite( Pid::unique_name("test_archive_and_restore_suite")  );
   suite->add_family( "f1" )->add_task("t1");
   //cout << theDefs << "\n";

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,suite->absNodePath())));
   BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set");
   BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()),"Archive path" << suite->archive_path() << " not created");
   BOOST_CHECK_MESSAGE( suite->nodeVec().empty(),"Children not removed");
   //cout << theDefs << "\n";

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,suite->absNodePath())));
   BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::RESTORED),"Restored flag not set");
   BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared");
   BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()),"Archived file has not been deleted after restore");
   BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(),"Children are not restored");

   // Archive again but restore via begin
   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,suite->absNodePath())));
   BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set");
   BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()),"Archive path" << suite->archive_path() << " not created");
   BOOST_CHECK_MESSAGE( suite->nodeVec().empty(),"Children not removed");

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new BeginCmd(suite->absNodePath(),true))); // nodes will be active
   BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared");
   BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::RESTORED),"restored flag not *cleared");
   BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()),"Archived file has not been deleted after restore");
   BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(),"Children are not restored");

   //PrintStyle::setStyle(PrintStyle::MIGRATE);
   //cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE( test_archive_and_restore_family )
{
   cout << "Base:: ...test_archive_and_restore_family\n";

   // Create the defs file corresponding to the text below
   //suite suite
   //  edit ECF_HOME ....
   //  family f1
   //     family f2
   //       family f3
   //          task t1

   Defs theDefs;
   std::string ecf_home = File::test_data("Base/test","Base");
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);
   suite_ptr suite = theDefs.add_suite( Pid::unique_name("test_archive_and_restore_family")  );
   family_ptr f3 = suite->add_family( "f1" )->add_family("f2")->add_family("f3");
   f3->add_task("t1");

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,f3->absNodePath())));
   BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set");
   BOOST_CHECK_MESSAGE(fs::exists(f3->archive_path()),"Archive path" << f3->archive_path() << " not created");
   BOOST_CHECK_MESSAGE( f3->nodeVec().empty(),"Children not removed");

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,f3->absNodePath())));
   BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::RESTORED),"restored flag not set");
   BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared");
   BOOST_CHECK_MESSAGE(!fs::exists(f3->archive_path()),"Archived file has not been deleted after restore");
   BOOST_CHECK_MESSAGE(!f3->nodeVec().empty(),"Children are not restored");

   // Archive again but restore via begin
   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,f3->absNodePath())));
   BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set");
   BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::RESTORED),"Restored flag not cleared");
   BOOST_CHECK_MESSAGE(fs::exists(f3->archive_path()),"Archive path" << suite->archive_path() << " not created");
   BOOST_CHECK_MESSAGE( f3->nodeVec().empty(),"Children not removed");

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new BeginCmd(suite->absNodePath(),true))); // nodes will be active
   BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared");
   BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::RESTORED),"Restored flag not *cleared");
   BOOST_CHECK_MESSAGE(!fs::exists(f3->archive_path()),"Archived file has not been deleted after restore");
   BOOST_CHECK_MESSAGE(!f3->nodeVec().empty(),"Children are not restored");

//   PrintStyle::setStyle(PrintStyle::MIGRATE);
//   cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE( test_archive_and_restore_all )
{
   cout << "Base:: ...test_archive_and_restore_all\n";

   // Create the defs file corresponding to the text below
   //suite suite
   //  edit ECF_HOME ....
   //  family f1
   //     task t1; task t2
   //     family f2
   //       task t1; task t2
   //       family f3
   //         task t1; task t2
   //         family f4
   //           task t1; task t2

   Defs theDefs; {
      std::string ecf_home = File::test_data("Base/test","Base");
      theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);
      suite_ptr suite = theDefs.add_suite( Pid::unique_name("test_archive_and_restore_all")  );
      family_ptr f1 = suite->add_family( "f1" );
      f1->add_task("t1");
      f1->add_task("t2");
      family_ptr f2 = f1->add_family( "f2" );
      f2->add_task("t1");
      f2->add_task("t2");
      family_ptr f3 = f2->add_family( "f3" );
      f3->add_task("t1");
      f3->add_task("t2");
      family_ptr f4= f3->add_family( "f4" );
      f4->add_task("t1");
      f4->add_task("t2");
      TestHelper::invokeRequest(&theDefs,Cmd_ptr( new BeginCmd(suite->absNodePath(),false))); // al nodes wil be active
   }
   //cout << theDefs << "\n";


   // ****************************************************************************************
   // *** Each time we restore, we have a new set of pointers *FOR* the children of archived/restored NODE
   // *** Hence use paths
   // ****************************************************************************************
   std::vector<node_ptr> nodes;
   theDefs.get_all_nodes(nodes);
   std::vector<std::string> nc_vec;
   for(size_t i = 0; i < nodes.size(); i++) {
       NodeContainer* nc = nodes[i]->isNodeContainer();
       if (!nc) continue;
       nc_vec.push_back(nc->absNodePath());
   }

   for(size_t i = 0; i < nc_vec.size(); i++) {
      // cout << "archive and restore " << nc_vec[i]  << "\n";
      {
         TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,nc_vec[i],true))); // use force since jobs are active
         node_ptr node = theDefs.findAbsNode(nc_vec[i]);
         BOOST_REQUIRE_MESSAGE(node,"Could not find node " << nc_vec[i] );
         NodeContainer* nc = node->isNodeContainer();
         BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(fs::exists(nc->archive_path()),"Archive path" << nc->archive_path() << " not created");
         BOOST_CHECK_MESSAGE( nc->nodeVec().empty(),"Children not removed " << nc->absNodePath());

         TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,nc_vec[i] )));
         node = theDefs.findAbsNode(nc_vec[i]);
         BOOST_REQUIRE_MESSAGE(node,"Could not find node " << nc_vec[i] );
         nc = node->isNodeContainer();
         BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::RESTORED),"restored flag should be set " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!fs::exists(nc->archive_path()),"Archived file has not been deleted after restore " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!nc->nodeVec().empty(),"Children are not restored " << nc->absNodePath());
      }
      {
         // Archive again but restore via re-queue
         TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,nc_vec[i],true)));
         node_ptr node = theDefs.findAbsNode(nc_vec[i]);
         BOOST_REQUIRE_MESSAGE(node,"Could not find node " << nc_vec[i] );
         NodeContainer* nc = node->isNodeContainer();
         BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::RESTORED),"Restored flag should be clear " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(fs::exists(nc->archive_path()),"Archive path" << nc->archive_path() << " not created");
         BOOST_CHECK_MESSAGE( nc->nodeVec().empty(),"Children not removed " << nc->absNodePath());

         TestHelper::invokeRequest(&theDefs,Cmd_ptr( new RequeueNodeCmd(nc_vec[i],RequeueNodeCmd::FORCE)));
         node = theDefs.findAbsNode(nc_vec[i]);
         BOOST_REQUIRE_MESSAGE(node,"Could not find node " << nc_vec[i] );
         nc = node->isNodeContainer();
         BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::RESTORED),"Restored flag not *cleared " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!fs::exists(nc->archive_path()),"Archived file has not been deleted after restore " << nc->absNodePath());
         BOOST_CHECK_MESSAGE(!nc->nodeVec().empty(),"Children are not restored " << nc->absNodePath());
      }
   }
   //cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE( test_archive_and_restore_overlap )
{
   cout << "Base:: ...test_archive_and_restore_overlap\n";

   // Create the defs file corresponding to the text below
   //suite suite
   //  edit ECF_HOME ....
   //  family f1
   //     task t1

   Defs theDefs;
   std::string ecf_home = File::test_data("Base/test","Base");
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);
   suite_ptr suite = theDefs.add_suite( Pid::unique_name("test_archive_and_restore_overlap")  );
   std::string f1_abs_node_path;
   {
      family_ptr f1 = suite->add_family( "f1" );
      f1->add_task("t1");
      f1_abs_node_path = f1->absNodePath();
   }

   // Archive suite and its immediate child. This should be detected and we should only archive the suite and not the family
   std::vector<std::string> paths;
   paths.push_back(suite->absNodePath());  // archive suite
   paths.push_back(f1_abs_node_path);      // archive family its child at the same time, should be ignored

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::ARCHIVE,paths))); // family_ptr f1 removed from the suite
   BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not set");
   BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()),"Archive path " << suite->archive_path() << " not created");
   BOOST_CHECK_MESSAGE(suite->nodeVec().empty(),"Children not removed");
   BOOST_CHECK_MESSAGE(!theDefs.findAbsNode(f1_abs_node_path),"f1 should have been removed");

   TestHelper::invokeRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,suite->absNodePath())));
   BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::RESTORED),"Restored flag not set");
   BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED),"Archived flag not *cleared");
   BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()),"Archived file has not been deleted after restore");
   BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(),"Children are not restored");
   node_ptr f1 = theDefs.findAbsNode(f1_abs_node_path);
   BOOST_CHECK_MESSAGE(f1,"f1 should have been restored");
   BOOST_CHECK_MESSAGE(!f1->flag().is_set(ecf::Flag::RESTORED),"Family f1 should not have restored flag set");
}

BOOST_AUTO_TEST_CASE( test_archive_and_restore_errors )
{
   cout << "Base:: ...test_archive_and_restore_errors\n";

   // Create the defs file corresponding to the text below
   //suite suite
   //  edit ECF_HOME ....
   //  family f1
   //     task t1

   // We use Pid::unique_name, to allow multiple invocation of this test
   Defs theDefs;
   std::string ecf_home = File::test_data("Base/test","Base");
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);
   suite_ptr suite = theDefs.add_suite( Pid::unique_name("test_archive_and_restore_errors")  );
   family_ptr f1 = suite->add_family( "f1" );
   f1->add_task("t1");

   // FLAG ecf::Flag::ARCHIVED should be set, for restore to work
   TestHelper::invokeFailureRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,suite->absNodePath())));

   // set  ecf::Flag::ARCHIVED, but now suite has children, the suite should have no children for restore
   suite->flag().set(ecf::Flag::ARCHIVED);
   TestHelper::invokeFailureRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,suite->absNodePath())));

   // remove the suite children, should no fail since the restore file is missing
   f1->remove();
   TestHelper::invokeFailureRequest(&theDefs,Cmd_ptr( new PathsCmd(PathsCmd::RESTORE,suite->absNodePath())));

   /// Destroy singleton's to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

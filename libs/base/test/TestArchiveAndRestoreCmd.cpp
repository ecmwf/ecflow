/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/DeleteCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ArchiveAndRestoreCmd)

BOOST_AUTO_TEST_CASE(test_add_log4) {
    // create once for all test below, then remove at the end
    Log::create("test_add_log4.log");
    BOOST_CHECK_MESSAGE(true, "stop boost test form complaining");
}

BOOST_AUTO_TEST_CASE(test_archive_and_restore_suite) {
    ECF_NAME_THIS_TEST();

    // Please note: after archive we delete the children, hence any references to child refer to a different object

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     task t1

    // We use Pid::unique_name, to allow multiple invocation of this test
    Defs theDefs;
    std::string ecf_home = File::test_data("libs/base/test", "libs/base");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
    suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_archive_and_restore_suite"));
    suite->add_family("f1")->add_task("t1");
    // cout << theDefs << "\n";

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, suite->absNodePath())));
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set");
    BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()), "Archive path" << suite->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(suite->nodeVec().empty(), "Children not removed");
    // cout << theDefs << "\n";

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, suite->absNodePath())));
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::RESTORED), "Restored flag not set");
    BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not *cleared");
    BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()), "Archived file has not been deleted after restore");
    BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(), "Children are not restored");

    // Archive again but restore via begin
    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, suite->absNodePath())));
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set");
    BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()), "Archive path" << suite->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(suite->nodeVec().empty(), "Children not removed");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new BeginCmd(suite->absNodePath(), true))); // nodes will be active
    BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not *cleared");
    BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::RESTORED), "restored flag not *cleared");
    BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()), "Archived file has not been deleted after restore");
    BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(), "Children are not restored");

    // PrintStyle::setStyle(PrintStyle::MIGRATE);
    // cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE(test_archive_and_restore_family) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     family f2
    //       family f3
    //          task t1

    Defs theDefs;
    std::string ecf_home = File::test_data("libs/base/test", "libs/base");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
    suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_archive_and_restore_family"));
    family_ptr f3   = suite->add_family("f1")->add_family("f2")->add_family("f3");
    f3->add_task("t1");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, f3->absNodePath())));
    BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set");
    BOOST_CHECK_MESSAGE(fs::exists(f3->archive_path()), "Archive path" << f3->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(f3->nodeVec().empty(), "Children not removed");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, f3->absNodePath())));
    BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::RESTORED), "restored flag not set");
    BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not *cleared");
    BOOST_CHECK_MESSAGE(!fs::exists(f3->archive_path()), "Archived file has not been deleted after restore");
    BOOST_CHECK_MESSAGE(!f3->nodeVec().empty(), "Children are not restored");

    // Archive again but restore via begin
    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, f3->absNodePath())));
    BOOST_CHECK_MESSAGE(f3->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set");
    BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::RESTORED), "Restored flag not cleared");
    BOOST_CHECK_MESSAGE(fs::exists(f3->archive_path()), "Archive path" << suite->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(f3->nodeVec().empty(), "Children not removed");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new BeginCmd(suite->absNodePath(), true))); // nodes will be active
    BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not *cleared");
    BOOST_CHECK_MESSAGE(!f3->flag().is_set(ecf::Flag::RESTORED), "Restored flag not *cleared");
    BOOST_CHECK_MESSAGE(!fs::exists(f3->archive_path()), "Archived file has not been deleted after restore");
    BOOST_CHECK_MESSAGE(!f3->nodeVec().empty(), "Children are not restored");

    //   PrintStyle::setStyle(PrintStyle::MIGRATE);
    //   cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE(test_archive_and_restore_all) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     task t1; task t2
    //     family f2
    //       task t1; task t2
    //       family f3
    //         task t1; task t2
    //         family f4
    //           task t1; task t2

    Defs theDefs;
    {
        std::string ecf_home = File::test_data("libs/base/test", "libs/base");
        theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
        suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_archive_and_restore_all"));
        family_ptr f1   = suite->add_family("f1");
        f1->add_task("t1");
        f1->add_task("t2");
        family_ptr f2 = f1->add_family("f2");
        f2->add_task("t1");
        f2->add_task("t2");
        family_ptr f3 = f2->add_family("f3");
        f3->add_task("t1");
        f3->add_task("t2");
        family_ptr f4 = f3->add_family("f4");
        f4->add_task("t1");
        f4->add_task("t2");
        TestHelper::invokeRequest(&theDefs,
                                  Cmd_ptr(new BeginCmd(suite->absNodePath(), false))); // al nodes wil be active
    }
    // cout << theDefs << "\n";

    // ****************************************************************************************
    // *** Each time we restore, we have a new set of pointers *FOR* the children of archived/restored NODE
    // *** Hence use paths
    // ****************************************************************************************
    std::vector<node_ptr> nodes;
    theDefs.get_all_nodes(nodes);
    std::vector<std::string> nc_vec;
    for (auto& node : nodes) {
        NodeContainer* nc = node->isNodeContainer();
        if (!nc)
            continue;
        nc_vec.push_back(nc->absNodePath());
    }

    for (const auto& i : nc_vec) {
        // cout << "archive and restore " << nc_vec[i]  << "\n";
        {
            TestHelper::invokeRequest(
                &theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, i, true))); // use force since jobs are active
            node_ptr node = theDefs.findAbsNode(i);
            BOOST_REQUIRE_MESSAGE(node, "Could not find node " << i);
            NodeContainer* nc = node->isNodeContainer();
            BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(fs::exists(nc->archive_path()), "Archive path" << nc->archive_path() << " not created");
            BOOST_CHECK_MESSAGE(nc->nodeVec().empty(), "Children not removed " << nc->absNodePath());

            TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, i)));
            node = theDefs.findAbsNode(i);
            BOOST_REQUIRE_MESSAGE(node, "Could not find node " << i);
            nc = node->isNodeContainer();
            BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::RESTORED),
                                "restored flag should be set " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::ARCHIVED),
                                "Archived flag not *cleared " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!fs::exists(nc->archive_path()),
                                "Archived file has not been deleted after restore " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!nc->nodeVec().empty(), "Children are not restored " << nc->absNodePath());
        }
        {
            // Archive again but restore via re-queue
            TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, i, true)));
            node_ptr node = theDefs.findAbsNode(i);
            BOOST_REQUIRE_MESSAGE(node, "Could not find node " << i);
            NodeContainer* nc = node->isNodeContainer();
            BOOST_CHECK_MESSAGE(nc->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::RESTORED),
                                "Restored flag should be clear " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(fs::exists(nc->archive_path()), "Archive path" << nc->archive_path() << " not created");
            BOOST_CHECK_MESSAGE(nc->nodeVec().empty(), "Children not removed " << nc->absNodePath());

            TestHelper::invokeRequest(&theDefs, Cmd_ptr(new RequeueNodeCmd(i, RequeueNodeCmd::FORCE)));
            node = theDefs.findAbsNode(i);
            BOOST_REQUIRE_MESSAGE(node, "Could not find node " << i);
            nc = node->isNodeContainer();
            BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::RESTORED),
                                "Restored flag not *cleared " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!nc->flag().is_set(ecf::Flag::ARCHIVED),
                                "Archived flag not *cleared " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!fs::exists(nc->archive_path()),
                                "Archived file has not been deleted after restore " << nc->absNodePath());
            BOOST_CHECK_MESSAGE(!nc->nodeVec().empty(), "Children are not restored " << nc->absNodePath());
        }
    }
    // cout << theDefs << "\n";
}

BOOST_AUTO_TEST_CASE(test_archive_and_restore_overlap) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     task t1

    Defs theDefs;
    std::string ecf_home = File::test_data("libs/base/test", "libs/base");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
    suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_archive_and_restore_overlap"));
    std::string f1_abs_node_path;
    {
        family_ptr f1 = suite->add_family("f1");
        f1->add_task("t1");
        f1_abs_node_path = f1->absNodePath();
    }

    // Archive suite and its immediate child. This should be detected and we should only archive the suite and not the
    // family
    std::vector<std::string> paths;
    paths.push_back(suite->absNodePath()); // archive suite
    paths.push_back(f1_abs_node_path);     // archive family its child at the same time, should be ignored

    TestHelper::invokeRequest(&theDefs,
                              Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, paths))); // family_ptr f1 removed from the suite
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set");
    BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()), "Archive path " << suite->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(suite->nodeVec().empty(), "Children not removed");
    BOOST_CHECK_MESSAGE(!theDefs.findAbsNode(f1_abs_node_path), "f1 should have been removed");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, suite->absNodePath())));
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::RESTORED), "Restored flag not set");
    BOOST_CHECK_MESSAGE(!suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not *cleared");
    BOOST_CHECK_MESSAGE(!fs::exists(suite->archive_path()), "Archived file has not been deleted after restore");
    BOOST_CHECK_MESSAGE(!suite->nodeVec().empty(), "Children are not restored");
    node_ptr f1 = theDefs.findAbsNode(f1_abs_node_path);
    BOOST_CHECK_MESSAGE(f1, "f1 should have been restored");
    BOOST_CHECK_MESSAGE(!f1->flag().is_set(ecf::Flag::RESTORED), "Family f1 should not have restored flag set");
}

BOOST_AUTO_TEST_CASE(test_archive_and_delete_suite) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     task t1
    // Here we are archiving the family and then the suite
    // We then delete the suite and check that both archived files are removed.

    // We use Pid::unique_name, to allow multiple invocation of this test
    Defs theDefs;
    std::string ecf_home = File::test_data("libs/base/test", "libs/base");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
    suite_ptr suite   = theDefs.add_suite(Pid::unique_name("test_archive_and_delete_suite"));
    family_ptr family = suite->add_family("f1");
    family->add_task("t1");
    // cout << theDefs << "\n";

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, family->absNodePath())));
    BOOST_CHECK_MESSAGE(family->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set on family");
    BOOST_CHECK_MESSAGE(family->has_archive(), "Archived flag not set on family");
    std::string family_archive_path = family->archive_path();
    BOOST_CHECK_MESSAGE(fs::exists(family_archive_path), "Archive path" << family->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(family->nodeVec().empty(), "Children of family not removed after archive");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::ARCHIVE, suite->absNodePath())));
    BOOST_CHECK_MESSAGE(suite->flag().is_set(ecf::Flag::ARCHIVED), "Archived flag not set on suite");
    BOOST_CHECK_MESSAGE(suite->has_archive(), "Archived flag not set on family");
    std::string suite_archive_path = suite->archive_path();
    BOOST_CHECK_MESSAGE(fs::exists(suite_archive_path), "Archive path" << suite->archive_path() << " not created");
    BOOST_CHECK_MESSAGE(suite->nodeVec().empty(), "Children of suite not removed after archive");

    TestHelper::invokeRequest(&theDefs, Cmd_ptr(new DeleteCmd(suite->absNodePath())));
    BOOST_CHECK_MESSAGE(!fs::exists(suite_archive_path), "Suite Archived file not removed after DeleteCmd");
    BOOST_CHECK_MESSAGE(!fs::exists(family_archive_path), "Family Archived file not removed after DeleteCmd");
}

BOOST_AUTO_TEST_CASE(test_archive_and_restore_errors) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_HOME ....
    //  family f1
    //     task t1

    // We use Pid::unique_name, to allow multiple invocation of this test
    Defs theDefs;
    std::string ecf_home = File::test_data("libs/base/test", "libs/base");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
    suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_archive_and_restore_errors"));
    family_ptr f1   = suite->add_family("f1");
    f1->add_task("t1");

    // FLAG ecf::Flag::ARCHIVED should be set, for restore to work
    TestHelper::invokeFailureRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, suite->absNodePath())));

    // set  ecf::Flag::ARCHIVED, but now suite has children, the suite should have no children for restore
    suite->flag().set(ecf::Flag::ARCHIVED);
    TestHelper::invokeFailureRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, suite->absNodePath())));

    // remove the suite children, should no fail since the restore file is missing
    f1->remove();
    TestHelper::invokeFailureRequest(&theDefs, Cmd_ptr(new PathsCmd(PathsCmd::RESTORE, suite->absNodePath())));

    /// Destroy singleton's to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_CASE(test_destroy_log4) {
    Log::destroy();
    fs::remove("test_add_log4.log");
    BOOST_CHECK_MESSAGE(true, "stop boost test form complaining");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

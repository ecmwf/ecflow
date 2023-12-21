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

#include "MockServer.hpp"
#include "MyDefsFixture.hpp"
#include "ecflow/base/cts/user/DeleteCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_DeleteNodeCmd)

BOOST_AUTO_TEST_CASE(test_delete_node_cmd) {
    cout << "Base:: ...test_delete_node_cmd\n";
    TestLog test_log(
        "test_delete_node_cmd.log"); // will create log file, and destroy log and remove file at end of scope

    MyDefsFixture fixtureDef;
    MockServer mockServer(&fixtureDef.defsfile_);

    // IMPORTANT: ******************************************************************************
    // If the DeleteCmd is given a EMPTY list of paths, it will delete *EVERYTHING*
    // *****************************************************************************************

    // Delete all Aliases
    {
        std::vector<alias_ptr> vec;
        fixtureDef.defsfile_.get_all_aliases(vec);
        BOOST_CHECK_MESSAGE(vec.size() > 0, "Expected > 0 aliases but found " << vec.size());

        std::vector<std::string> paths;
        paths.reserve(vec.size());
        for (alias_ptr t : vec) {
            paths.push_back(t->absNodePath());
        }

        size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
        BOOST_CHECK_MESSAGE(!paths.empty(), "Expected paths to be specified, *OTHERWISE* we delete all nodes");
        DeleteCmd cmd(paths);
        cmd.setup_user_authentification();
        STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
        BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to delete aliases");
        {
            // ECFLOW-434
            const std::vector<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
            size_t edit_history_size_after               = edit_history_size_before + paths.size();
            if (edit_history_size_after > Defs::max_edit_history_size_per_node())
                edit_history_size_after = Defs::max_edit_history_size_per_node();
            BOOST_CHECK_MESSAGE(edit_history.size() == edit_history_size_after,
                                "Expected   " << edit_history_size_after << " edit history but found "
                                              << edit_history.size());
        }

        std::vector<alias_ptr> afterDeleteVec;
        fixtureDef.defsfile_.get_all_aliases(afterDeleteVec);
        BOOST_REQUIRE_MESSAGE(afterDeleteVec.empty(),
                              "Expected all aliases to be deleted but found " << afterDeleteVec.size());
    }

    // Delete all tasks
    {
        std::vector<Task*> vec;
        fixtureDef.defsfile_.getAllTasks(vec);
        BOOST_CHECK_MESSAGE(vec.size() > 0, "Expected > 0 tasks but found " << vec.size());

        std::vector<std::string> paths;
        paths.reserve(vec.size());
        for (Task* t : vec) {
            paths.push_back(t->absNodePath());
        }

        BOOST_CHECK_MESSAGE(!paths.empty(), "Expected paths to be specified, *OTHERWISE* we delete all nodes");
        size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
        DeleteCmd cmd(paths);
        cmd.setup_user_authentification();
        STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
        BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to delete tasks");
        {
            const std::vector<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
            size_t edit_history_size_after               = edit_history_size_before + paths.size();
            if (edit_history_size_after > Defs::max_edit_history_size_per_node())
                edit_history_size_after = Defs::max_edit_history_size_per_node();
            BOOST_CHECK_MESSAGE(edit_history.size() == edit_history_size_after,
                                "Expected   " << edit_history_size_after << " edit history but found "
                                              << edit_history.size());
        }

        std::vector<Task*> afterDeleteVec;
        fixtureDef.defsfile_.getAllTasks(afterDeleteVec);
        BOOST_REQUIRE_MESSAGE(afterDeleteVec.empty(),
                              "Expected all tasks to be deleted but found " << afterDeleteVec.size());
    }

    // Delete all Families
    {
        // DONT use getAllFamilies as this will return hierarchical families
        // we don't want to delete families twice
        //		std::vector<Family*> vec;
        //		fixtureDef.defsfile_.getAllFamilies(vec);
    }

    // Delete all Suites
    {
        std::vector<suite_ptr> vec = fixtureDef.defsfile_.suiteVec();
        BOOST_CHECK_MESSAGE(vec.size() > 0, "Expected > 0 Suites but found " << vec.size());
        for (suite_ptr s : vec) {
            {
                // Delete all Families
                // *********************************************************************************************
                // **EXPLICITLY** check for empty paths otherwise we can end up deleting ALL suites accidentally
                // if DeleteCmd is given an empty list, we will delete all nodes including the suites
                // *********************************************************************************************
                std::vector<family_ptr> familyVec = s->familyVec();
                // 	DONT USE:
                //        for(family_ptr f: s->familyVec()) {
                //  As with this will invalidate the iterators.
                std::vector<std::string> paths;
                paths.reserve(vec.size());
                for (family_ptr f : familyVec) {
                    paths.push_back(f->absNodePath());
                }

                if (!paths.empty()) {
                    size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
                    DeleteCmd cmd(paths);
                    cmd.setup_user_authentification();
                    STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
                    BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to delete families");
                    BOOST_REQUIRE_MESSAGE(s->familyVec().empty(),
                                          "Expected all Families to be deleted but found " << s->familyVec().size());
                    {
                        const std::vector<std::string>& edit_history =
                            fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
                        size_t edit_history_size_after = edit_history_size_before + paths.size();
                        if (edit_history_size_after > Defs::max_edit_history_size_per_node())
                            edit_history_size_after = Defs::max_edit_history_size_per_node();
                        BOOST_CHECK_MESSAGE(edit_history.size() == edit_history_size_after,
                                            "Expected   " << edit_history_size_after << " edit history but found "
                                                          << edit_history.size());
                    }
                }
            }

            // delete the suite
            size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
            std::string absNodePath         = s->absNodePath();
            DeleteCmd cmd(absNodePath);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to delete suite at path " << absNodePath);
            {
                const std::vector<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
                size_t edit_history_size_after               = edit_history_size_before + 1;
                if (edit_history_size_after > Defs::max_edit_history_size_per_node())
                    edit_history_size_after = Defs::max_edit_history_size_per_node();
                BOOST_CHECK_MESSAGE(edit_history.size() == edit_history_size_after,
                                    "Expected   " << edit_history_size_after << " edit history but found "
                                                  << edit_history.size());
            }
        }

        BOOST_REQUIRE_MESSAGE(fixtureDef.defsfile_.suiteVec().empty(),
                              "Expected all Suites to be deleted but found " << fixtureDef.defsfile_.suiteVec().size());
    }

    {
        // Clear edit history
        fixtureDef.defsfile_.clear_edit_history();
        BOOST_CHECK_MESSAGE(fixtureDef.defsfile_.get_edit_history().size() == 0, "Expected edit history to be empty");
    }
}

BOOST_AUTO_TEST_CASE(test_delete_node_edit_history_ECFLOW_1684) {
    cout << "Base:: ...test_delete_node_edit_history_ECFLOW_1684\n";

    // This test will ensure that if a suite/family node is deleted, we *remove* any *OLD* edit history associated
    // with the node, AND and of its children

    // Create log file for the commands to work. auto destroy at end of scope
    TestLog test_log("test_delete_node_edit_history.log");

    MyDefsFixture fixtureDef;
    MockServer mockServer(&fixtureDef.defsfile_);

    // suspend all suites and tasks, then check we have some edit history
    {
        size_t edit_history_root_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
        size_t edit_history_before           = fixtureDef.defsfile_.get_edit_history().size();
        BOOST_CHECK_MESSAGE(edit_history_root_size_before == 0,
                            "Expected edit_history_root_size_before == 0 but found " << edit_history_root_size_before);
        BOOST_CHECK_MESSAGE(edit_history_before == 0,
                            "Expected edit_history_before == 0 but found " << edit_history_before);

        std::vector<Task*> task_vec;
        fixtureDef.defsfile_.getAllTasks(task_vec);
        BOOST_CHECK_MESSAGE(task_vec.size() > 0, "Expected > 0 tasks but found " << task_vec.size());
        std::vector<std::string> paths;
        paths.reserve(task_vec.size());
        for (Task* t : task_vec) {
            paths.push_back(t->absNodePath());
        }
        BOOST_CHECK_MESSAGE(!paths.empty(), "Expected paths to be specified, *OTHERWISE* we delete all nodes");

        std::vector<suite_ptr> suite_vec = fixtureDef.defsfile_.suiteVec();
        for (suite_ptr s : suite_vec) {
            paths.push_back(s->absNodePath());
        }

        PathsCmd cmd(PathsCmd::SUSPEND, paths);
        cmd.setup_user_authentification();
        STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
        BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to suspend tasks");
        {
            size_t edit_history_root_size_after = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
            size_t edit_history_after           = fixtureDef.defsfile_.get_edit_history().size();

            BOOST_CHECK_MESSAGE(edit_history_root_size_after == 0,
                                "Expected edit_history_root_size_after == 0 but found "
                                    << edit_history_root_size_after);
            BOOST_CHECK_MESSAGE(edit_history_after == paths.size(),
                                "Expected edit_history_after to be same number of suspended tasks and suites("
                                    << paths.size() << ") but found " << edit_history_after);
        }
    }

    // Delete all Suites, this should delete edit history associated with suites and child tasks
    {
        std::vector<suite_ptr> vec = fixtureDef.defsfile_.suiteVec();
        BOOST_CHECK_MESSAGE(vec.size() > 0, "Expected > 0 Suites but found " << vec.size());
        for (suite_ptr s : vec) {
            std::string absNodePath = s->absNodePath();
            DeleteCmd cmd(absNodePath);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_CHECK_MESSAGE(returnCmd->ok(), "Failed to delete suite at path " << absNodePath);
        }
        BOOST_REQUIRE_MESSAGE(fixtureDef.defsfile_.suiteVec().empty(),
                              "Expected all Suites to be deleted but found " << fixtureDef.defsfile_.suiteVec().size());

        size_t edit_history_root_size_after = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
        size_t edit_history_after           = fixtureDef.defsfile_.get_edit_history().size();

        BOOST_CHECK_MESSAGE(edit_history_root_size_after == vec.size(),
                            "Expected edit_history size of root node to be same as number of deleted suites("
                                << vec.size() << ") but found " << edit_history_root_size_after);
        BOOST_CHECK_MESSAGE(edit_history_after == 1,
                            "Expected edit_history_after == 1 to *ONLY* have root node, but found "
                                << edit_history_after << "\n"
                                << fixtureDef.defsfile_.dump_edit_history());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

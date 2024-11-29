/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/NOrder.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

///
///    This test will TEST:
///       o Alias creation and running
///       o Alias ordering
///       o Alias deletion
///    Will indirectly test the Alias memento's
///

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Alias)

void wait_for_alias_to_complete(const std::string& alias_path) {
    AssertTimer assertTimer(10, false); // Bomb out after 10 seconds, fall back if test fail
    while (1) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs  = TestFixture::client().defs();
        node_ptr alias = defs->findAbsNode(alias_path);
        BOOST_REQUIRE_MESSAGE(alias.get(),
                              "Could not locate created alias at path " << alias_path << "\n"
                                                                        << TestFixture::client().errorMsg());
        if (alias->state() == NState::COMPLETE)
            break;
        sleep(2);
    }
}

BOOST_AUTO_TEST_CASE(test_alias) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Create the defs file corresponding to the text below
    // ECF_HOME variable is automatically added by the test harness.
    // ECF_INCLUDE variable is automatically added by the test harness.
    // SLEEPTIME variable is automatically added by the test harness.
    // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
    //                     This is substituted in ecf includes
    //                     Allows test to run without requiring installation
    Defs theDefs;
    task_ptr task_a;
    {
        suite_ptr suite = theDefs.add_suite("test_alias");
        suite->add_variable("SLEEPTIME", "0");
        task_a = suite->add_task("task_a");
        task_a->addMeter(Meter("meter", 0, 20, 20));
        task_a->addEvent(Event(1, "event"));
        task_a->addLabel(Label("task_a_label", "Label1"));
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_alias.def"));

    // After the task has completed create a Alias
    TestFixture::client().set_throw_on_error(false);

    // Get the ecf script from the current task and use it to create alias
    BOOST_REQUIRE_MESSAGE(TestFixture::client().file(
                              task_a->absNodePath(), CFileCmd::toString(CFileCmd::ECF), "10000") == 0,
                          "Expected to retreive script\n"
                              << TestFixture::client().errorMsg());
    std::string script = TestFixture::client().get_string();
    BOOST_CHECK_MESSAGE(!script.empty(), "script for task " << task_a->absNodePath() << " is empty");

    // TEST Alias CREATION and running =============================================================================
    // Split the file into line
    std::vector<std::string> script_lines;
    Str::split(script, script_lines, "\n");

    NameValueVec used_variables;
    int result = TestFixture::client().edit_script_submit(
        task_a->absNodePath(), used_variables, script_lines, true /*create alias*/, true /*run alias*/);
    BOOST_REQUIRE_MESSAGE(result == 0,
                          "Expected alias creation and run to succeed\n"
                              << TestFixture::client().errorMsg());
    std::string alias0_path = task_a->absNodePath() + "/alias0";

    // Wait for alias to complete.
    wait_for_alias_to_complete(alias0_path);

    // #ifdef DEBUG
    //    BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "Could not get the defs from server\n" <<
    //    TestFixture::client().errorMsg()); defs_ptr defs = TestFixture::client().defs(); PrintStyle
    //    style(PrintStyle::STATE); std::cout << *defs;
    // #endif

    // TEST ORDERING:: Create another alias, but don't run it.
    // =============================================================================
    result = TestFixture::client().edit_script_submit(
        task_a->absNodePath(), used_variables, script_lines, true /*create alias*/, false /*run alias*/);
    BOOST_REQUIRE_MESSAGE(result == 0, "Expected alias creation succeed\n" << TestFixture::client().errorMsg());
    {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        std::vector<alias_ptr> aliases;
        defs->get_all_aliases(aliases);
        BOOST_REQUIRE_MESSAGE(aliases.size() == 2, "Expected 2 aliases\n" << TestFixture::client().errorMsg());
    }

    BOOST_REQUIRE_MESSAGE(TestFixture::client().order(alias0_path, NOrder::toString(NOrder::DOWN)) == 0,
                          "Expected order NOrder::DOWN to succeed\n"
                              << TestFixture::client().errorMsg());
    {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        Task* task    = defs->findAbsNode(task_a->absNodePath())->isTask();
        BOOST_REQUIRE_MESSAGE(task, "expected to find Task");

        std::vector<std::string> expected;
        expected.push_back("alias1");
        expected.push_back("alias0");
        BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                              "NOrder::DOWN expected "
                                  << ecf::algorithm::join(expected) << " but found "
                                  << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));
    }

    BOOST_REQUIRE_MESSAGE(TestFixture::client().order(alias0_path, NOrder::toString(NOrder::UP)) == 0,
                          "Expected order NOrder::UP to succeed\n"
                              << TestFixture::client().errorMsg());
    {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        Task* task    = defs->findAbsNode(task_a->absNodePath())->isTask();
        BOOST_REQUIRE_MESSAGE(task, "expected to find Task");

        std::vector<std::string> expected;
        expected.push_back("alias0");
        expected.push_back("alias1");
        BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                              "NOrder::UP expected "
                                  << ecf::algorithm::join(expected) << " but found "
                                  << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));
    }

    BOOST_REQUIRE_MESSAGE(TestFixture::client().order(alias0_path, NOrder::toString(NOrder::ORDER)) == 0,
                          "Expected order NOrder::ORDER to succeed\n"
                              << TestFixture::client().errorMsg());
    {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        Task* task    = defs->findAbsNode(task_a->absNodePath())->isTask();
        BOOST_REQUIRE_MESSAGE(task, "expected to find Task");

        std::vector<std::string> expected;
        expected.push_back("alias1");
        expected.push_back("alias0");
        BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                              "NOrder::ORDER expected "
                                  << ecf::algorithm::join(expected) << " but found "
                                  << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));
    }

    BOOST_REQUIRE_MESSAGE(TestFixture::client().order(alias0_path, NOrder::toString(NOrder::ALPHA)) == 0,
                          "Expected order NOrder::ALPHA to succeed\n"
                              << TestFixture::client().errorMsg());
    {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "Could not get the defs from server\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        Task* task    = defs->findAbsNode(task_a->absNodePath())->isTask();
        BOOST_REQUIRE_MESSAGE(task, "expected to find Task");

        std::vector<std::string> expected;
        expected.push_back("alias0");
        expected.push_back("alias1");
        BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                              "NOrder::ALPHA expected "
                                  << ecf::algorithm::join(expected) << " but found "
                                  << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));
    }

    // TEST Alias DELETION =============================================================================
    std::string alias1_path = task_a->absNodePath() + "/alias1";
    BOOST_REQUIRE_MESSAGE(TestFixture::client().delete_node(alias0_path) == 0,
                          "delete alias0 failed\n"
                              << TestFixture::client().errorMsg());
    BOOST_REQUIRE_MESSAGE(TestFixture::client().delete_node(alias1_path) == 0,
                          "delete alias1 failed\n"
                              << TestFixture::client().errorMsg());

    // Get the defs from the server
    BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                          "Could not get the defs from server\n"
                              << TestFixture::client().errorMsg());
    defs_ptr defs = TestFixture::client().defs();
    std::vector<alias_ptr> aliases;
    defs->get_all_aliases(aliases);
    BOOST_REQUIRE_MESSAGE(aliases.empty(), "Alias deletion falied\n" << TestFixture::client().errorMsg());

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_CtsWaitCmd)

static void create_defs(Defs& theDefs, const std::string& suite_name) {
    // suite test_wait_cmd
    //   family family0
    //       task wait
    //   family family1
    //       task a
    //       task b
    //   endfamily
    //   family family2
    //          task aa
    //          task bb
    //    endfamily
    // endsuite
    suite_ptr suite = theDefs.add_suite(suite_name);
    family_ptr fam0 = suite->add_family("family0");
    task_ptr wait   = fam0->add_task("wait");
    wait->addVerify(VerifyAttr(NState::COMPLETE, 1)); // task should complete 1 times

    family_ptr fam1 = suite->add_family("family1");
    fam1->add_task("a");
    fam1->add_task("b");

    family_ptr fam2 = suite->add_family("family2");
    fam2->add_task("aa");
    fam2->add_task("bb");
}

static bool wait_for_state(std::vector<std::pair<std::string, NState::State>>& path_state_vec, int max_time_to_wait) {
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (true) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "sync_local failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs      = TestFixture::client().defs();
        bool all_states_ok = true;
        for (size_t i = 0; i < path_state_vec.size(); ++i) {
            node_ptr node = defs->findAbsNode(path_state_vec[i].first);
            BOOST_REQUIRE_MESSAGE(node,
                                  "Could not find path '" << path_state_vec[i].first << "' in the defs\n"
                                                          << ecf::as_string(*defs, PrintStyle::DEFS));
            if (node->state() != path_state_vec[i].second) {
                all_states_ok = false;
                break;
            }
        }
        if (all_states_ok) {
            return true;
        }

        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < assertTimer.timeConstraint(),
                                  "wait_for_state: Test wait " << assertTimer.duration()
                                                               << " taking longer than time constraint of "
                                                               << assertTimer.timeConstraint() << " aborting\n"
                                                               << ecf::as_string(*defs, PrintStyle::DEFS));
            break;
        }
        sleep(1);
    }
    return false;
}

// The time limit, in seconds, for the jobs to reach the expected states.
// A child command that fails to connect to the server retries after 10 seconds (in release builds), and so the
// limit allows each job to be delayed by a few retries.
static constexpr int job_state_timeout = 60;

// Runs the suite, where the job of task 'wait' uses the given ecf file, waits for the task to abort, and checks that
// the job output reports the expected cause
static void run_until_wait_task_aborts(const std::string& suite_name,
                                       const std::string& templateEcfFileForWait,
                                       const std::string& expected_cause) {
    DurationTimer timer;

    Defs theDefs;
    create_defs(theDefs, suite_name);

    // A single try, so that the abort is final, and no job is left running after the test
    const std::string wait_path = "/" + suite_name + "/family0/wait";
    theDefs.findAbsNode(wait_path)->addVariable(Variable("ECF_TRIES", "1"));

    std::map<std::string, std::string> taskEcfFileMap;
    taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs, "wait"), templateEcfFileForWait));

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs,
                          ServerTestHarness::testDataDefsLocation(suite_name + ".def"),
                          taskEcfFileMap,
                          1 /*timeout*/,
                          false /* do not wait for test to finish */);

    // wait for family1 and family2 to complete
    std::vector<std::pair<std::string, NState::State>> path_state_vec;
    path_state_vec.push_back(std::make_pair("/" + suite_name + "/family1", NState::COMPLETE));
    path_state_vec.push_back(std::make_pair("/" + suite_name + "/family2", NState::COMPLETE));
    wait_for_state(path_state_vec, job_state_timeout);

    // wait for 'wait' task to abort
    path_state_vec.clear();
    path_state_vec.push_back(std::make_pair(wait_path, NState::ABORTED));
    wait_for_state(path_state_vec, job_state_timeout);

    // the abort is caused by the wait command, as reported in the job output
    BOOST_REQUIRE_MESSAGE(TestFixture::client().file(wait_path, CFileCmd::toString(CFileCmd::JOBOUT)) == 0,
                          "Could not retrieve the job output of " << wait_path << "\n"
                                                                  << TestFixture::client().errorMsg());
    const std::string& jobout = TestFixture::client().get_string();
    BOOST_CHECK_MESSAGE(jobout.find(expected_cause) != std::string::npos,
                        "Expected the job output of " << wait_path << " to report '" << expected_cause << "'\n"
                                                      << jobout);

    std::cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount()
              << ")\n";
}

// Test the wait command. The Wait command is a child command.
// The wait command is provided an expression. This expression is evaluated
// in the server. If the evaluate returns false then the client should
// blocks until the evaluation is true.
// In this case the job associated with task 'wait' should block until the expression evaluates
// to true, which should be after the completion of all other tasks
BOOST_AUTO_TEST_CASE(test_wait_cmd) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    Defs theDefs;
    create_defs(theDefs, "test_wait_cmd");

    // Create a custom sms file for test_wait_cmd/family0/wait to invoke the child wait command
    // with an expression that forces it to block, until all other tasks complete
    std::string templateEcfFileForWait;
    templateEcfFileForWait += "%include <head.h>\n";
    templateEcfFileForWait += "\n";
    templateEcfFileForWait += "echo do some work\n";
    templateEcfFileForWait += "%ECF_CLIENT_EXE_PATH% --wait=\"../family1/a eq complete and ../family1/b eq complete "
                              "and ../family2/aa eq complete and ../family2/bb eq complete\"\n";
    templateEcfFileForWait += "\n";
    templateEcfFileForWait += "%include <tail.h>\n";

    // The test harness will create corresponding directory structure
    // Override the default ECF_ file, with our custom ECF_ file
    std::map<std::string, std::string> taskEcfFileMap;
    taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs, "wait"), templateEcfFileForWait));

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_wait_cmd.def"), taskEcfFileMap);

    std::cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount()
              << ")\n";
}

BOOST_AUTO_TEST_CASE(test_wait_cmd_parse_fail) {
    ECF_NAME_THIS_TEST();

    // This time we add a wait expression that
    // should fail to parse, and we should return an error
    // The error should captured by the trap in .ecf script, which will
    // then abort the task
    // clang-format off
    std::string templateEcfFileForWait =
        "%include <head.h>\n"
        "\n"
        "echo do some work\n"
        "%ECF_CLIENT_EXE_PATH% --wait=\"(((((((((../family1/a eq complete and ../family1/b eq complete and ../family2/aa eq complete and ../family2/bb eq complete\"\n"
        "\n"
        "%include <tail.h>\n";
    // clang-format on

    run_until_wait_task_aborts(
        "test_wait_cmd_parse_fail", templateEcfFileForWait, "CtsWaitCmd: Failed to parse expression");
}

BOOST_AUTO_TEST_CASE(test_wait_cmd_non_existant_paths) {
    ECF_NAME_THIS_TEST();

    // This time we add a wait expression that should fail
    // because the paths referenced in the expression do not exist
    // NOTE: ../family1/FRED does not exist
    // clang-format off
    std::string templateEcfFileForWait =
        "%include <head.h>\n"
        "\n"
        "echo do some work\n"
        "%ECF_CLIENT_EXE_PATH% --wait=\"../family1/FRED eq complete and ../family1/b eq complete and ../family2/aa eq complete and ../family2/bb eq complete\"\n"
        "\n"
        "%include <tail.h>\n";
    // clang-format on

    run_until_wait_task_aborts(
        "test_wait_cmd_non_existant_paths", templateEcfFileForWait, "Unrecognised path ../family1/FRED");
}

BOOST_AUTO_TEST_CASE(test_wait_cmd_when_job_starts_late) {
    ECF_NAME_THIS_TEST();

    // The job of task 'wait' starts 11 seconds late, which exceeds the period of 10 seconds after which a child
    // command retries a failed connection to the server.
    // The shebang is repeated on the first line, so that the job runs under bash, as with the shebang in head.h.
    // clang-format off
    std::string templateEcfFileForWait =
        "#!/usr/bin/env bash\n"
        "sleep 11\n"
        "%include <head.h>\n"
        "\n"
        "echo do some work\n"
        "%ECF_CLIENT_EXE_PATH% --wait=\"../family1/FRED eq complete\"\n"
        "\n"
        "%include <tail.h>\n";
    // clang-format on

    run_until_wait_task_aborts(
        "test_wait_cmd_when_job_starts_late", templateEcfFileForWait, "Unrecognised path ../family1/FRED");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

///
///    This test will TEST:
///       o The owner of a task is the user that loaded the suite, and is exposed as ECF_OWNER
///       o ECF_OWNER is substituted in the job command, and the value reaches the spawned command
///       o A requeue keeps the owner as the user that requeued
///

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Owner)

BOOST_AUTO_TEST_CASE(test_owner) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // suite test_owner
    //   edit SLEEPTIME 0
    //   edit ECF_JOB_CMD 'echo %ECF_OWNER% > %ECF_JOBOUT%.owner; %ECF_JOB% 1> %ECF_JOBOUT% 2>&1'
    //   task t1
    // endsuite
    // ECF_HOME, ECF_INCLUDE and ECF_CLIENT_EXE_PATH are added by the test harness.
    // The job command records ECF_OWNER, as substituted at spawn time, in a file next to the job output.
    Defs theDefs;
    task_ptr t1;
    {
        suite_ptr suite = theDefs.add_suite("test_owner");
        suite->add_variable("SLEEPTIME", "0");
        suite->add_variable("ECF_JOB_CMD", "echo %ECF_OWNER% > %ECF_JOBOUT%.owner; %ECF_JOB% 1> %ECF_JOBOUT% 2>&1");
        t1 = suite->add_task("t1");
        t1->addVerify(VerifyAttr(NState::COMPLETE, 1));
    }

    // The harness loads the suite as the login user, which becomes the owner of its tasks
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_owner.def"));

    const std::string user = get_login_name();
    BOOST_REQUIRE_MESSAGE(!user.empty(), "Expected a login name");

    // The server reports the owner, as the generated variable of the task
    BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "sync failed\n" << TestFixture::client().errorMsg());
    {
        node_ptr node = TestFixture::client().defs()->findAbsNode(t1->absNodePath());
        BOOST_REQUIRE_MESSAGE(node && node->isTask(), "Expected task " << t1->absNodePath());
        BOOST_CHECK_MESSAGE(node->isTask()->owner() == user,
                            "Expected owner '" << user << "', got '" << node->isTask()->owner() << "'");
        std::string value;
        BOOST_CHECK_MESSAGE(node->findParentVariableValue("ECF_OWNER", value) && value == user,
                            "Expected ECF_OWNER '" << user << "', got '" << value << "'");
    }

    // The job command received the owner: the file sits next to the job output, under the ECF_HOME of the suite
    const std::string ecf_home = theDefs.findSuite("test_owner")->findVariable(ecf::environment::ECF_HOME).value();
    BOOST_REQUIRE_MESSAGE(!ecf_home.empty(), "Expected ECF_HOME on the suite");
    auto owner_file = [&](int try_no) {
        return ecf_home + t1->absNodePath() + "." + ecf::convert_to<std::string>(try_no) + ".owner";
    };
    auto owner_recorded_by_the_job = [&](int try_no) {
        std::string path = owner_file(try_no);
        std::ifstream file(path);
        BOOST_REQUIRE_MESSAGE(file, "Expected the job command to write " << path);
        std::string owner;
        std::getline(file, owner);
        return owner;
    };
    BOOST_CHECK_MESSAGE(owner_recorded_by_the_job(1) == user,
                        "Expected the first job to run for '" << user << "', got '" << owner_recorded_by_the_job(1)
                                                              << "'");

    // A requeue by the same user runs the task again, for that user. A requeue resets the try number, so
    // the second run writes the same files as the first: remove the record and wait for it to come back.
    fs::remove(owner_file(1));
    TestFixture::client().set_throw_on_error(true);
    TestFixture::client().requeue(t1->absNodePath());
    {
        AssertTimer assertTimer(30, false);
        while (true) {
            BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                                  "sync failed\n"
                                      << TestFixture::client().errorMsg());
            node_ptr node = TestFixture::client().defs()->findAbsNode(t1->absNodePath());
            BOOST_REQUIRE_MESSAGE(node && node->isTask(), "Expected task " << t1->absNodePath());
            if (node->state() == NState::COMPLETE && fs::exists(owner_file(1))) {
                break;
            }
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < 30, "The task did not run again within 30 seconds");
            sleep(1);
        }
    }
    BOOST_CHECK_MESSAGE(owner_recorded_by_the_job(1) == user,
                        "Expected the second job to run for '" << user << "', got '" << owner_recorded_by_the_job(1)
                                                               << "'");

    // Submitting an edited script runs the task for the user submitting it (try number 2)
    BOOST_REQUIRE_MESSAGE(TestFixture::client().file(t1->absNodePath(), CFileCmd::toString(CFileCmd::ECF), "10000") ==
                              0,
                          "Expected to retrieve the script\n"
                              << TestFixture::client().errorMsg());
    std::vector<std::string> script_lines;
    ecf::algorithm::split_at(script_lines, TestFixture::client().get_string(), "\n");
    NameValueVec used_variables;
    BOOST_REQUIRE_MESSAGE(TestFixture::client().edit_script_submit(
                              t1->absNodePath(), used_variables, script_lines, false /*alias*/, true /*run*/) == 0,
                          "Expected the submission of the edited script to succeed\n"
                              << TestFixture::client().errorMsg());
    {
        AssertTimer assertTimer(30, false);
        while (true) {
            BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                                  "sync failed\n"
                                      << TestFixture::client().errorMsg());
            node_ptr node = TestFixture::client().defs()->findAbsNode(t1->absNodePath());
            BOOST_REQUIRE_MESSAGE(node && node->isTask(), "Expected task " << t1->absNodePath());
            if (node->isTask()->try_no() == 2 && node->state() == NState::COMPLETE && fs::exists(owner_file(2))) {
                break;
            }
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < 30, "The edited script did not run within 30 seconds");
            sleep(1);
        }
    }
    BOOST_CHECK_MESSAGE(owner_recorded_by_the_job(2) == user,
                        "Expected the edited script to run for '" << user << "', got '" << owner_recorded_by_the_job(2)
                                                                  << "'");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

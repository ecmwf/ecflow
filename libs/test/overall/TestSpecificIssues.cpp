// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_SpecificIssues)

BOOST_AUTO_TEST_CASE(test_ECFLOW_1589) {
    ECF_NAME_THIS_TEST();

    // Test ECF_JOB_CMD where task *completes* but the ECF_JOB_CMD still fails. i.e ECFLOW-1589

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Create the defs file corresponding to the text below
    // ECF_HOME variable is automatically added by the test harness.
    // ECF_INCLUDE variable is automatically added by the test harness.
    // SLEEPTIME variable is automatically added by the test harness.
    // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
    //                     This is substituted in sms includes
    //                     Allows test to run without requiring installation

    // # Note: we have to use relative paths, since these tests are relocatable
    //  suite test_ECFLOW_1589
    //     task t1
    //  endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_ECFLOW_1589");
        suite->add_task("t1")->addVerify(VerifyAttr(NState::COMPLETE, 1));
        // cout << theDefs;
    }

    // Create a custom ecf file for test_ECFLOW_1589/t1 that completes and then exits with a non-zero status
    std::string templateEcfFile;
    templateEcfFile += "%include <head.h>\n";
    templateEcfFile += "\n";
    templateEcfFile += "echo do some work\n";
    templateEcfFile += "\n";
    templateEcfFile += "%include <bad_tail.h>\n"; // this will exit 1

    // The test harness will create corresponding directory structure
    // Override the default ECF file, with our custom ECF_ file
    std::map<std::string, std::string> taskEcfFileMap;
    taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs, "t1"), templateEcfFile));

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_ECFLOW_1589.def"), taskEcfFileMap);

    // Since the job will call exit 1, i.e from bad_tail.h, we expect ecf::Flag::JOBCMD_FAILED
    // and since task t1 has *ALREADY* completed we expect ecf::Flag::ZOMBIE
    //
    // The flags are only set once the server 'reaps' (i.e. collects the exit status of) the
    // terminated ECF_JOB_CMD child, which happens in System::processTerminatedChildren() at the
    // end of the next job generation tick. The test harness returns as soon as the task is
    // complete, so poll for the flags for a bounded time.
    const int max_time_to_wait = std::max(10, 3 * TestFixture::job_submission_interval());
    AssertTimer assertTimer(max_time_to_wait, false);
    node_ptr task;
    while (true) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "sync_local failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        task          = defs->findAbsNode("/test_ECFLOW_1589/t1");
        BOOST_REQUIRE_MESSAGE(task, "Expected to find task\n");
        if (task->get_flag().is_set(ecf::Flag::ZOMBIE) && task->get_flag().is_set(ecf::Flag::JOBCMD_FAILED)) {
            break;
        }
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            break;
        }
        sleep(1);
    }
    BOOST_CHECK_MESSAGE(task->get_flag().is_set(ecf::Flag::ZOMBIE),
                        "expected zombie flag to be set after waiting "
                            << max_time_to_wait << "s\n"
                            << ecf::as_string(*TestFixture::client().defs(), PrintStyle::STATE));
    BOOST_CHECK_MESSAGE(task->get_flag().is_set(ecf::Flag::JOBCMD_FAILED),
                        "expected JOBCMD_FAILED flag to be set after waiting "
                            << max_time_to_wait << "s\n"
                            << ecf::as_string(*TestFixture::client().defs(), PrintStyle::STATE));

    std::cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount()
              << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

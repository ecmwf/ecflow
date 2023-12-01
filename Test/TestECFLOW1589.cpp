/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(TestSuite)

BOOST_AUTO_TEST_CASE(test_ECFLOW_1589) {
    // Test ECF_JOB_CMD where task *completes* but the ECF_JOB_CMD still fails. i.e ECFLOW-1589

    DurationTimer timer;
    cout << "Test:: ...test_ECFLOW_1589 " << flush;
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

    // Create a custom ecf file for test_task_abort_cmd/family0/abort to invoke the child abort command
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
    BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                          "sync_local failed should return 0\n"
                              << TestFixture::client().errorMsg());
    defs_ptr defs = TestFixture::client().defs();
    // cout << defs;
    node_ptr task = defs->findAbsNode("/test_ECFLOW_1589/t1");
    BOOST_REQUIRE_MESSAGE(task, "Expected to find task\n");
    BOOST_CHECK_MESSAGE(task->flag().is_set(ecf::Flag::ZOMBIE), "expected zombie flag to be set\n");
    BOOST_CHECK_MESSAGE(task->flag().is_set(ecf::Flag::JOBCMD_FAILED),
                        "expected JOBCMD_FAILED failed flag to be set\n");

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

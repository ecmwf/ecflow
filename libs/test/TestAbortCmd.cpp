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
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_AbortCmd)

// Test the abort command. This will test the abort command and the
// retry behaviour. i.e if a task is aborted, and the variable ECF_TRIES
// is defined. Then providing its value is less the the task's try number
// we should do an immediate job submission.
BOOST_AUTO_TEST_CASE(test_abort_cmd) {
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

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_task_abort_cmd
    //   edit ECF_TRIES '4'
    //   family family0
    //     task abort
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_abort_cmd");
        suite->addVariable(Variable("ECF_TRIES", "4"));
        family_ptr fam0 = suite->add_family("family0");
        task_ptr abort  = fam0->add_task("abort");
        abort->addVerify(VerifyAttr(NState::ABORTED, 3));  // task should abort 3 times & succeed on 4th attempt
        abort->addVerify(VerifyAttr(NState::COMPLETE, 1)); // task should complete 1 times
    }

    // Create a custom ecf file for test_task_abort_cmd/family0/abort to invoke the child abort command
    std::string templateEcfFile;
    templateEcfFile += "%include <head.h>\n";
    templateEcfFile += "\n";
    templateEcfFile += "echo do some work\n";
    templateEcfFile += "if [ %ECF_TRYNO% -le 3 ] ; then\n";
    templateEcfFile += "   %ECF_CLIENT_EXE_PATH% --abort=\"expected abort at task try no %ECF_TRYNO%\"\n";
    templateEcfFile += "   trap 0       # Remove all traps\n";
    templateEcfFile += "   exit 0       # End the shell before child command complete\n";
    templateEcfFile += "fi\n";
    templateEcfFile += "\n";
    templateEcfFile += "%include <tail.h>\n";

    // The test harness will create corresponding directory structure
    // Override the default ECF file, with our custom ECF_ file
    std::map<std::string, std::string> taskEcfFileMap;
    taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(theDefs, "abort"), templateEcfFile));

    // Avoid standard verification since we expect to abort many times
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_abort_cmd.def"), taskEcfFileMap);

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

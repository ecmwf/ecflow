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
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_AbortCmd)

// Test the abort command.
//
// This will test the abort command and the retry behavior.
//
// Aborting a task with an associated variable ECF_TRIES defined,
// when ECF_TRIES value is less than the task's try number,
// triggers an immediate job submission.
//
BOOST_AUTO_TEST_CASE(test_abort_cmd) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Create the defs file corresponding to the text below
    //
    // The following variables are automatically added by the test harness:
    //  * ECF_HOME
    //  * ECF_INCLUDE
    //  * SLEEPTIME
    //  * ECF_CLIENT_EXE_PATH
    //      n.b. This is substituted in .ecf includes and allows tests to run without requiring installation
    //

    // clang-format off
    std::string definition =
        "suite test_task_abort_cmd\n"
        "  edit ECF_TRIES '4'\n"
        "  family family0\n"
        "    task abort\n"
        "    verify aborted:3\n"
        "    verify complete:1\n"
        "  endfamily\n"
        "endsuite\n";
    // clang-format on

    Defs defs;
    defs.restore_from_string(definition);

    // Create a custom ecf file for test_task_abort_cmd/family0/abort to invoke the child abort command
    //

    std::string templateEcfFile = "%include <head.h>\n"
                                  "\n"
                                  "echo do some work\n"
                                  "if [ %ECF_TRYNO% -le 3 ] ; then\n"
                                  "   %ECF_CLIENT_EXE_PATH% --abort=\"expected abort at task try no %ECF_TRYNO%\"\n"
                                  "   trap 0       # Remove all traps\n"
                                  "   exit 0       # End the shell before child command complete\n"
                                  "fi\n"
                                  "\n"
                                  "%include <tail.h>\n";

    // The test harness will create the corresponding directory structure
    // Override the default ECF file, with our custom ECF_ file
    std::map<std::string, std::string> taskEcfFileMap;
    taskEcfFileMap.insert(std::make_pair(TestFixture::taskAbsNodePath(defs, "abort"), templateEcfFile));

    // Avoid standard verification since we expect to abort many times
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(defs, ServerTestHarness::testDataDefsLocation("test_abort_cmd.def"), taskEcfFileMap);

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

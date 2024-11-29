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

BOOST_AUTO_TEST_SUITE(T_EcfNoScriptCmd)

BOOST_AUTO_TEST_CASE(test_ecf_no_script) {
    ECF_NAME_THIS_TEST();

    // This test is used to case where we ONLY want to execute ECF_JOB_CMD WITHOUT processing .ecf script.
    // For this ECF_NO_SCRIPT must be set to any value.
    // The ECF_JOB_CMD must then encompass ecflow_client --init/complete and the environment settings
    // Additionally the user can see the output by directing the word done to ECF_JOBOUT
    // Although ServerTestHarness creates the .ecf file, they are ignored when ECF_NO_SCRIPT is specified.

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
    //  suite test_ecf_no_script_cmd
    //       edit ECF_NO_SCRIPT 1
    //       edit ECF_JOB_CMD "export ECF_PASS=%ECF_PASS%;...."
    //       task t1
    //       task t2
    //      endfamily
    //  endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_ecf_no_script");
        suite->add_variable("ECF_NO_SCRIPT", "1");
        std::string ecf_job_cmd = "export ECF_PASS=%ECF_PASS%;export ECF_PORT=%ECF_PORT%;export "
                                  "ECF_HOST=%ECF_HOST%;export ECF_NAME=%ECF_NAME%;export ECF_TRYNO=%ECF_TRYNO%;";
        ecf_job_cmd += "%ECF_CLIENT_EXE_PATH% --init=$$; echo 'test test_ecf_no_script' >> %ECF_JOBOUT%; "
                       "%ECF_CLIENT_EXE_PATH% --complete";
        suite->add_variable("ECF_JOB_CMD", ecf_job_cmd);
        suite->add_task("t1")->addVerify(VerifyAttr(NState::COMPLETE, 1));
        suite->add_task("t2")->addVerify(VerifyAttr(NState::COMPLETE, 1));
        // cout << theDefs;
    }

    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_ecf_no_script.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

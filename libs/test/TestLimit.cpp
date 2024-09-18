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
#include "TestNaming.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Limit)

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE(test_limit) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Create the defs file corresponding to the text below
    // ECF_HOME variable is automatically added by the test harness.
    // ECF_INCLUDE variable is automatically added by the test harness.
    // SLEEPTIME variable is automatically added by the test harness.
    // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
    //                     This is substituted in sms includes
    //                     Allows test to run without requiring installation

    // # Test the ecf file can be found via ECF_SCRIPT
    // # Note: we have to use relative paths, since these tests are relocatable
    // Create the defs file
    // suite test_limit
    //   limit disk 50
    //   limit fast 1
    //   edit ECF_HOME data/ECF_HOME    # added by test harness
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     inlimit /suite1:fast
    //     task t1
    //     task t2
    //     task t3
    //   endfamily
    //   family family2
    //     inlimit /suite1:disk 20
    //     task t1
    //     task t2
    //     task t3
    //   endfamily
    // endsuite

    Defs theDefs;
    {
        std::string suiteName   = "test_limit";
        std::string pathToLimit = "/" + suiteName;

        suite_ptr suite = theDefs.add_suite(suiteName);
        suite->addLimit(Limit("fast", 1));
        suite->addLimit(Limit("disk", 50));

        family_ptr fam = suite->add_family("family");
        fam->addInLimit(InLimit("fast", pathToLimit));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));
        int taskSize = 3;
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }

        family_ptr fam2 = suite->add_family("family2");
        fam2->addInLimit(InLimit("disk", pathToLimit, 20));
        fam2->addVerify(VerifyAttr(NState::COMPLETE, 1));
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam2->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }
    }

    // The test harness will create corresponding directory structure
    // and populate with standard ecf files.
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_limit.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

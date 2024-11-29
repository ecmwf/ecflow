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
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_EcfScriptCmd)

BOOST_AUTO_TEST_CASE(test_ECF_SCRIPT_CMD) {
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

    // # Note: we have to use relative paths, since these tests are relocatable
    //  suite test_ECF_SCRIPT_CMD
    //    family family
    //      task check
    //          edit ECF_SCRIPT_CMD "cat $ECF_HOME/test_ECF_SCRIPT_CMD/family/check.ecf"
    //      task t1
    //          trigger check == complete
    //          edit ECF_SCRIPT_CMD "cat $ECF_HOME/test_ECF_SCRIPT_CMD/family/t1.ecf"
    //    endfamily
    //  endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_ECF_SCRIPT_CMD");
        family_ptr fam  = suite->add_family("family");
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr task_check = fam->add_task("check");
        task_check->addVerify(VerifyAttr(NState::COMPLETE, 1));
        task_check->add_variable("ECF_SCRIPT_CMD",
                                 "cat " + TestFixture::smshome() + task_check->absNodePath() + File::ECF_EXTN());

        task_ptr task_t1 = fam->add_task("t1");
        task_t1->add_trigger("check == complete");
        task_t1->addVerify(VerifyAttr(NState::COMPLETE, 1));
        task_t1->add_variable("ECF_SCRIPT_CMD",
                              "cat " + TestFixture::smshome() + task_t1->absNodePath() + File::ECF_EXTN());
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_ECF_SCRIPT_CMD.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

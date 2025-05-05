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
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Trigger)

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.
BOOST_AUTO_TEST_CASE(test_triggers_and_meters) {
    ECF_NAME_THIS_TEST();

    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_triggers_and_meters
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     task model
    //       meter file 0 100 100
    //       task t0
    //         trigger model:file ge 10
    //       task t1
    //         trigger model:file ge 20
    //       task t2
    //         trigger model:file ge 30
    //       ....
    //   endfamily
    // endsuite
    std::string meterName = "file";
    std::string taskName  = "model";
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_triggers_and_meters");
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        family_ptr fam = suite->add_family("family");
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr taskModel = fam->add_task(taskName);
        taskModel->addMeter(Meter(meterName, 0, 100, 100)); // ServerTestHarness will add correct ecf

        int taskSize = 9; // on linux 1024 tasks take ~4 seconds for job submission
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i * 10 + 10));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
            task->add_trigger(taskName + Str::COLON() + meterName + " ge " + ecf::convert_to<std::string>(i * 10 + 10));
        }
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_triggers_and_meters.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE(test_triggers_with_limits) {
    ECF_NAME_THIS_TEST();

    // One family is in the limits, another is without. Bit of hack
    // But shows use of limits in triggers
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // # Note: we have to use relative paths, since these tests are relocatable
    // suite test_triggers_with_limits
    //   limit limit 10
    //   edit SLEEPTIME 1
    //   edit ECF_INCLUDE $ECF_HOME/includes
    //   family family
    //     inlimit /test_triggers_and_meters:limit 3
    //     task model
    //     task t0
    //     task t1
    //     task t2
    //   endfamily
    //   family other
    //     trigger /test_triggers_with_limits:limit >1
    //     task t1
    //     task t2
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_triggers_with_limits");
        suite->addLimit(Limit("limit", 10));
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));
        family_ptr fam = suite->add_family("family");
        fam->addInLimit(InLimit("limit", "/test_triggers_with_limits", 3));
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));
        int taskSize = 10;
        for (int i = 0; i < taskSize; i++) {
            task_ptr task = fam->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }

        family_ptr fam2 = suite->add_family("other");
        fam2->addVerify(VerifyAttr(NState::COMPLETE, 1));
        fam2->add_trigger("/test_triggers_with_limits:limit > 1");
        for (int i = 0; i < 3; i++) {
            task_ptr task = fam2->add_task("t" + ecf::convert_to<std::string>(i));
            task->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_triggers_and_meters.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

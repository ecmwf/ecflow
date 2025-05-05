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
#include "ecflow/attribute/QueueAttr.hpp"
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

BOOST_AUTO_TEST_SUITE(T_QueueCmd)

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.

BOOST_AUTO_TEST_CASE(test_queue) {
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
    // suite test_queue
    //   family f1
    //     queue q1 1 2 3
    //     task t
    //   endfamily
    //   family f2
    //     task a
    //       queue q2 1 2 3
    //     task b
    //       trigger /test_queue/f1:q1 > 1
    //     task c
    //       trigger /test_queue/f2/a:q2 > 1
    //   endfamily
    // endsuite
    Defs theDefs;
    {
        std::vector<std::string> queue_items;
        queue_items.emplace_back("1");
        queue_items.emplace_back("2");
        queue_items.emplace_back("3");
        suite_ptr suite = theDefs.add_suite("test_queue");
        {
            family_ptr f1 = suite->add_family("f1");
            QueueAttr q1("q1", queue_items);
            f1->add_queue(q1);
            f1->add_task("t")->addVerify(VerifyAttr(NState::COMPLETE, 1));

            family_ptr f2 = suite->add_family("f2");
            task_ptr a    = f2->add_task("a");
            QueueAttr q2("q2", queue_items);
            a->add_queue(q2);

            task_ptr b = f2->add_task("b");
            b->add_trigger("/test_queue/f1:q1 > 1");
            b->addVerify(VerifyAttr(NState::COMPLETE, 1));
            task_ptr c = f2->add_task("c");
            c->add_trigger("/test_queue/f2/a:q2 > 1");
            c->addVerify(VerifyAttr(NState::COMPLETE, 1));
        }
        // cout << theDefs;
    }

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs, ServerTestHarness::testDataDefsLocation("test_queue.def"));

    cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

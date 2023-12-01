/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(BaseTestSuite)

static defs_ptr create_defs() {
    // suite s1
    //   family f1
    //     trigger f2 == complete
    //     task t1
    //     task t2
    //   endfamily
    //   family f2
    //     task t1
    //     task t2
    //   endfamily
    // endsuite
    defs_ptr theDefs = Defs::create();
    suite_ptr suite  = theDefs->add_suite("s1");
    family_ptr f1    = suite->add_family("f1");
    family_ptr f2    = suite->add_family("f2");
    f1->add_trigger("f2 == complete");
    f1->add_task("t1");
    f1->add_task("t2");
    f2->add_task("t1");
    f2->add_task("t2");

    return theDefs;
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_189) {
    cout << "Base:: ...test_ECFLOW_189\n";
    TestLog test_log("test_ECFLOW_189.log"); // will create log file, and destroy log and remove file at end of scope

    defs_ptr the_defs = create_defs();
    the_defs->beginAll();
    node_ptr t1 = the_defs->findAbsNode("/s1/f1/t1");
    node_ptr t2 = the_defs->findAbsNode("/s1/f1/t2");

    TestHelper::invokeRequest(the_defs.get(), Cmd_ptr(new PathsCmd(PathsCmd::SUSPEND, t1->absNodePath())));
    TestHelper::invokeRequest(the_defs.get(), Cmd_ptr(new PathsCmd(PathsCmd::SUSPEND, t2->absNodePath())));

    TestHelper::test_state(t1, DState::SUSPENDED);
    TestHelper::test_state(t2, DState::SUSPENDED);

    // Now resume /s1/f1/t1 and /s1/f1/t2
    TestHelper::invokeRequest(the_defs.get(), Cmd_ptr(new PathsCmd(PathsCmd::RESUME, t1->absNodePath())));

    // We expect state to be queued since the trigger on /s1/f1 should prevent jobs from running
    // If we find submitted or aborted(i.e it was free to run, but could not generate the jobs), then its an error
    TestHelper::test_state(t1, NState::QUEUED);
    TestHelper::test_state(t2, NState::QUEUED);

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

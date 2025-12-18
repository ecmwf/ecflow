/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "TestUtil.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/simulator/Simulator.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Simulator)

BOOST_AUTO_TEST_SUITE(T_Analysis)

/// Use this class to test single simulation of definition file that we want to add
/// to Test Simulator. This is a separate exe

BOOST_AUTO_TEST_CASE(test_analysys) {
    cout << "Simulator:: ...test_analysys\n";
    // suite suite
    //   family family
    //     task t1
    //       trigger t2 == complete
    //     task t2
    //       trigger t1 == complete
    //   endfamily
    // endsuite

    // This simulation is expected to fail, since we have a deadlock/ race condition
    // It will prodice a defs.depth and defs.flat files. Make sure to remove them
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_analysys");
        family_ptr fam  = suite->add_family("family");

        task_ptr task1 = fam->add_task("t1");
        task1->add_trigger("t2 == complete");

        task_ptr task2 = fam->add_task("t2");
        task2->add_trigger("t1 == complete");
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(!simulator.run(theDefs, findTestDataLocation("test_analysys.def"), errorMsg), errorMsg);

    fs::remove("defs.depth");
    fs::remove("defs.flat");

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_analysys.def") + ".log";
    fs::remove(logFileName);

    /// Destroy singleton's to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

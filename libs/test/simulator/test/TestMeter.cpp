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
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/simulator/Simulator.hpp"

using namespace ecf;

/// Simulate definition files that are created on then fly. This us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE(SimulatorTestSuite)

BOOST_AUTO_TEST_CASE(test_meter) {
    std::cout << "Simulator:: ...test_meter\n";

    // suite suite
    //   clock real <todays date>
    //   family family
    //     task fc
    //       meter hour 0 240
    //       task half
    //         trigger fc:hour >= 120
    //   endfamily
    // endsuite

    // Initialise clock with todays date  then create a time attribute + minutes
    // such that the task should only run once, in the next minute
    auto theLocalTime = Calendar::second_clock_time();

    Defs theDefs;
    {
        ClockAttr clockAttr(theLocalTime);
        suite_ptr suite = theDefs.add_suite("test_meter");
        suite->addClock(clockAttr);
        suite->addVerify(VerifyAttr(NState::COMPLETE, 1));

        family_ptr fam = suite->add_family("family");
        fam->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr fc = fam->add_task("fc");
        fc->addMeter(Meter("hour", 0, 240, 240));
        fc->addVerify(VerifyAttr(NState::COMPLETE, 1));

        task_ptr half = fam->add_task("half");
        half->add_trigger("fc:hour >= 120");
        half->addVerify(VerifyAttr(NState::COMPLETE, 1));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_meter.def"), errorMsg), errorMsg);

    // The simulator will set all the meter values, so final value must be the max value.
    bool found_task = false;
    auto tasks      = ecf::get_all_tasks(theDefs);
    for (auto task : tasks) {
        if (task->name() == "fc") {
            found_task                       = true;
            const std::vector<Meter>& meters = task->meters();
            BOOST_REQUIRE_MESSAGE(meters.size() == 1, "Expected one meter but found " << meters.size());
            BOOST_CHECK_MESSAGE(meters[0].value() == meters[0].max(),
                                "Expected meter to have value of " << meters[0].max() << " but found "
                                                                   << meters[0].value());
        }
    }
    BOOST_REQUIRE_MESSAGE(found_task, "Failed to find task fc ");

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_meter.def") + ".log";
    fs::remove(logFileName);

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

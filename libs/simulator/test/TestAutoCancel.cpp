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
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/simulator/Simulator.hpp"

using namespace ecf;

/// Simulate definition files that are created on then fly. This us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE(S_Simulator)

BOOST_AUTO_TEST_SUITE(T_AutoCancel)

BOOST_AUTO_TEST_CASE(test_autocancel_ast_node_reset) {
    std::cout << "Simulator:: ...test_autocancel_ast_node_reset\n";

    // ****: Since we have no time dependencies the simulator calendar increment
    // ****: is in hours. Hence autocancel at hour resolution
    Defs theDefs;
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("s1");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->add_trigger("/s2/family/t == complete");

        task_ptr task2 = fam->add_task("t2");
        task2->add_trigger("/s3/family/t == complete");
        task2->add_complete("/s2/family == complete");

        task_ptr task3 = fam->add_task("t3");
        task3->add_trigger("/s3 == complete");
        task3->add_complete("/s2 == complete");
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday

        suite_ptr suite = theDefs.add_suite("s2");
        suite->addClock(clockAttr);
        suite->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), false));

        family_ptr fam = suite->add_family("family");
        fam->add_task("t");
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("s3");
        suite->addClock(clockAttr);
        suite->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), false));

        family_ptr fam = suite->add_family("family");
        fam->add_task("t");
    }

    // Check number of AST nodes. The AST should be created on the fly
    std::set<Node*> theSet;
    theDefs.getAllAstNodes(theSet);
    BOOST_CHECK_MESSAGE(theSet.size() == 5,
                        "Expected to have 5 AST nodes in trigger/complete expressions but found " << theSet.size());

    // Run the simulator
    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_autocancel_ast_node_reset.def"), errorMsg),
                        errorMsg);

    // Auto cancel should delete suite s2 and s3, leaving one suite i.e s1
    BOOST_CHECK_MESSAGE(theDefs.suiteVec().size() == 1,
                        "Expected to have 1 suites but found " << theDefs.suiteVec().size());

    // The references to nodes in suites s2, s3 should have been cleared in suite s1
    {
        std::set<Node*> theSet;
        theDefs.getAllAstNodes(theSet);
        BOOST_CHECK_MESSAGE(theSet.empty(),
                            "Expected to have 0 AST nodes in trigger/complete expressions but found " << theSet.size());
    }

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_autocancel_ast_node_reset.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_autocancel_suite) {
    std::cout << "Simulator:: ...test_autocancel_suite\n";

    // ****: Since we have no time dependencies the simulator calendar increment
    // ****: is in hours. Hence autocancel at hour resolution
    Defs theDefs;
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_10_hours_relative");
        suite->addClock(clockAttr);
        suite->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(10, 0), true));
        family_ptr fam = suite->add_family("family");
        fam->add_task("t");
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_1_hours_real");
        suite->addClock(clockAttr);
        suite->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), false));
        family_ptr fam = suite->add_family("family");
        fam->add_task("t");
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_1_day_relative");
        suite->addClock(clockAttr);
        suite->addAutoCancel(ecf::AutoCancelAttr(1));
        family_ptr fam = suite->add_family("family");
        fam->add_task("t");
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_autocancel_suite.def"), errorMsg), errorMsg);

    // make sure autocancel deletes the suite.
    BOOST_CHECK_MESSAGE(theDefs.suiteVec().size() == 0,
                        "Expected to have 0 suites but found " << theDefs.suiteVec().size());

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_autocancel_suite.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_autocancel_family_and_task) {
    std::cout << "Simulator:: ...test_autocancel_family_and_task\n";

    // ****: Since we have no time dependencies the simulator calendar increment
    // ****: is in hours. Hence autocancel at hour resolution
    Defs theDefs;
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_9_10_hours_relative");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(10, 0), true));

        task_ptr task = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(9, 0), true));
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_1_2_hours_real");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(2, 0), false));

        task_ptr task = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), false));
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        ClockAttr end_clock(true);
        end_clock.date(14, 10, 2009); // 14 October 2009 was a Wednesday

        suite_ptr suite = theDefs.add_suite("test_autocancel_1_2_day_relative");
        suite->addClock(clockAttr);
        suite->add_end_clock(end_clock);

        family_ptr fam = suite->add_family("family");
        fam->addAutoCancel(ecf::AutoCancelAttr(2)); // 2 days

        task_ptr task = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(1)); // 1 days
    }
    // cout << theDefs << "\n";

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_autocancel_family_and_task.def"), errorMsg),
                        errorMsg);

    // make sure autocancel deletes the families.
    std::vector<Family*> famVec;
    theDefs.getAllFamilies(famVec);
    BOOST_CHECK_MESSAGE(famVec.size() == 0,
                        "Expected to have 0 families but found " << famVec.size() << "\n"
                                                                 << ecf::as_string(theDefs, PrintStyle::DEFS));

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_autocancel_family_and_task.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_autocancel_task) {
    std::cout << "Simulator:: ...test_autocancel_task\n";

    // ****: Since we have no time dependencies the simulator calendar increment
    // ****: is in hours. Hence autocancel at hour resolution
    Defs theDefs;
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_10_hours_relative");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(10, 0), true));
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_1_hours_real");
        suite->addClock(clockAttr);
        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), false));
    }
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_autocancel_1_day_relative");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        task_ptr task  = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(1));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_autocancel_task.def"), errorMsg), errorMsg);

    // make sure autocancel deletes the tasks and leaves families intact.
    std::vector<task_ptr> task_vec;
    theDefs.get_all_tasks(task_vec);

    std::vector<Family*> famVec;
    theDefs.getAllFamilies(famVec);

    BOOST_CHECK_MESSAGE(famVec.size() == 3, "Expected to have 3 families but found " << famVec.size());
    BOOST_CHECK_MESSAGE(task_vec.size() == 0, "Expected to have 0 tasks but found " << task_vec.size());

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_autocancel_task.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE(test_two_autocancel_in_hierarchy) {
    std::cout << "Simulator:: ...test_two_autocancel_in_hierarchy\n"; // ECFLOW-556

    // ****: Since we have no time dependencies the simulator calendar increment
    // ****: is in hours. Hence autocancel at hour resolution
    Defs theDefs;
    {
        ClockAttr clockAttr(true);
        clockAttr.date(12, 10, 2009); // 12 October 2009 was a Monday
        suite_ptr suite = theDefs.add_suite("test_two_autocancel_in_hierarchy");
        suite->addClock(clockAttr);

        family_ptr fam = suite->add_family("family");
        fam->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), true));
        task_ptr task = fam->add_task("t");
        task->addAutoCancel(ecf::AutoCancelAttr(ecf::TimeSlot(1, 0), true));
    }

    Simulator simulator;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(simulator.run(theDefs, findTestDataLocation("test_two_autocancel_in_hierarchy.def"), errorMsg),
                        errorMsg);

    std::vector<task_ptr> task_vec;
    theDefs.get_all_tasks(task_vec);

    std::vector<Family*> famVec;
    theDefs.getAllFamilies(famVec);

    BOOST_CHECK_MESSAGE(famVec.size() == 0, "Expected to have 0 families but found " << famVec.size());
    BOOST_CHECK_MESSAGE(task_vec.size() == 0, "Expected to have 0 tasks but found " << task_vec.size());

    // remove generated log file. Comment out to debug
    std::string logFileName = findTestDataLocation("test_two_autocancel_in_hierarchy.def") + ".log";
    fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

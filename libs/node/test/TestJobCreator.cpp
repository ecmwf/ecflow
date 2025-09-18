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

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_JobCreator)

BOOST_AUTO_TEST_CASE(test_job_creator) {
    ECF_NAME_THIS_TEST();

    // SET SMSHOME
    std::string ecf_home = File::test_data("libs/node/test/data/SMSHOME", "libs/node");

    // Create the defs file corresponding to the text below
    // # Test the sms file can be found via ECF_SCRIPT
    // # Note: we have to use relative paths, since these tests are relocatable
    // #
    // suite suite
    //	edit SLEEPTIME 10
    //	edit ECF_INCLUDE $ECF_HOME/includes
    //	family family
    //   	task t1
    //   	task t2
    //    	task t3
    //  	endfamily
    // endsuite
    // #
    // # This test suite should force a backwards search since the sms files
    // # are located in SMSHOME
    // suite suite1
    //    family family
    //   		task suite1_task1
    //   		task suite1_task2
    //    	    task suite1_task3
    //    endfamily
    // endsuite
    // Create a defs file, where the task name mirrors the sms files in the given directory
    Defs theDefs;
    {
        suite_ptr suite = Suite::create("suite");
        family_ptr fam  = Family::create("family");
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/../includes"));
        suite->addVariable(Variable("SLEEPTIME", "1"));
        suite->addVariable(Variable("ECF_CLIENT_EXE_PATH", "a/made/up/path"));
        fam->addTask(Task::create("t1"));
        fam->addTask(Task::create("t2"));
        fam->addTask(Task::create("t3"));
        suite->addFamily(fam);
        theDefs.addSuite(suite);
    }
    {
        suite_ptr suite1(new Suite("suite1"));
        family_ptr fam(new Family("family"));
        fam->addTask(Task::create("suite1_task1"));
        fam->addTask(Task::create("suite1_task2"));
        fam->addTask(Task::create("suite1_task3"));
        suite1->addFamily(fam);
        theDefs.addSuite(suite1);
    }
    // 	cerr << theDefs << "\n";

    // get all the task, assume non hierarchical families
    auto tasks = ecf::get_all_tasks(theDefs);
    BOOST_REQUIRE_MESSAGE(tasks.size() == 6, "Expected 6 tasks but found, " << tasks.size());

    // Override ECF_HOME.   ECF_HOME is need to locate to the .ecf files
    theDefs.server_state().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files
    theDefs.beginAll();

    // Test Job creator. The job creation should succeed 3 times only, since
    // the sms file suite1_task1, suite1_task2,suite1_task3 are empty.
    JobsParam jobsParam(true /*create jobs*/); // spawn_jobs = false
    Jobs jobs(&theDefs);
    jobs.generate(jobsParam);
    BOOST_REQUIRE_MESSAGE(jobsParam.submitted().size() == 3,
                          "expected 3 jobs but found " << jobsParam.submitted().size() << "\n"
                                                       << jobsParam.errorMsg());

    // Expect error message complaining about sms file suite1_task1, suite1_task2,suite1_task3 being empty
    BOOST_REQUIRE_MESSAGE(!jobsParam.errorMsg().empty(), "expected error message about empty ecf files");

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

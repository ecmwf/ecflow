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
#include <string>

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/base/cts/task/AbortCmd.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_Cmd)

BOOST_AUTO_TEST_CASE(test_simple_cmd) {
    ECF_NAME_THIS_TEST();

    TestLog test_log("test_simple_cmd.log"); // will create log file, and destroy log and remove file at end of scope

    // Create the defs file. Note that the default ECF_TRIES = 3
    //   suite suite
    //     family f
    //       task t1
    //       task t2
    //     endfamily
    //   endsuite
    Defs defs;
    string suite_f_t1     = "suite/f/t1";
    std::string suitename = "suite";
    family_ptr f          = Family::create("f");
    task_ptr t1           = Task::create("t1");
    task_ptr t2           = Task::create("t2");
    suite_ptr s           = Suite::create(suitename);
    {
        f->addTask(t1);
        f->addTask(t2);
        s->addFamily(f);
        defs.addSuite(s);
    }

    // ***********************************************************************
    // Create a request to begin suite
    // make sure chosen suite can begin to resolve dependencies.
    // beginning the suite will:
    //     1/ set all children to the QUEUED state
    //     2/ Begin job submission, and hence changes state to ACTIVE for submitted jobs
    {
        TestHelper::invokeRequest(&defs, Cmd_ptr(new BeginCmd(suitename, false)));
        BOOST_CHECK_MESSAGE(s->state() == NState::ACTIVE,
                            "expected state NState::ACTIVE, but found to be " << NState::toString(s->state()));
        BOOST_CHECK_MESSAGE(f->state() == NState::ACTIVE,
                            "expected state NState::ACTIVE, but found to be " << NState::toString(f->state()));
        BOOST_CHECK_MESSAGE(t1->state() == NState::ACTIVE,
                            "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));
        BOOST_CHECK_MESSAGE(t2->state() == NState::ACTIVE,
                            "expected state NState::ACTIVE, but found to be " << NState::toString(t2->state()));
    }

    // ***********************************************************************
    // Create a request to abort  Node: suite1/f/t1, Since the default ECF_TRIES is > 0, the aborted tasks
    // should be re-submitted, until the task try number > ECF_TRIES
    {
        std::string varValue;
        if (t1->findParentUserVariableValue(ecf::environment::ECF_TRIES, varValue)) {
            auto ecf_tries = ecf::convert_to<int>(varValue);
            while (true) {
                TestHelper::invokeRequest(
                    &defs,
                    Cmd_ptr(new AbortCmd(
                        suite_f_t1, Submittable::DUMMY_JOBS_PASSWORD(), Submittable::DUMMY_PROCESS_OR_REMOTE_ID(), 1)));
                TestHelper::invokeRequest(&defs, Cmd_ptr(new CtsCmd(CtsCmd::FORCE_DEP_EVAL)));
                BOOST_CHECK_MESSAGE(t1->state() == NState::ACTIVE,
                                    "expected state NState::ACTIVE, but found to be " << NState::toString(t1->state()));
                BOOST_CHECK_MESSAGE(f->state() == NState::ACTIVE,
                                    "expected state NState::ACTIVE, but found to be " << NState::toString(f->state()));
                BOOST_CHECK_MESSAGE(s->state() == NState::ACTIVE,
                                    "expected state NState::ACTIVE, but found to be " << NState::toString(s->state()));
                // std::cout << "tryNo = " << t1->try_no() << " ECF_TRIES = " <<  ecf_tries << "\n";
                if (t1->try_no() == ecf_tries)
                    break;
            }

            /// Since we have exceeded the try number, abort should mean abort
            TestHelper::invokeRequest(
                &defs,
                Cmd_ptr(new AbortCmd(
                    suite_f_t1, Submittable::DUMMY_JOBS_PASSWORD(), Submittable::DUMMY_PROCESS_OR_REMOTE_ID(), 1)));
            TestHelper::invokeRequest(&defs, Cmd_ptr(new CtsCmd(CtsCmd::FORCE_DEP_EVAL)), false);
            BOOST_CHECK_MESSAGE(t1->state() == NState::ABORTED,
                                "expected state NState::ABORTED, but found to be " << NState::toString(t1->state()));
            BOOST_CHECK_MESSAGE(f->state() == NState::ABORTED,
                                "expected state NState::ABORTED, but found to be " << NState::toString(f->state()));
            BOOST_CHECK_MESSAGE(s->state() == NState::ABORTED,
                                "expected state NState::ABORTED, but found to be " << NState::toString(s->state()));
        }
    }

    {
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(defs.checkInvariants(errorMsg), errorMsg);
    }

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

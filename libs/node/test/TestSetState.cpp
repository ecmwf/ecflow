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

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SetState)

static void set_state(node_ptr n, NState::State set, NState::State expected) {
    n->set_state(set);
    BOOST_REQUIRE_MESSAGE(n->state() == expected,
                          "Expected state " << NState::toString(expected) << " but found "
                                            << NState::toString(n->state()) << " for " << n->debugNodePath());
}

static void test_state(node_ptr n, NState::State expected) {
    BOOST_REQUIRE_MESSAGE(n->state() == expected,
                          "Expected state " << NState::toString(expected) << " but found "
                                            << NState::toString(n->state()) << " for " << n->debugNodePath());
}

BOOST_AUTO_TEST_CASE(test_set_state) {
    ECF_NAME_THIS_TEST();

    std::vector<NState::State> stateVec = NState::states();

    // 	cout << "Defs setState\n";
    Defs theDefs;
    for (NState::State state : stateVec) {
        theDefs.set_state(state);
        BOOST_REQUIRE_MESSAGE(theDefs.state() == state,
                              "Expected defs state " << NState::toString(state) << " but found "
                                                     << NState::toString(theDefs.state()));
    }
    //  	theDefs.resume();                   // unset the suspended state. Start with default, for next set of
    //  test
    theDefs.set_state(NState::UNKNOWN); // Start with default, for next set of test

    // 	cout << "Suite setState\n";
    suite_ptr s = Suite::create("s");
    theDefs.addSuite(s);
    for (NState::State state : stateVec) {
        set_state(s, state, state); // suite with no children, state should be what was set
    }
    s->resume();                   // unset the suspended state. Start with default, for next set of test
    s->set_state(NState::UNKNOWN); // Start with default, for next set of test

    // 	cout << "family setState\n";
    family_ptr f = Family::create("f");
    s->addFamily(f);
    for (NState::State state : stateVec) {
        set_state(f, state, state); // family with no children, state should be what was set
    }
    f->resume();                   // unset the suspended state. Start with default, for next set of test
    f->set_state(NState::UNKNOWN); // Start with default, for next set of test

    // 	cout << "task setState\n";
    task_ptr t = Task::create("t");
    f->addTask(t);

    for (NState::State state : stateVec) {
        f->setStateOnly(NState::UNKNOWN); // reset family state
        s->setStateOnly(NState::UNKNOWN); // reset suite state
        set_state(t, state, state);       // task state should be what was set

        test_state(f, state); // family state should be computed state
        test_state(s, state); // suite state should be computed state
    }

    /// Everytime we set the state on a nodecontainer, we call handleStateChange
    /// This by default works out the most significant state of the children
    /// ie. the computed state. Hence setting the state on Suite/Family is really
    /// meaningless, since it will always be the computed state.
}

BOOST_AUTO_TEST_CASE(test_set_aborted) {
    // see ECFLOW-344
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr suite = defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addDefStatus(DState::COMPLETE);
    defs.beginAll();

    {
        Jobs jobs(&defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);
        BOOST_CHECK_MESSAGE(jobsParam.submitted().size() == 0,
                            "No jobs should be submitted when task is complete but found "
                                << jobsParam.submitted().size() << " submitted");
    }

    t1->setStateOnly(NState::ABORTED, true /*force*/);
    {
        Jobs jobs(&defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);
        BOOST_CHECK_MESSAGE(jobsParam.submitted().size() == 0,
                            "No jobs should be submitted when task is forcibly aborted but found "
                                << jobsParam.submitted().size() << " submitted");
    }

    Node::Requeue_args args;
    t1->requeue(args);

    t1->set_state(NState::ABORTED); // mimic non forced, i.e like job aborted
    {
        Jobs jobs(&defs);
        JobsParam jobsParam;
        jobs.generate(jobsParam);
        BOOST_CHECK_MESSAGE(jobsParam.submitted().size() == 1,
                            "Expected 1 job, when job aborts and ECF_TRIES > 1 but found "
                                << jobsParam.submitted().size() << " submitted");
        BOOST_CHECK_MESSAGE(t1->try_no() == 1, " expected try_no to be 1 but found " << t1->try_no());
    }

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

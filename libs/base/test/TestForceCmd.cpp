/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include <boost/test/unit_test.hpp> // IWYU pragma: keep

#include "MockServer.hpp"
#include "MyDefsFixture.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/PrintStyle.hpp" // IWYU pragma: keep
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ForceCmd)

static defs_ptr create_defs() {
    defs_ptr theDefs = Defs::create();
    suite_ptr suite  = theDefs->add_suite("s1");
    family_ptr f1    = suite->add_family("f1");
    task_ptr t1      = f1->add_task("t1");
    t1->addTime(TimeAttr(10, 30));
    t1->add_alias_only();
    task_ptr t2 = f1->add_task("t2");
    t2->add_alias_only();
    return theDefs;
}

BOOST_AUTO_TEST_CASE(test_add_log2) {
    // create once for all test below, then remove at the end
    Log::create("test_add_log2.log");
    BOOST_CHECK_MESSAGE(true, "stop boost test form complaining");
}

BOOST_AUTO_TEST_CASE(test_force_cmd) {
    ECF_NAME_THIS_TEST();

    defs_ptr the_defs = create_defs();
    the_defs->beginAll();
    node_ptr s1 = the_defs->findAbsNode("/s1");
    node_ptr f1 = the_defs->findAbsNode("/s1/f1");
    node_ptr t1 = the_defs->findAbsNode("/s1/f1/t1");
    node_ptr t2 = the_defs->findAbsNode("/s1/f1/t2");

    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", true /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");
    BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");

    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(t2->absNodePath(), "complete", true /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t2, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(
        !t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set since there are NO time depedencies");
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");
    BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");

    the_defs->requeue();
    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(s1->absNodePath(), "complete", true /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(s1, NState::COMPLETE);
    TestHelper::test_state(f1, NState::COMPLETE);
    TestHelper::test_state(t1, NState::COMPLETE);

    Node::Requeue_args args(Node::Requeue_args::FULL,
                            true /* reset repeats*/,
                            0 /* clear_suspended_in_child_nodes */,
                            false /* reset_next_time_slot_ */,
                            true /* reset relative duration */);
    s1->requeue(args);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
    BOOST_CHECK_MESSAGE(!t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
    BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");

    node_ptr alias = the_defs->findAbsNode("/s1/f1/t1/alias0");
    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(alias->absNodePath(), "complete", true /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(alias, NState::COMPLETE);
}

static void
doForce(MockServer& mockServer, Node* fnode, const std::string& stateOrEvent, const std::vector<Node*>& nodes) {
    ForceCmd cmd(fnode->absNodePath(), stateOrEvent, true /*recursive */, true /* set Repeat to last value */);
    cmd.setup_user_authentification();
    STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
    BOOST_REQUIRE_MESSAGE(returnCmd->ok(), "Failed to force for node " << fnode->debugNodePath());

    for (auto node : nodes) {
        if (NState::isValid(stateOrEvent)) {
            NState::State state = NState::toState(stateOrEvent);

            // Force Cmd recursive does **NOT** apply to aliases.
            if (node->isAlias()) {
                // The alias should still be in default QUEUED state
                BOOST_CHECK_MESSAGE(node->state() == NState::QUEUED,
                                    "Expected state NState::QUEUED for alias but found "
                                        << NState::toString(node->state()) << " for alias " << node->debugNodePath());
            }
            else {
                BOOST_CHECK_MESSAGE(node->state() == state,
                                    "Expected state " << NState::toString(state) << " but found "
                                                      << NState::toString(node->state()) << " for node "
                                                      << node->debugNodePath());
            }
        }
        else
            BOOST_CHECK_MESSAGE(false, "oops");

        if (!(node->repeat().empty())) {
            BOOST_CHECK_MESSAGE(!node->repeat().valid(), "Expected repeat to be set to last value. ie in valid");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_force_cmd_recursive) {
    ECF_NAME_THIS_TEST();

    defs_ptr the_defs = create_defs();
    node_ptr suite    = the_defs->findAbsNode("/s1");
    std::vector<Node*> nodes;
    suite->getAllNodes(nodes);

    MockServer mockServer(the_defs);
    std::vector<std::string> all_states = NState::allStates();
    for (const std::string& state : all_states) {
        doForce(mockServer, suite.get(), state, nodes);
    }
}

BOOST_AUTO_TEST_CASE(test_force_cmd_bubbles_up_state_changes) {
    ECF_NAME_THIS_TEST();

    defs_ptr the_defs = create_defs();
    std::vector<Node*> nodes;
    std::vector<Task*> tasks;
    the_defs->getAllNodes(nodes);
    the_defs->getAllTasks(tasks);
    node_ptr suite = the_defs->findAbsNode("/s1");

    MockServer mockServer(the_defs);

    std::vector<std::string> all_states = NState::allStates();
    for (const std::string& state : all_states) {

        // 		cout << "Setting all tasks to state " << state << "\n";
        for (Task* task : tasks) {
            ForceCmd cmd(task->absNodePath(), state, false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_REQUIRE_MESSAGE(returnCmd->ok(), "Failed to force for node " << task->debugNodePath());
        }

        // Check that state change set on task has bubbled up to the suite.
        // Since the state has been set on *all* tasks
        // 		cout << "Suite state = " << NState::toString(suite->state()) << "\n";
        NState::State expected_state = NState::toState(state);
        BOOST_CHECK_MESSAGE(suite->state() == expected_state,
                            "Expected state " << NState::toString(expected_state) << " but found "
                                              << NState::toString(suite->state()) << " for suite "
                                              << suite->debugNodePath());
    }
}

BOOST_AUTO_TEST_CASE(test_force_cmd_alias_does_not_bubble_up_state_changes) {
    ECF_NAME_THIS_TEST();

    defs_ptr the_defs = create_defs();
    std::vector<Node*> nodes;
    std::vector<alias_ptr> aliases;
    the_defs->getAllNodes(nodes);
    the_defs->get_all_aliases(aliases);
    node_ptr suite = the_defs->findAbsNode("/s1");

    // initialize by setting all nodes to state QUEUED
    for (Node* n : nodes) {
        n->set_state(NState::QUEUED);
    }

    MockServer mockServer(the_defs);
    std::vector<std::string> all_states = NState::allStates();
    for (const std::string& state : all_states) {

        for (alias_ptr alias : aliases) {
            ForceCmd cmd(alias->absNodePath(), state, false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_REQUIRE_MESSAGE(returnCmd->ok(), "Failed to force for alias " << alias->debugNodePath());
        }

        // Check that state change set on alias has *NOT* bubbled up to the suite.
        BOOST_CHECK_MESSAGE(suite->state() == NState::QUEUED,
                            "Expected suite to have state QUEUED but found " << NState::toString(suite->state()));
    }
}

BOOST_AUTO_TEST_CASE(test_force_events) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture fixtureDef;
    MockServer mockServer(&fixtureDef.defsfile_);

    node_ptr suite = fixtureDef.fixtureDefsFile().findAbsNode("/suiteName");
    BOOST_REQUIRE_MESSAGE(suite.get(), "Could not find suite");
    std::vector<Node*> nodes;
    suite->getAllNodes(nodes);

    /// Set and clear events
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + ":" + e.name_or_number();
            ForceCmd cmd(path, Event::SET(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_REQUIRE_MESSAGE(returnCmd->ok(), "Failed to force event for node " << node->debugNodePath());
        }
        for (const Event& e : node->events()) {
            BOOST_CHECK_MESSAGE(e.value(), "Event not set as expected for node " << node->debugNodePath());
        }
    }
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + ":" + e.name_or_number();
            ForceCmd cmd(path, Event::CLEAR(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            STC_Cmd_ptr returnCmd = cmd.handleRequest(&mockServer);
            BOOST_REQUIRE_MESSAGE(returnCmd->ok(), "Failed to force event for node " << node->debugNodePath());
        }
        for (const Event& e : node->events()) {
            BOOST_CHECK_MESSAGE(!e.value(), "Event not cleared as expected for node " << node->debugNodePath());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_force_events_errors) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture fixtureDef;
    MockServer mockServer(&fixtureDef.defsfile_);

    node_ptr suite = fixtureDef.fixtureDefsFile().findAbsNode("/suiteName");
    BOOST_REQUIRE_MESSAGE(suite.get(), "Could not find suite");
    std::vector<Node*> nodes;
    suite->getAllNodes(nodes);

    /// Make a path that does not exist
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + "/path/doesnot/exist" + ":" + e.name_or_number();
            ForceCmd cmd(path, Event::SET(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + "/path/doesnot/exist" + ":" + e.name_or_number();
            ForceCmd cmd(path, Event::CLEAR(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }

    /// Make path that does not contain a event
    for (Node* node : nodes) {
        if (node->events().empty()) {
            std::string path = node->absNodePath();
            ForceCmd cmd(path, Event::SET(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }
    for (Node* node : nodes) {
        if (node->events().empty()) {
            std::string path = node->absNodePath();
            ForceCmd cmd(path, Event::CLEAR(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }

    /// Make a event that does not exist
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + ":" + e.name_or_number() + "made_up";
            ForceCmd cmd(path, Event::SET(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }
    for (Node* node : nodes) {
        for (const Event& e : node->events()) {
            std::string path = node->absNodePath() + ":" + e.name_or_number() + "made_up";
            ForceCmd cmd(path, Event::CLEAR(), false /*recursive */, false /* set Repeat to last value */);
            cmd.setup_user_authentification();
            BOOST_REQUIRE_THROW(cmd.handleRequest(&mockServer), std::runtime_error);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_force_interactive) {
    // This test is custom. When the user interactively forces a node to the
    // complete state, and that node has pending time activities. The default
    // behaviour, is the node is re-queued again, and hence the propagation up the
    // node tree does not happen. This is test checks that the node does *not* exhibit
    // this functionality. What we want is that task is set to complete, without
    // forcing a re-queue, this is then propagated up the node tree. Which forces the
    // family to complete, and hence update the repeat variable.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //     family daily
    //       repeat date YMD 20101215 20101217 1
    //       task t1
    //         time 11:30
    //     endfamily
    //   endsuite
    // make sure time is set *before* 11:30, so that time dependency holds the task
    defs_ptr the_defs = Defs::create();
    suite_ptr suite   = Suite::create("s1");
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(1 /*hour*/, 0 /*minutes*/);
    suite->addClock(clockAttr);
    family_ptr f1 = Family::create("daily");
    f1->addRepeat(RepeatDate("date", 20101215, 20101217));
    task_ptr t1 = Task::create("t1");
    t1->addTime(ecf::TimeAttr(ecf::TimeSlot(11, 30)));
    f1->addTask(t1);
    suite->addFamily(f1);
    the_defs->addSuite(suite);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, before test");
    BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, before test");

    /// begin the suite
    TestHelper::invokeRequest(the_defs.get(), Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101215,
                          "Repeat value expected is 20101215 but found " << f1->repeat().value());

    // Force the task t1 to complete state.
    // Since task t1 is complete, the family 'daily' should complete.
    // This will cause the repeat to take the next value anf forcing a requeue
    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101216,
                          "Repeat value expected is 20101216 but found " << f1->repeat().value());

    // Reque should mean flag was cleared + on suite should never get set, since flag is stopped at repeat
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since task was REQUED");
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since family was REQUED");
    BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since repeat should stop flag "
                        "propagation up node tree");

    // Force the task t1 to complete again
    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101217,
                          "Repeat value expected is 20101217 but found " << f1->repeat().value());

    // Do for the last time,
    TestHelper::invokeRequest(
        the_defs.get(),
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    TestHelper::test_state(f1, NState::COMPLETE);
    TestHelper::test_state(suite, NState::COMPLETE);

    // Since we completed, without a requeue, we should expect flag to stay set.
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set, since there was no reque");

    // Flag propagation should stop at the repeat
    BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP *NOT* to be set");
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot) {
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user has a single time slot. We should stay complete and NOT requee
    //
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          time 10:00
    //   endsuite
    // make sure time is set *before* 10:00, so that time dependency holds the task
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addTime(TimeAttr(10, 0));
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(9 /*hour*/, 30 /*minutes*/); // start at 09:30
    suite->addClock(clockAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // since we started at 09:30 the next time slot should be 10:00
    const TimeSlot& next_time_slot = t1->timeVec().back().time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "Expected next time slot of 10:00 but found " << next_time_slot.toString());

    // Force the task t1 to complete state. Since we had ONLY a SINGLE time dependency we should stay complete
    // It should also advance the next time slot
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(!t1->timeVec().back().time_series().is_valid(), "Expected 10:00 time slot to have expired");
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");

    // call again should, should be do difference, EXPECT no change, if its complete, seting complete, avoids another
    // state change
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)),
        false /*dont Check change numbers*/);
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(!t1->timeVec().back().time_series().is_valid(), "Expected 10:00 time slot to have expired");
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");

    /// we will now Re-queue, Since the time is still 09:30, we expect next_time slot to be 10:00
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "After Re-queue, Expected next time slot of 10:00 but found " << next_time_slot.toString());
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_1) {
    // Start TIME at 9:30
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user set of time slots. In this case the node should complete and then
    // requee and miss the next time. If this is repeated, eventually we should reach the
    // end of the time slot. In which case the node should *not* re-queue and stay complete
    //
    // When the node is then re-queued check that the time has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          time 10:00
    //          time 11:00
    //          time 12:00
    //          time 13:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addTime(TimeAttr(10, 0));
    t1->addTime(TimeAttr(11, 0));
    t1->addTime(TimeAttr(12, 0));
    t1->addTime(TimeAttr(13, 0));
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(9 /*hour*/, 30 /*minutes*/); // *start* at 9:30
    suite->addClock(clockAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // get all the time attributes
    const TimeSeries& ts_10 = t1->timeVec()[0].time_series();
    const TimeSeries& ts_11 = t1->timeVec()[1].time_series();
    const TimeSeries& ts_12 = t1->timeVec()[2].time_series();
    const TimeSeries& ts_13 = t1->timeVec()[3].time_series();
    BOOST_CHECK_MESSAGE(ts_10.is_valid(), "Expected time 10 to be valid since we started at 9:30 ");
    BOOST_CHECK_MESSAGE(ts_11.is_valid(), "Expected time 11 to be valid since we started at 9:30");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid since we started at 9:30");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid since we started at 9:30");

    const TimeSlot& time_10 = t1->timeVec()[0].time_series().get_next_time_slot();
    const TimeSlot& time_11 = t1->timeVec()[1].time_series().get_next_time_slot();
    const TimeSlot& time_12 = t1->timeVec()[2].time_series().get_next_time_slot();
    const TimeSlot& time_13 = t1->timeVec()[3].time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(time_10 == TimeSlot(10, 0),
                        "Expected next time slot of 10:00 but found " << time_10.toString());
    BOOST_CHECK_MESSAGE(time_11 == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << time_11.toString());
    BOOST_CHECK_MESSAGE(time_12 == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << time_12.toString());
    BOOST_CHECK_MESSAGE(time_13 == TimeSlot(13, 0),
                        "Expected next time slot of 13:00 but found " << time_13.toString());

    // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired  ");
    BOOST_CHECK_MESSAGE(ts_11.is_valid(), "Expected time 11 to be valid.");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");

    // Repeat
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");

    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_12.is_valid(), "Expected time 12 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");

    // Repeat *last* time, since all times have expired, we expect task to stay complete.
    // Addtionally since we have *not* re-queued the flag NO_REQUE_IF_SINGLE_TIME_DEP should have remained set
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_12.is_valid(), "Expected time 12 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_13.is_valid(), "Expected time 13 to be expired due to force cmd");

    /// we will now Re-queue, Since the time is still 10:30, we expect valid from 11:00 and not 10:00
    /// We should also have cleared NO_REQUE_IF_SINGLE_TIME_DEP
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(ts_10.is_valid(), "Expected time 10 to be in valid since we started clock at 9:30 ");
    BOOST_CHECK_MESSAGE(ts_11.is_valid(), "Expected time 11 to be valid");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_2) {
    // Start TIME at 9:30
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user has a time range. In this case the node should complete and then
    // requee and miss the next time slot. If this is repeated, eventually we should reach the
    // end of the time slot. In which case the node should *not* reque and stay complete
    //
    // When the node is then requeed check that the next time slot has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          time 10:00 14:00 01:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addTime(TimeAttr(TimeSlot(10, 0), TimeSlot(14, 0), TimeSlot(1, 0)));
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(9 /*hour*/, 30 /*minutes*/); // start at 99:30
    suite->addClock(clockAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //      PrintStyle style(PrintStyle::MIGRATE); std::cout << defs;

    // since we started at 09:30 the next time slot should be 10:00
    const TimeSlot& next_time_slot = t1->timeVec().back().time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "Expected next time slot of 10:00 but found " << next_time_slot.toString());

    // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
    // It should also advance the next time slot
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << next_time_slot.toString());

    // Repeat, to make sure next_time_slot is advanced
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << next_time_slot.toString());

    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(13, 0),
                        "Expected next time slot of 13:00 but found " << next_time_slot.toString());

    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(14, 0),
                        "Expected next time slot of 14:00 but found " << next_time_slot.toString());

    // Repeat, ** THIS time we have *exceeded* the time range, it should no longer requeue, and should stay complete
    //         Additionally since there is no reque we expect NO_REQUE_IF_SINGLE_TIME_DEP to remain set
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(15, 0),
                        "Expected next time slot of 15:00 but found " << next_time_slot.toString());

    /// we will now Re-queue, Since the time is still 10:30, we expect next_time slot to be 11:00 and not 10:00
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "After Re-queue, Expected next time slot of 10:00 but found " << next_time_slot.toString());
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_3) {
    // Start TIME at 10:30
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user set of time slots. In this case the node should complete and then
    // requee and miss the next time. If this is repeated, eventually we should reach the
    // end of the time slot. In which case the node should *not* re-queue and stay complete
    //
    // When the node is then re-queued check that the time has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          time 10:00
    //          time 11:00
    //          time 12:00
    //          time 13:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addTime(TimeAttr(10, 0));
    t1->addTime(TimeAttr(11, 0));
    t1->addTime(TimeAttr(12, 0));
    t1->addTime(TimeAttr(13, 0));
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(10 /*hour*/, 30 /*minutes*/); // *start* at 10:30
    suite->addClock(clockAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // get all the time attributes
    const TimeSeries& ts_10 = t1->timeVec()[0].time_series();
    const TimeSeries& ts_11 = t1->timeVec()[1].time_series();
    const TimeSeries& ts_12 = t1->timeVec()[2].time_series();
    const TimeSeries& ts_13 = t1->timeVec()[3].time_series();
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be in-valid since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(ts_11.is_valid(), "Expected time 11 to be valid since we started at 10:30");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid since we started at 10:30");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid since we started at 10:30");

    const TimeSlot& time_10 = t1->timeVec()[0].time_series().get_next_time_slot();
    const TimeSlot& time_11 = t1->timeVec()[1].time_series().get_next_time_slot();
    const TimeSlot& time_12 = t1->timeVec()[2].time_series().get_next_time_slot();
    const TimeSlot& time_13 = t1->timeVec()[3].time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(time_10 == TimeSlot(10, 0),
                        "Expected next time slot of 10:00 but found " << time_10.toString());
    BOOST_CHECK_MESSAGE(time_11 == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << time_11.toString());
    BOOST_CHECK_MESSAGE(time_12 == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << time_12.toString());
    BOOST_CHECK_MESSAGE(time_13 == TimeSlot(13, 0),
                        "Expected next time slot of 13:00 but found " << time_13.toString());

    // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired, since we just completed.");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");

    // Repeat
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_12.is_valid(), "Expected time 12 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");

    // Repeat *last* time, since all times have expired, we expect task to complete.
    // Addtionally since we have *not* re-queued the flag NO_REQUE_IF_SINGLE_TIME_DEP should have remained set
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be expired since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(!ts_11.is_valid(), "Expected time 11 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_12.is_valid(), "Expected time 12 to be expired due to force cmd");
    BOOST_CHECK_MESSAGE(!ts_13.is_valid(), "Expected time 13 to be expired due to force cmd");

    /// we will now Re-queue, Since the time is still 10:30, we expect valid from 11:00 and not 10:00
    /// We should also have cleared NO_REQUE_IF_SINGLE_TIME_DEP
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!ts_10.is_valid(), "Expected time 10 to be in valid since we started clock at 10:30 ");
    BOOST_CHECK_MESSAGE(ts_11.is_valid(), "Expected time 11 to be valid");
    BOOST_CHECK_MESSAGE(ts_12.is_valid(), "Expected time 12 to be valid");
    BOOST_CHECK_MESSAGE(ts_13.is_valid(), "Expected time 13 to be valid");
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_4) {
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user has a time range. In this case the node should complete and then
    // requee and miss the next time slot. If this is repeated, eventually we should reach the
    // end of the time slot. In which case the node should *not* reque and stay complete
    //
    // When the node is then requeed check that the next time slot has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          time 10:00 14:00 01:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    task_ptr t1     = suite->add_task("t1");
    t1->addTime(TimeAttr(TimeSlot(10, 0), TimeSlot(14, 0), TimeSlot(1, 0)));
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(10 /*hour*/, 30 /*minutes*/); // start at 10:30
    suite->addClock(clockAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // since we started at 10:30 the next time slot should be 11:00
    const TimeSlot& next_time_slot = t1->timeVec().back().time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << next_time_slot.toString());

    // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
    // It should also advance the next time slot
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << next_time_slot.toString());

    // Repeat, to make sure next_time_slot is advanced
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(13, 0),
                        "Expected next time slot of 13:00 but found " << next_time_slot.toString());

    // Repeat, to make sure next_time_slot is advanced
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(14, 0),
                        "Expected next time slot of 14:00 but found " << next_time_slot.toString());

    // Repeat, ** THIS time we have *exceeded* the time range, it should no longer requeue, and should stay complete
    //         Additionally since there is no reque we expect NO_REQUE_IF_SINGLE_TIME_DEP to remain set
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(15, 0),
                        "Expected next time slot of 15:00 but found " << next_time_slot.toString());

    /// we will now Re-queue, Since the time is still 10:30, we expect next_time slot to be 11:00 and not 10:00
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(11, 0),
                        "After Re-queue, Expected next time slot of 11:00 but found " << next_time_slot.toString());
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_for_cron) {
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user has a time range. In this case the node should complete and then
    // requee and miss the next time slot. If this is repeated, eventually we should reach the
    // end of the time slot.
    //
    // When the node is then requeed check that the next time slot has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //       task t1
    //          cron 10:00 13:00 01:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(9 /*hour*/, 30 /*minutes*/); // start at 09:30
    suite->addClock(clockAttr);

    task_ptr t1 = suite->add_task("t1");
    ecf::CronAttr cronAttr;
    cronAttr.addTimeSeries(TimeSlot(10, 0), TimeSlot(13, 0), TimeSlot(1, 0));
    t1->addCron(cronAttr);

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // since we started at 09:30 the next time slot should be 10:00
    const TimeSlot& next_time_slot = t1->crons().back().time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "Expected next time slot of 10:00 but found " << next_time_slot.toString());

    // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
    // It should also advance the next time slot
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << next_time_slot.toString());

    // Repeat, to make sure next_time_slot is advanced
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << next_time_slot.toString());

    // Repeat, to make sure next_time_slot is advanced
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(13, 0),
                        "Expected next time slot of 13:00 but found " << next_time_slot.toString());

    // Repeat, ** THIS time we have *exceeded* the time range, However the cron *ALWAYS* re-queues
    //         Additionally since there is no reque we expect NO_REQUE_IF_SINGLE_TIME_DEP to remain set
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, after a requeue");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(14, 0),
                        "Expected next time slot of 14:00 but found " << next_time_slot.toString());

    /// we will now Re-queue, Since the time is still 09:30, we expect next_time slot to be 10:00 and not 14:00
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new RequeueNodeCmd(t1->absNodePath())));
    TestHelper::test_state(t1, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(next_time_slot == TimeSlot(10, 0),
                        "After Re-queue, Expected next time slot of 10:00 but found " << next_time_slot.toString());
}

BOOST_AUTO_TEST_CASE(test_force_interactive_next_time_slot_for_cron_on_family) {
    // This test is custom. When the user interactively forces a node to the complete state,
    // But where the user has a time range. In this case the node should complete and then
    // requee and miss the next time slot. If this is repeated, eventually we should reach the
    // end of the time slot.
    //
    // When the node is then requeed check that the next time slot has been correctly reset.
    ECF_NAME_THIS_TEST();

    //   suite s1
    //     family
    //       cron 10:00 13:00 01:00
    //       task t1
    //          time 11:00
    //       task t2
    //          time 12:00
    //   endsuite
    Defs the_defs;
    suite_ptr suite = the_defs.add_suite("s1");
    ClockAttr clockAttr(15, 12, 2010, false);
    clockAttr.set_gain(9 /*hour*/, 30 /*minutes*/); // start at 09:30
    suite->addClock(clockAttr);

    family_ptr f1 = suite->add_family("f1");
    ecf::CronAttr cronAttr;
    cronAttr.addTimeSeries(TimeSlot(10, 0), TimeSlot(13, 0), TimeSlot(1, 0));
    f1->addCron(cronAttr);

    task_ptr t1 = f1->add_task("t1");
    t1->addTime(TimeAttr(11, 0));

    task_ptr t2 = f1->add_task("t2");
    t2->addTime(TimeAttr(12, 0));

    // before test flags should be clear
    BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");
    BOOST_CHECK_MESSAGE(!t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

    /// begin the suite
    TestHelper::invokeRequest(&the_defs, Cmd_ptr(new BeginCmd("s1", false)));
    TestHelper::test_state(t1, NState::QUEUED);
    TestHelper::test_state(t2, NState::QUEUED);
    //   PrintStyle style(PrintStyle::STATE); cout << the_defs << "\n";

    // since we started at 09:30 the next time slot should be 11:00
    const TimeSlot& t1_next_time_slot = t1->timeVec().back().time_series().get_next_time_slot();
    const TimeSlot& t2_next_time_slot = t2->timeVec().back().time_series().get_next_time_slot();
    BOOST_CHECK_MESSAGE(t1_next_time_slot == TimeSlot(11, 0),
                        "Expected next time slot of 11:00 but found " << t1_next_time_slot.toString());
    BOOST_CHECK_MESSAGE(t2_next_time_slot == TimeSlot(12, 0),
                        "Expected next time slot of 12:00 but found " << t2_next_time_slot.toString());

    // Force the task t1 & t2 to complete state. Since we only have a single time dependency it should expire
    // It should also advance the next time slot
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t1->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::COMPLETE);
    TestHelper::test_state(t2, NState::QUEUED);
    BOOST_CHECK_MESSAGE(!t1->timeVec().back().time_series().is_valid(), "Expected 11:00 time slot to be expired ");

    // Forcing t2 to complete as well should, end up requeueing t1,t2 due to parent cron
    TestHelper::invokeRequest(
        &the_defs,
        Cmd_ptr(
            new ForceCmd(t2->absNodePath(), "complete", false /*recursive */, false /* set Repeat to last value */)));
    TestHelper::test_state(t1, NState::QUEUED);
    TestHelper::test_state(t2, NState::QUEUED);
    BOOST_CHECK_MESSAGE(t1->timeVec().back().time_series().is_valid(), "Expected 11:00 time slot to be valid ");
    BOOST_CHECK_MESSAGE(t2->timeVec().back().time_series().is_valid(), "Expected 12:00 time slot to be valid ");
    BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
    BOOST_CHECK_MESSAGE(!t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),
                        "Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_CASE(test_destroy_log2) {
    Log::destroy();
    fs::remove("test_add_log2.log");
    BOOST_CHECK_MESSAGE(true, "stop boost test form complaining");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

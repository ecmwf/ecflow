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

#include "MockServer.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/cts/task/LabelCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/QueryCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

// ---------------------------------------------------------------------------------------------------------
// Test fixture and helpers
//
// Every test below starts from its own, freshly-built Defs (see make_test_defs()), and submits a single
// QueryCmd to it via invoke_query(). Tests are organised into one BOOST_AUTO_TEST_SUITE per query type
// (state, dstate, repeat, event, meter, limit/limit_max, label, trigger, variable, and variables attached
// to the server itself), so that a failure immediately identifies which attribute kind is affected.
// ---------------------------------------------------------------------------------------------------------

namespace {

// Names of the attributes attached to the fixture built by make_test_defs(), kept as named constants so
// that tests refer to them without repeating (and risking a typo in) the literal names used during setup.
const std::string k_meter_name = "m";
const std::string k_event_name = "event";
const std::string k_label_name = "name";
const std::string k_limit_name = "limit_x";

///
/// Builds the Defs fixture shared by all tests in this file:
///
///  suite suite
///     limit limit_x 12
///     repeat date YMD 20090916 20200916
///     family f
///           edit var2 var2
///           task t1
///               edit var1 var1
///               meter m 0 100 100
///               event event
///           task t2
///               label name value new_value
///     endfamily
///     task task
///  endsuite
///
/// Each test calls this to obtain its own copy of the fixture, so that a test that mutates state (e.g.
/// suspending a node, updating a label) cannot affect any other test.
///
static Defs make_test_defs() {
    Defs defs;

    task_ptr t1 = Task::create("t1");
    t1->addMeter(Meter(k_meter_name, 0, 100, 100));
    t1->addEvent(Event(k_event_name));
    t1->add_variable("var1", "var1");

    suite_ptr s = defs.add_suite("suite");
    s->addLimit(Limit(k_limit_name, 12));
    s->addRepeat(RepeatDate("YMD", 20090916, 20200916, 1));

    family_ptr f = s->add_family("f");
    f->add_variable("var2", "var2");
    f->addTask(t1);

    task_ptr t2 = f->add_task("t2");
    t2->add_label(k_label_name, "value", "");

    s->addTask(Task::create("task"));

    defs.beginAll(); // clears label 'new_value', computes generated variables, etc.
    return defs;
}

///
/// @brief Submits `cmd` to a MockServer wrapping `defs`
///
/// Any failure is reported by `cmd` throwing (typically std::runtime_error),
/// which is propagated so that tests can assert on it directly, e.g. via BOOST_CHECK_THROW.
///
/// @param defs the suite definition being used
/// @param cmd the command being invoked/handled
/// @return the text-based reply.
/// @throws std::runtime_error or whatever the command handling throws when the command fails
///
static std::string invoke(Defs& defs, const Cmd_ptr& cmd) {
    ClientToServerRequest request;
    request.set_cmd(cmd);
    MockServer server(&defs);
    STC_Cmd_ptr result = request.handleRequest(&server);
    return result ? result->get_string() : std::string();
}

///
/// @brief Invokes a QueryCmd on the given `defs`
///
/// This is a onvenience wrapper over `invoke()`, for the common case of submitting a QueryCmd.
/// `path_to_task`
/// defaults to the task used throughout these tests as the (fictitious) caller of the query; it is only
/// relevant for logging and has no bearing on the query result.
///
/// @param defs the suite definition being used
/// @param query_type the query type, one of [state, dstate, event, meter, limit, limit_max, label, variable, trigger]
/// @param path_to_attribute the path to the node whose attribute is being queried
/// @param attribute the name of the attribute being queried (empty for state and dstate)
/// @param path_to_task the path to the task that is invoking the query (for logging only)
/// @return the text-based reply, when command handling is sucessful; otherwise, empty string.
/// @throws std::runtime_error or whatever the command handling throws when the command fails
///
static std::string invoke_query(Defs& defs,
                                const std::string& query_type,
                                const std::string& path_to_attribute,
                                const std::string& attribute    = "",
                                const std::string& path_to_task = "/suite/f/t1") {
    return invoke(defs, Cmd_ptr(new QueryCmd(query_type, path_to_attribute, attribute, path_to_task)));
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_QueryCmd)

BOOST_AUTO_TEST_SUITE(T_State)

//
// Scope: --query state
//

BOOST_AUTO_TEST_CASE(test_query_state_fails_for_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the state of a node that does not exist in the definition fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "state", "/suite/f/t11"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_state_returns_current_state) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the state of an existing node returns its current state.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "state", "/suite");
    BOOST_CHECK_MESSAGE(res == "queued", "expected query state to return queued but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_state_reflects_all_possible_states) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the state of a node correctly reflects every possible NState value.
    //

    Defs defs = make_test_defs();

    // Note: a task out of the repeat on `suite` is selected, because setting a task to complete inside a repeat would
    //       immediately cause it to requeue. The node is mutated directly (bypassing commands) purely to set up the
    //       pre-condition for each query.

    node_ptr task = defs.findAbsNode("/suite/task");

    for (const auto& state : NState::allStates()) {
        task->setStateOnly(NState::toState(state));
        std::string res = invoke_query(defs, "state", task->absNodePath());
        BOOST_CHECK_MESSAGE(res == state, "expected query state to return " << state << " but found: " << res);
    }
}

BOOST_AUTO_TEST_CASE(test_query_state_returns_server_state) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying state at path '/' returns the server's own state, not a node's.
    //

    Defs defs = make_test_defs();
    defs.set_state_only(NState::State::UNKNOWN);
    std::string res = invoke_query(defs, "state", "/");
    BOOST_CHECK_MESSAGE(res == "unknown", "expected query state to return unknown but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_State

BOOST_AUTO_TEST_SUITE(T_DState)

//
// Scope: --query dstate
//

BOOST_AUTO_TEST_CASE(test_query_dstate_fails_for_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the dstate of a node that does not exist in the definition fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "dstate", "/suite/f/t11"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_dstate_returns_current_state) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the dstate of an existing, non-suspended node returns its current state.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "dstate", "/suite/f");
    BOOST_CHECK_MESSAGE(res == "queued", "expected query dstate to return queued but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_dstate_returns_suspended) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: unlike 'state', 'dstate' distinguishes the 'suspended' state.
    //

    Defs defs = make_test_defs();
    invoke(defs, Cmd_ptr(new PathsCmd(PathsCmd::SUSPEND, "/suite/task")));

    std::string res = invoke_query(defs, "dstate", "/suite/task", "", "/suite/task");
    BOOST_CHECK_MESSAGE(res == "suspended", "expected query dstate to return suspended but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_DState

BOOST_AUTO_TEST_SUITE(T_Repeat)

//
// Scope: --query repeat
//

BOOST_AUTO_TEST_CASE(test_query_repeat_fails_when_node_has_no_repeat) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the repeat on a node that has no such attribute fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "repeat", "/suite/f/t1", ""), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_repeat_fails_for_invalid_attribute) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the repeat with an attribute other than 'prev'/'next' fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "repeat", "/suite", "fred", "/suite"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_repeat_returns_current_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the repeat with no attribute returns its current value.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "repeat", "/suite", "", "/suite");
    BOOST_CHECK_MESSAGE(res == "20090916", "expected query repeat to return '20090916' but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_repeat_returns_previous_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the repeat with attribute 'prev' returns the previous value, without modifying the real
    // repeat.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "repeat", "/suite", "prev", "/suite");
    BOOST_CHECK_MESSAGE(res == "20090916", "expected query repeat to return '20090916' but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_repeat_returns_next_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying the repeat with attribute 'next' returns the next value, without modifying the real
    // repeat.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "repeat", "/suite", "next", "/suite");
    BOOST_CHECK_MESSAGE(res == "20090917", "expected query repeat to return '20090917' but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Repeat

BOOST_AUTO_TEST_SUITE(T_Event)

//
// Scope: --query event
//

BOOST_AUTO_TEST_CASE(test_query_event_fails_for_unknown_event_name) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an unknown event name fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "event", "/suite/f/t1", "eventxx"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_event_fails_when_node_has_no_event) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an event on a node that has no such attribute fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "event", "/suite", k_event_name), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_event_fails_for_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an event on an unknown node path fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "event", "xxxx/f/t1", k_event_name), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_event_returns_current_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an event returns its current value ('clear', since it has not been set).
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "event", "/suite/f/t1", k_event_name);
    BOOST_CHECK_MESSAGE(res == "clear", "expected query event to return clear but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_event_returns_set_when_event_is_set) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: once an event is set, querying it returns 'set' rather than the default 'clear'.
    //

    Defs defs = make_test_defs();
    defs.findAbsNode("/suite/f/t1")->set_event(k_event_name);

    std::string res = invoke_query(defs, "event", "/suite/f/t1", k_event_name);
    BOOST_CHECK_MESSAGE(res == "set", "expected query event to return set but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Event

BOOST_AUTO_TEST_SUITE(T_Meter)

//
// Scope: --query meter
//

BOOST_AUTO_TEST_CASE(test_query_meter_fails_for_unknown_meter_name) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an unknown meter name fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "meter", "/suite/f/t1", "meterxx"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_meter_fails_when_node_has_no_meter) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a meter on a node that has no such attribute fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "meter", "/suite", k_meter_name), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_meter_returns_current_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a meter returns its current value.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "meter", "/suite/f/t1", k_meter_name);
    BOOST_CHECK_MESSAGE(res == "0", "expected query meter to return 0 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_meter_returns_updated_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: after the meter's value is changed, querying it returns the new value, not just 0.
    //

    Defs defs = make_test_defs();
    defs.findAbsNode("/suite/f/t1")->set_meter(k_meter_name, 42);

    std::string res = invoke_query(defs, "meter", "/suite/f/t1", k_meter_name);
    BOOST_CHECK_MESSAGE(res == "42", "expected query meter to return 42 but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Meter

BOOST_AUTO_TEST_SUITE(T_Limit)

//
// Scope: --query limit / --query limit_max
//

BOOST_AUTO_TEST_CASE(test_query_limit_fails_for_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a limit on an unknown node path fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "limit", "xxxx/f/t1", k_limit_name, "/suite"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_limit_max_fails_for_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a limit's maximum value on an unknown node path fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "limit_max", "xxxx/f/t1", k_limit_name, "/suite"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_limit_returns_current_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a limit returns its current value.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "limit", "/suite", k_limit_name);
    BOOST_CHECK_MESSAGE(res == "0", "expected query limit to return 0 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_limit_max_returns_maximum_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a limit's maximum value returns the configured maximum.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "limit_max", "/suite", k_limit_name);
    BOOST_CHECK_MESSAGE(res == "12", "expected query limit_max to return 12 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_limit_returns_updated_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: after some of the limit's tokens have been consumed, querying it returns the updated
    // value, not the initial 0.
    //

    Defs defs = make_test_defs();
    defs.findAbsNode("/suite")->find_limit(k_limit_name)->increment(5, "/suite/f/t1");

    std::string res = invoke_query(defs, "limit", "/suite", k_limit_name);
    BOOST_CHECK_MESSAGE(res == "5", "expected query limit to return 5 but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Limit

BOOST_AUTO_TEST_SUITE(T_Label)

//
// Scope: --query label
//

BOOST_AUTO_TEST_CASE(test_query_label_fails_when_node_has_no_label) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a label on a node that has no such attribute fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "label", "/suite/f/t1", k_label_name), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_label_fails_for_unknown_label_name) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an unknown label name fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "label", "/suite/f/t2", "fred", "/suite/f/t2"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_label_returns_initial_value) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying a label returns its initial value before any update.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "label", "/suite/f/t2", k_label_name, "/suite/f/t2");
    BOOST_CHECK_MESSAGE(res == "value", "expected query label to return 'value' but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_label_returns_new_value_after_update) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: after a LabelCmd updates the label's new value, querying the label returns that updated value.
    //

    Defs defs = make_test_defs();
    invoke(defs,
           Cmd_ptr(new LabelCmd("/suite/f/t2",
                                Submittable::DUMMY_JOBS_PASSWORD(),
                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                1,
                                k_label_name,
                                "new_value")));

    std::string res = invoke_query(defs, "label", "/suite/f/t2", k_label_name, "/suite/f/t2");
    BOOST_CHECK_MESSAGE(res == "new_value", "expected query label to return 'new_value' but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Label

BOOST_AUTO_TEST_SUITE(T_Trigger)

//
// Scope: --query trigger
//

BOOST_AUTO_TEST_CASE(test_query_trigger_fails_to_parse) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression that fails to parse must be rejected, regardless of which node is used to
    // evaluate it.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "trigger", "/suite", "1 == "), std::runtime_error);
    BOOST_CHECK_THROW(invoke_query(defs, "trigger", "/suite/f/t1", "1 == "), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_fails_for_reference_to_unknown_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression referencing a node that does not exist must be rejected.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "trigger", "/suite/f/t1", "t3 == complete"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_evaluates_to_false) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression referencing another node's state evaluates to 'false' when condition does not hold.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "t2 == complete");
    BOOST_CHECK_MESSAGE(res == "false", "expected query trigger to return false but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_evaluates_to_true) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a tautological trigger expression evaluates to 'true'.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "1 == 1");
    BOOST_CHECK_MESSAGE(res == "true", "expected query trigger to return true but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_supports_node_scoped_variable_reference) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression can reference a variable defined on the node itself.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "/suite/f/t1:var1 == 0");
    BOOST_CHECK_MESSAGE(res == "true", "expected query trigger to return true but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_supports_parent_scoped_variable_reference) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression can reference a variable defined on a parent family.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "/suite/f:var2 == 0");
    BOOST_CHECK_MESSAGE(res == "true", "expected query trigger to return true but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_supports_meter_reference) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression can reference a meter's value.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "/suite/f/t1:m == 0");
    BOOST_CHECK_MESSAGE(res == "true", "expected query trigger to return true but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_supports_event_reference) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression can reference an event, evaluating to 'false' while the event is clear.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite/f/t1", "/suite/f/t1:event");
    BOOST_CHECK_MESSAGE(res == "false", "expected query trigger to return false but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_trigger_supports_repeat_reference) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a trigger expression can reference a repeat's current value.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "trigger", "/suite", "/suite:YMD == 20090916");
    BOOST_CHECK_MESSAGE(res == "true", "expected query trigger to return true but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Trigger

BOOST_AUTO_TEST_SUITE(T_Variable)

//
// Scope: --query variable (node-scoped)
//

BOOST_AUTO_TEST_CASE(test_query_variable_fails_for_unknown_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an unknown variable name fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "variable", "/suite/f/t1", "XXXX"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_variable_returns_value_defined_on_node) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a variable defined directly on the queried node is found.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "variable", "/suite/f/t1", "var1");
    BOOST_CHECK_MESSAGE(res == "var1", "expected query variable to return var1 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_returns_value_defined_on_parent_family) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: the variable search walks up the node tree, so a variable defined on a parent family is found
    // even when queried via a child task's path.
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "variable", "/suite/f/t1", "var2");
    BOOST_CHECK_MESSAGE(res == "var2", "expected query variable to return var2 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_returns_repeat_value_defined_on_parent_suite) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: the variable search also finds repeat/generated variables defined further up the tree (here,
    // the YMD repeat defined on the parent suite).
    //

    Defs defs       = make_test_defs();
    std::string res = invoke_query(defs, "variable", "/suite/f/t1", "YMD");
    BOOST_CHECK_MESSAGE(res == "20090916", "expected query variable to return 20090916 but found: " << res);
}

BOOST_AUTO_TEST_SUITE_END() // T_Variable

BOOST_AUTO_TEST_SUITE(T_ServerVariable)

//
// Scope: --query variable /:name (path '/' represents the server itself)
//

BOOST_AUTO_TEST_CASE(test_query_variable_returns_user_defined_server_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a user-defined variable attached to the server itself (path '/') can be queried via
    // `--query variable /:name`.
    //

    Defs defs = make_test_defs();
    defs.server_state().add_or_update_user_variables("SERVER_VAR", "server_var_value");

    std::string res = invoke_query(defs, "variable", "/", "SERVER_VAR");
    BOOST_CHECK_MESSAGE(res == "server_var_value",
                        "expected query variable to return server_var_value but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_returns_generated_server_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a generated/server-defined variable (as opposed to a user-defined one) can also be queried via
    // path '/'.
    //

    Defs defs = make_test_defs();
    defs.server_state().set_server_variables({Variable("ECF_PORT", "3141")});

    std::string res = invoke_query(defs, "variable", "/", "ECF_PORT");
    BOOST_CHECK_MESSAGE(res == "3141", "expected query variable to return 3141 but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_prefers_user_defined_over_generated_server_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: when both a user-defined and a server-defined (generated) variable exist with the same name,
    // the user-defined variable's value takes precedence.
    //

    Defs defs = make_test_defs();
    defs.server_state().set_server_variables({Variable("ECF_PORT", "3141")});
    defs.server_state().add_or_update_user_variables("ECF_PORT", "user_override_value");

    std::string res = invoke_query(defs, "variable", "/", "ECF_PORT");
    BOOST_CHECK_MESSAGE(res == "user_override_value",
                        "expected query variable to return user_override_value (user variable should take "
                        "precedence) but found: "
                            << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_returns_empty_value_for_server_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: a server variable can legitimately hold an empty value; querying it must succeed and return an
    // empty string, rather than being (incorrectly) reported as "not found". Regression test for
    // ECFLOW-2108.
    //

    Defs defs = make_test_defs();
    defs.server_state().add_or_update_user_variables("EMPTY_SERVER_VAR", "");

    // If the query incorrectly throws, `res` is left at this sentinel (non-empty) value, so the
    // BOOST_CHECK_MESSAGE below still fails instead of masking the regression as a false pass.
    std::string res = "<not set>";
    BOOST_CHECK_NO_THROW(res = invoke_query(defs, "variable", "/", "EMPTY_SERVER_VAR"));
    BOOST_CHECK_MESSAGE(res.empty(), "expected query variable to return an empty string but found: " << res);
}

BOOST_AUTO_TEST_CASE(test_query_variable_fails_for_unknown_server_variable) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: querying an unknown server variable fails.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "variable", "/", "XXXX_SERVER_VAR"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_query_fails_for_unsupported_query_type_against_server) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: the server (path '/') only supports 'state' and 'variable' queries; any other query type must
    // fail.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "event", "/", "event"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END() // T_ServerVariable

BOOST_AUTO_TEST_CASE(test_query_print_includes_calling_task_path) {
    ECF_NAME_THIS_TEST();

    //
    // Test: when a QueryCmd is constructed with a non-empty path_to_task (as when invoked from a
    // '.ecf' script), print() includes that path, distinct from path_to_attribute, so the log can be
    // traced back to the calling task.
    //

    QueryCmd cmd("state", "/suite/f/t1", "", "/suite/f/t2");
    std::string out;
    cmd.print(out);
    BOOST_CHECK_MESSAGE(out.find("/suite/f/t2") != std::string::npos,
                        "expected print() output to include the calling task path but found: " << out);
}

BOOST_AUTO_TEST_CASE(test_query_fails_for_unrecognised_query_type) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_query_cmd.log");

    //
    // Test: an unrecognised query type (not one of state/dstate/repeat/event/meter/label/trigger/limit/
    // limit_max/variable) must fail.
    //

    Defs defs = make_test_defs();
    BOOST_CHECK_THROW(invoke_query(defs, "xxxxx", "/suite/f/t1", k_event_name), std::runtime_error);

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END() // T_QueryCmd

BOOST_AUTO_TEST_SUITE_END() // U_Base

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Pins the behaviour of Node::changeRepeat, used by `--alter change repeat`.
///

namespace {

///
/// @brief Enables the server-side change numbers for the lifetime of the object.
///
struct ServerScope
{
    ServerScope() { Ecf::set_server(true); }
    ~ServerScope() { Ecf::set_server(false); }
    ServerScope(const ServerScope&)            = delete;
    ServerScope& operator=(const ServerScope&) = delete;
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_NodeChangeRepeat)

BOOST_AUTO_TEST_CASE(change_repeat_updates_the_value_only) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s  = defs.add_suite("s");
    family_ptr f = s->add_family("f");
    f->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));

    ServerScope server;
    auto node_change_no   = f->state_change_no();
    auto repeat_change_no = f->repeat().state_change_no();

    f->changeRepeat("20260315");

    BOOST_CHECK_EQUAL(f->repeat().value(), 20260315);
    BOOST_CHECK_EQUAL(f->repeat().start(), 20260101);
    BOOST_CHECK_EQUAL(f->repeat().end(), 20261231);
    BOOST_CHECK_EQUAL(f->repeat().step(), 1);
    BOOST_CHECK_GT(f->repeat().state_change_no(), repeat_change_no);
    // Only the Repeat records the change; the node change number, which triggers a resend of all the node
    // attributes during an incremental sync, is left unchanged
    BOOST_CHECK_EQUAL(f->state_change_no(), node_change_no);
}

BOOST_AUTO_TEST_CASE(change_repeat_leaves_the_node_state_unchanged) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    t->addRepeat(RepeatInteger("N", 0, 1, 1));

    ServerScope server;
    defs.beginAll();

    // A queued node stays queued
    BOOST_REQUIRE_EQUAL(t->state(), NState::QUEUED);
    t->changeRepeat("1");
    BOOST_CHECK_EQUAL(t->repeat().value(), 1);
    BOOST_CHECK_EQUAL(t->state(), NState::QUEUED);

    // Completing the task increments the Repeat; once past its end, the task stays complete
    t->set_state(NState::COMPLETE);
    BOOST_REQUIRE_EQUAL(t->state(), NState::COMPLETE);
    BOOST_REQUIRE(!t->repeat().valid());
    BOOST_REQUIRE_EQUAL(t->repeat().value(), 2);

    // A complete node stays complete, even though its Repeat becomes valid again
    t->changeRepeat("0");
    BOOST_CHECK_EQUAL(t->repeat().value(), 0);
    BOOST_CHECK(t->repeat().valid());
    BOOST_CHECK_EQUAL(t->state(), NState::COMPLETE);
}

BOOST_AUTO_TEST_CASE(change_repeat_is_seen_at_once_by_variables_and_triggers) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s  = defs.add_suite("s");
    family_ptr f = s->add_family("f");
    f->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));
    task_ptr t  = f->add_task("t");
    task_ptr t2 = s->add_task("t2");
    t2->add_trigger("f:YMD eq 20260315 and f:YMD_DD eq 15");
    t->addRepeat(RepeatInteger("N", 0, 10, 1));
    defs.beginAll();

    BOOST_REQUIRE(!t2->triggerAst()->evaluate());
    // the generated variables of a task are created on first use
    BOOST_REQUIRE_EQUAL(t->findGenVariable("N").value(), "0");

    f->changeRepeat("20260315");
    t->changeRepeat("4");

    // Once created, the generated variable named after the Repeat is refreshed only on requeue or job submission
    BOOST_CHECK_EQUAL(f->findGenVariable("YMD").value(), "20260101");
    BOOST_CHECK_EQUAL(f->findGenVariable("YMD_DD").value(), "15");
    BOOST_CHECK_EQUAL(t->findGenVariable("N").value(), "0");

    // Variable lookup, substitution and triggers read the Repeat itself
    std::string value;
    BOOST_REQUIRE(t->findParentVariableValue("YMD", value));
    BOOST_CHECK_EQUAL(value, "20260315");
    std::string cmd = "%YMD% %YMD_DD% %YMD_JULIAN% %N%";
    BOOST_REQUIRE(t->variableSubstitution(cmd));
    BOOST_CHECK_EQUAL(cmd, "20260315 15 " + std::to_string(ecf::CalendarDate(20260315).as_julian_day().value()) + " 4");
    BOOST_CHECK(t2->triggerAst()->evaluate());
}

BOOST_AUTO_TEST_CASE(change_repeat_without_repeat_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");

    std::string error;
    try {
        t->changeRepeat("1");
    }
    catch (const std::runtime_error& e) {
        error = e.what();
    }
    BOOST_CHECK_MESSAGE(error.find("Could not find repeat on /s/t") != std::string::npos,
                        "unexpected error: " << error);
    BOOST_CHECK(t->repeat().empty());
}

BOOST_AUTO_TEST_CASE(change_repeat_with_invalid_value_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    t->addRepeat(RepeatInteger("N", 0, 10, 1));

    BOOST_CHECK_THROW(t->changeRepeat("11"), std::runtime_error);
    BOOST_CHECK_THROW(t->changeRepeat("x"), std::runtime_error);
    BOOST_CHECK_EQUAL(t->repeat().value(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // T_NodeChangeRepeat

BOOST_AUTO_TEST_SUITE_END() // U_Node

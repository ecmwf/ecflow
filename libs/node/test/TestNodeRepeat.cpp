// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Aspect.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Pins how a node holds its Repeat: adding and deleting it, applying the mementos of an incremental sync,
///        and resolving variables named like the Repeat.
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

///
/// @brief Retrieves the message of the exception thrown by the given function, or an empty string.
///
template <typename F>
std::string error_of(F&& f) {
    try {
        f();
    }
    catch (const std::exception& e) {
        return e.what();
    }
    return std::string{};
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_NodeRepeat)

/*
 * Test Suite: ::add_delete
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(add_delete)

BOOST_AUTO_TEST_CASE(add_repeat_twice_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");
    t->addRepeat(RepeatInteger("N", 0, 10, 1));

    auto error = error_of([&]() { t->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1)); });
    BOOST_CHECK_MESSAGE(error.find("Repeat of name 'N' already exists") != std::string::npos, error);
    BOOST_CHECK(t->repeat().repeatBase()->isInteger());
}

BOOST_AUTO_TEST_CASE(repeat_and_cron_conflict_depends_on_the_order) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");

    {
        // a cron, then a Repeat: refused
        task_ptr t = s->add_task("cron_first");
        t->addCron(ecf::CronAttr::create("10:00"));
        auto error = error_of([&]() { t->addRepeat(RepeatInteger("N", 0, 10, 1)); });
        BOOST_CHECK_MESSAGE(error.find("already has a cron") != std::string::npos, error);
        BOOST_CHECK(t->repeat().empty());
    }
    {
        // a Repeat, then a cron without increment: accepted
        task_ptr t = s->add_task("repeat_first");
        t->addRepeat(RepeatInteger("N", 0, 10, 1));
        BOOST_CHECK_NO_THROW(t->addCron(ecf::CronAttr::create("10:00")));
        BOOST_CHECK_EQUAL(t->crons().size(), 1u);
    }
    {
        // a Repeat, then a cron with an increment: refused
        task_ptr t = s->add_task("repeat_first_series");
        t->addRepeat(RepeatInteger("N", 0, 10, 1));
        BOOST_CHECK_THROW(t->addCron(ecf::CronAttr::create("00:00 23:00 01:00")), std::runtime_error);
        BOOST_CHECK(t->crons().empty());
    }
}

BOOST_AUTO_TEST_CASE(delete_repeat_without_repeat_changes_nothing) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");

    ServerScope server;
    auto change_no = t->state_change_no();
    BOOST_CHECK_NO_THROW(t->deleteRepeat());
    BOOST_CHECK(t->repeat().empty());
    BOOST_CHECK_EQUAL(t->state_change_no(), change_no);
}

BOOST_AUTO_TEST_CASE(delete_repeat_removes_the_generated_variables) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");
    t->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));
    BOOST_REQUIRE(!t->findGenVariable("YMD_YYYY").empty());

    t->deleteRepeat();
    BOOST_CHECK(t->findGenVariable("YMD").empty());
    BOOST_CHECK(t->findGenVariable("YMD_YYYY").empty());

    // ... and a Repeat can be added again
    BOOST_CHECK_NO_THROW(t->addRepeat(RepeatInteger("N", 0, 10, 1)));
}

BOOST_AUTO_TEST_CASE(refused_add_repeat_leaves_the_repeat_on_the_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");

    // A Repeat without name is assigned before its generated variables are named, which then fails
    auto error = error_of([&]() { t->addRepeat(RepeatEnumerated()); });
    BOOST_CHECK_MESSAGE(error.find("Invalid name") != std::string::npos, error);
    BOOST_CHECK(!t->repeat().empty());
    BOOST_CHECK(t->repeat().name().empty());

    // ... so that no other Repeat can be added
    error = error_of([&]() { t->addRepeat(RepeatInteger("N", 0, 10, 1)); });
    BOOST_CHECK_MESSAGE(error.find("Repeat of name '' already exists") != std::string::npos, error);
}

BOOST_AUTO_TEST_SUITE_END() // add_delete

/*
 * Test Suite: ::memento
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(memento)

BOOST_AUTO_TEST_CASE(repeat_memento_onto_existing_repeat_copies_the_value_only) {
    ECF_NAME_THIS_TEST();

    std::vector<ecf::Aspect::Type> aspects;
    {
        Defs defs;
        task_ptr t = defs.add_suite("s")->add_task("t");
        t->addRepeat(RepeatInteger("N", 0, 10, 1));

        Repeat other(RepeatInteger("N", 0, 20, 2));
        other.set_value(6);
        NodeRepeatMemento memento(other);
        static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);

        BOOST_CHECK_EQUAL(t->repeat().value(), 6);
        BOOST_CHECK_EQUAL(t->repeat().start(), 0);
        BOOST_CHECK_EQUAL(t->repeat().end(), 10);
        BOOST_CHECK_EQUAL(t->repeat().step(), 1);
    }
    {
        // even another kind of Repeat only gives its value
        Defs defs;
        task_ptr t = defs.add_suite("s")->add_task("t");
        t->addRepeat(RepeatInteger("N", 0, 10, 1));

        Repeat other(RepeatDate("YMD", 20260101, 20261231, 1));
        other.set_value(20260315);
        NodeRepeatMemento memento(other);
        static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);

        BOOST_CHECK(t->repeat().repeatBase()->isInteger());
        BOOST_CHECK_EQUAL(t->repeat().value(), 20260315);
        BOOST_CHECK(!t->repeat().valid());
    }
}

BOOST_AUTO_TEST_CASE(repeat_memento_onto_node_without_repeat_adds_it) {
    ECF_NAME_THIS_TEST();

    std::vector<ecf::Aspect::Type> aspects;
    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");

    Repeat other(RepeatDate("YMD", 20260101, 20261231, 1));
    other.set_value(20260315);
    NodeRepeatMemento memento(other);
    static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);

    BOOST_CHECK(t->repeat() == other);
    BOOST_CHECK_EQUAL(t->findGenVariable("YMD_DD").value(), "15");
}

BOOST_AUTO_TEST_CASE(repeat_memento_aspect_only_changes_nothing) {
    ECF_NAME_THIS_TEST();

    std::vector<ecf::Aspect::Type> aspects;
    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");
    t->addRepeat(RepeatInteger("N", 0, 10, 1));

    Repeat other(RepeatInteger("N", 0, 10, 1));
    other.set_value(6);
    NodeRepeatMemento memento(other);
    static_cast<Node*>(t.get())->set_memento(&memento, aspects, true);

    BOOST_CHECK_EQUAL(t->repeat().value(), 0);
    BOOST_REQUIRE_EQUAL(aspects.size(), 1u);
    BOOST_CHECK_EQUAL(aspects[0], ecf::Aspect::REPEAT);
}

BOOST_AUTO_TEST_CASE(repeat_index_memento_copies_the_value_verbatim) {
    ECF_NAME_THIS_TEST();

    std::vector<ecf::Aspect::Type> aspects;

    auto index_memento = [](long value) {
        Repeat source(RepeatInteger("N", 0, 1000, 1));
        source.set_value(value);
        return NodeRepeatIndexMemento(source);
    };

    {
        // a node without Repeat ignores it
        Defs defs;
        task_ptr t   = defs.add_suite("s")->add_task("t");
        auto memento = index_memento(3);
        BOOST_CHECK_NO_THROW(static_cast<Node*>(t.get())->set_memento(&memento, aspects, false));
        BOOST_CHECK(t->repeat().empty());
    }
    {
        // a repeat day ignores it
        Defs defs;
        task_ptr t = defs.add_suite("s")->add_task("t");
        t->addRepeat(RepeatDay(2));
        auto memento = index_memento(5);
        static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);
        BOOST_CHECK_EQUAL(t->repeat().step(), 2);
    }
    {
        // a value past the end is copied as is
        Defs defs;
        task_ptr t = defs.add_suite("s")->add_task("t");
        t->addRepeat(RepeatInteger("N", 0, 10, 2));
        auto memento = index_memento(12);
        static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);
        BOOST_CHECK_EQUAL(t->repeat().value(), 12);
        BOOST_CHECK(!t->repeat().valid());
    }
    {
        // ... and so is an index past the last member of a list
        Defs defs;
        task_ptr t = defs.add_suite("s")->add_task("t");
        t->addRepeat(RepeatDateList("D", {20260101, 20260102, 20260103}));
        auto memento = index_memento(3);
        static_cast<Node*>(t.get())->set_memento(&memento, aspects, false);
        BOOST_CHECK_EQUAL(t->repeat().index_or_value(), 3);
        BOOST_CHECK(!t->repeat().valid());
    }
}

BOOST_AUTO_TEST_SUITE_END() // memento

/*
 * Test Suite: ::variables
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(variables)

BOOST_AUTO_TEST_CASE(user_variable_shadows_the_repeat_of_the_same_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");
    t->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));
    t->add_variable("YMD", "user");
    t->add_variable("YMD_YYYY", "user_year");

    std::string value;
    BOOST_REQUIRE(t->findParentVariableValue("YMD", value));
    BOOST_CHECK_EQUAL(value, "user");
    BOOST_REQUIRE(t->findParentVariableValue("YMD_YYYY", value));
    BOOST_CHECK_EQUAL(value, "user_year");
    BOOST_REQUIRE(t->findParentVariableValue("YMD_MM", value));
    BOOST_CHECK_EQUAL(value, "01");
}

BOOST_AUTO_TEST_CASE(repeat_shadows_a_generated_variable_of_the_same_name) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    task_ptr t = defs.add_suite("s")->add_task("t");
    t->addRepeat(RepeatInteger("ECF_TRYNO", 5, 10, 1));
    t->update_generated_variables();

    std::string value;
    BOOST_REQUIRE(t->findParentVariableValue("ECF_TRYNO", value));
    BOOST_CHECK_EQUAL(value, "5");
}

BOOST_AUTO_TEST_SUITE_END() // variables

BOOST_AUTO_TEST_SUITE_END() // T_NodeRepeat

BOOST_AUTO_TEST_SUITE_END() // U_Node

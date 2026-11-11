// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Owner)

namespace {

struct OwnedDefs
{
    Defs defs;
    suite_ptr s;
    family_ptr f;
    task_ptr t;

    OwnedDefs() {
        s = defs.add_suite("s");
        f = s->add_family("f");
        t = f->add_task("t");
    }
};

} // namespace

BOOST_AUTO_TEST_CASE(test_owner_add_change_delete) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    for (Node* node : {static_cast<Node*>(d.s.get()), static_cast<Node*>(d.f.get()), static_cast<Node*>(d.t.get())}) {
        BOOST_CHECK(node->get_owner() == nullptr);

        node->addOwner(OwnerAttr::create("alice"));
        BOOST_REQUIRE(node->get_owner() != nullptr);
        BOOST_CHECK_EQUAL(node->get_owner()->value(), "alice");

        node->changeOwner(OwnerAttr::create("bob:ops"));
        BOOST_REQUIRE(node->get_owner() != nullptr);
        BOOST_CHECK_EQUAL(node->get_owner()->value(), "bob:ops");

        node->deleteOwner();
        BOOST_CHECK(node->get_owner() == nullptr);
    }
}

BOOST_AUTO_TEST_CASE(test_owner_add_twice_is_rejected) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    d.f->addOwner(OwnerAttr::create("alice"));
    BOOST_CHECK_THROW(d.f->addOwner(OwnerAttr::create("bob")), std::runtime_error);
    BOOST_CHECK_EQUAL(d.f->get_owner()->value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_owner_change_and_delete_without_owner) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    BOOST_CHECK_THROW(d.t->changeOwner(OwnerAttr::create("bob")), std::runtime_error);
    BOOST_CHECK(d.t->get_owner() == nullptr);
    BOOST_CHECK_THROW(d.t->deleteOwner(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_empty_owner_is_rejected) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    BOOST_CHECK_THROW(d.t->addOwner(OwnerAttr{}), std::runtime_error);
    BOOST_CHECK(d.t->get_owner() == nullptr);

    d.t->addOwner(OwnerAttr::create("alice"));
    BOOST_CHECK_THROW(d.t->changeOwner(OwnerAttr{}), std::runtime_error);
    BOOST_CHECK_EQUAL(d.t->get_owner()->value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_owner_on_alias_is_rejected) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    alias_ptr alias = d.t->add_alias_only();
    BOOST_CHECK_THROW(alias->addOwner(OwnerAttr::create("alice")), std::runtime_error);
    BOOST_CHECK(alias->get_owner() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_owner_state_change_is_recorded) {
    ECF_NAME_THIS_TEST();

    // State change numbers are only incremented in the server
    struct ServerScope
    {
        ServerScope() { Ecf::set_server(true); }
        ~ServerScope() { Ecf::set_server(false); }
    } server_scope;

    OwnedDefs d;
    auto before = d.t->state_change_no();
    d.t->addOwner(OwnerAttr::create("alice"));
    auto after_add = d.t->state_change_no();
    BOOST_CHECK_GT(after_add, before);
    d.t->changeOwner(OwnerAttr::create("bob"));
    auto after_change = d.t->state_change_no();
    BOOST_CHECK_GT(after_change, after_add);
    d.t->deleteOwner();
    BOOST_CHECK_GT(d.t->state_change_no(), after_change);
}

BOOST_AUTO_TEST_CASE(test_owner_copy_assignment_and_equality) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    d.s->addOwner(OwnerAttr::create("alice"));
    d.t->addOwner(OwnerAttr::create("bob:ops"));

    Defs copy(d.defs);
    BOOST_CHECK_MESSAGE(copy == d.defs, "Copy constructor must preserve the Owners");
    BOOST_REQUIRE(copy.findAbsNode("/s/f/t"));
    BOOST_CHECK_EQUAL(copy.findAbsNode("/s/f/t")->get_owner()->value(), "bob:ops");

    Defs assigned;
    assigned = d.defs;
    BOOST_CHECK_MESSAGE(assigned == d.defs, "Assignment must preserve the Owners");

    // Equality detects a different, a missing and an extra Owner
    copy.findAbsNode("/s/f/t")->changeOwner(OwnerAttr::create("bob"));
    BOOST_CHECK_MESSAGE(!(copy == d.defs), "A different Owner must make the definitions differ");
    copy.findAbsNode("/s/f/t")->deleteOwner();
    BOOST_CHECK_MESSAGE(!(copy == d.defs), "A missing Owner must make the definitions differ");
    assigned.findAbsNode("/s/f")->addOwner(OwnerAttr::create("charlie"));
    BOOST_CHECK_MESSAGE(!(assigned == d.defs), "An extra Owner must make the definitions differ");
}

BOOST_AUTO_TEST_CASE(test_owner_node_assignment) {
    ECF_NAME_THIS_TEST();

    Task owned("owned");
    owned.addOwner(OwnerAttr::create("alice:ops"));
    Task unowned("unowned");

    // unowned <- owned: the Owner is copied
    Task target("target");
    target = owned;
    BOOST_REQUIRE(target.get_owner() != nullptr);
    BOOST_CHECK_EQUAL(target.get_owner()->value(), "alice:ops");

    // owned <- unowned: the Owner is removed
    target = unowned;
    BOOST_CHECK(target.get_owner() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_owner_counted_in_stats) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    BOOST_CHECK_MESSAGE(d.defs.stats().find("owners") == std::string::npos, "No Owner line expected without Owners");
    d.s->addOwner(OwnerAttr::create("alice"));
    d.t->addOwner(OwnerAttr::create("bob"));
    BOOST_CHECK_MESSAGE(d.defs.stats().find("owners              2\n") != std::string::npos, d.defs.stats());
}

BOOST_AUTO_TEST_CASE(test_owner_written_in_every_style) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    d.f->addDefStatus(DState::COMPLETE);
    d.f->addOwner(OwnerAttr::create("charlie:ops"));

    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        std::string text = ecf::as_string(d.defs, style);
        auto owner_pos   = text.find("owner charlie:ops");
        BOOST_CHECK_MESSAGE(owner_pos != std::string::npos,
                            "Owner missing in style " << PrintStyle::to_string(style) << ":\n"
                                                      << text);
        auto defstatus_pos = text.find("defstatus complete");
        BOOST_CHECK_MESSAGE(defstatus_pos != std::string::npos && defstatus_pos < owner_pos,
                            "Owner expected right after defstatus in style " << PrintStyle::to_string(style));
        BOOST_CHECK_EQUAL(text.find("owner", owner_pos + 1), std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(test_no_owner_written_without_owner) {
    ECF_NAME_THIS_TEST();

    OwnedDefs d;
    // The definition format of a node without Owner is unchanged
    BOOST_CHECK_EQUAL(ecf::as_string(d.defs, PrintStyle::DEFS),
                      "#" + ecf::Version::full() +
                          "\nsuite s\n  family f\n    task t\n  endfamily\nendsuite\n# enddef\n");

    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        std::string text = ecf::as_string(d.defs, style);
        BOOST_CHECK_MESSAGE(text.find("owner") == std::string::npos,
                            "No Owner expected in style " << PrintStyle::to_string(style) << ":\n"
                                                          << text);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

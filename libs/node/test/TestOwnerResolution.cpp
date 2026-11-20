// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Ownership.hpp"
#include "ecflow/node/ServerState.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_OwnerResolution)

namespace {

struct Tree
{
    Defs defs;
    suite_ptr s;
    family_ptr f;
    task_ptr t;
    alias_ptr a;

    Tree() {
        s = defs.add_suite("s");
        f = s->add_family("f");
        t = f->add_task("t");
        a = t->add_alias_only();
    }
};

void check_resolved(const Node& node, const std::string& expected_owner, const Node* expected_origin) {
    auto resolved = ownership::resolve(node);
    BOOST_REQUIRE_MESSAGE(resolved.has_value(), "Expected an effective Owner for " << node.absNodePath());
    BOOST_CHECK_EQUAL(resolved->owner.value(), expected_owner);
    BOOST_CHECK(resolved->origin == expected_origin);
    BOOST_CHECK_EQUAL(resolved->origin_path(), expected_origin ? expected_origin->absNodePath() : "server");
}

} // namespace

BOOST_AUTO_TEST_CASE(test_resolve_without_any_owner) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    for (const Node* node : {static_cast<const Node*>(tree.s.get()),
                             static_cast<const Node*>(tree.f.get()),
                             static_cast<const Node*>(tree.t.get()),
                             static_cast<const Node*>(tree.a.get())}) {
        BOOST_CHECK_MESSAGE(!ownership::resolve(*node).has_value(), "No Owner expected for " << node->absNodePath());
    }
}

BOOST_AUTO_TEST_CASE(test_resolve_own_and_inherited_owner) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    tree.s->addOwner(OwnerAttr::create("alice"));
    check_resolved(*tree.s, "alice", tree.s.get());
    check_resolved(*tree.f, "alice", tree.s.get());
    check_resolved(*tree.t, "alice", tree.s.get());

    // The closest Owner wins
    tree.f->addOwner(OwnerAttr::create("charlie:ops"));
    check_resolved(*tree.s, "alice", tree.s.get());
    check_resolved(*tree.f, "charlie:ops", tree.f.get());
    check_resolved(*tree.t, "charlie:ops", tree.f.get());

    tree.t->addOwner(OwnerAttr::create("bob"));
    check_resolved(*tree.t, "bob", tree.t.get());
}

BOOST_AUTO_TEST_CASE(test_resolve_alias_through_its_task) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    tree.f->addOwner(OwnerAttr::create("charlie"));
    check_resolved(*tree.a, "charlie", tree.f.get());

    tree.t->addOwner(OwnerAttr::create("bob"));
    check_resolved(*tree.a, "bob", tree.t.get());
}

BOOST_AUTO_TEST_CASE(test_resolve_server_owner) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    tree.defs.server_state().set_server_owner(OwnerAttr::create("ecflow-default:ecflow"));
    check_resolved(*tree.t, "ecflow-default:ecflow", nullptr);
    check_resolved(*tree.a, "ecflow-default:ecflow", nullptr);

    // Any Owner in the tree takes precedence over the server Owner
    tree.s->addOwner(OwnerAttr::create("alice"));
    check_resolved(*tree.t, "alice", tree.s.get());

    tree.s->deleteOwner();
    tree.defs.server_state().clear_server_owner();
    BOOST_CHECK(!ownership::resolve(*tree.t).has_value());
}

BOOST_AUTO_TEST_CASE(test_server_owner_only_in_full_synchronisation) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    tree.defs.server_state().set_server_owner(OwnerAttr::create("ecflow-default:ecflow"));

    // Transported in the NET style, used for full synchronisations of clients
    std::string net = ecf::as_string(tree.defs, PrintStyle::NET);
    BOOST_CHECK_MESSAGE(net.find("server_owner:ecflow-default:ecflow") != std::string::npos, net);
    Defs client;
    client.restore_from_string(net);
    BOOST_REQUIRE(client.server_state().server_owner() != nullptr);
    BOOST_CHECK_EQUAL(client.server_state().server_owner()->value(), "ecflow-default:ecflow");
    BOOST_CHECK(client.server_state().compare(tree.defs.server_state()));

    // Never written in checkpoints, nor in definition or state output
    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE}) {
        std::string text = ecf::as_string(tree.defs, style);
        BOOST_CHECK_MESSAGE(text.find("server_owner") == std::string::npos,
                            "Unexpected server Owner in style " << PrintStyle::to_string(style));
    }
    std::string checkpoint;
    ecf::save_as_string(checkpoint, tree.defs);
    BOOST_CHECK(checkpoint.find("server_owner") == std::string::npos);
    Defs restored;
    ecf::restore_from_string(checkpoint, restored);
    BOOST_CHECK(restored.server_state().server_owner() == nullptr);

    // The server Owner is configuration: operator== always ignores it, only compare() considers it
    BOOST_CHECK(restored.server_state() == tree.defs.server_state());
    BOOST_CHECK(!restored.server_state().compare(tree.defs.server_state()));
}

BOOST_AUTO_TEST_CASE(test_server_owner_in_handle_restricted_synchronisation) {
    ECF_NAME_THIS_TEST();

    // A client registered on a subset of the suites receives fabricated definitions, which keep the server Owner
    defs_ptr defs = Defs::create();
    defs->add_suite("s1")->add_task("t");
    defs->add_suite("s2")->add_task("t");
    defs->server_state().set_server_owner(OwnerAttr::create("ecflow-default:ecflow"));

    auto handle   = defs->client_suite_mgr().create_client_suite(false, {"s1"}, "someuser");
    auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
    BOOST_REQUIRE_EQUAL(filtered->suites().size(), std::size_t{1});
    BOOST_REQUIRE(filtered->server_state().server_owner() != nullptr);
    BOOST_CHECK_EQUAL(filtered->server_state().server_owner()->value(), "ecflow-default:ecflow");

    Defs client;
    client.restore_from_string(ecf::as_string(*filtered, PrintStyle::NET));
    auto resolved = ownership::resolve(*client.findAbsNode("/s1/t"));
    BOOST_REQUIRE(resolved.has_value());
    BOOST_CHECK_EQUAL(resolved->owner.value(), "ecflow-default:ecflow");
    BOOST_CHECK_EQUAL(resolved->origin_path(), "server");
}

BOOST_AUTO_TEST_CASE(test_invalid_server_owner_token_is_rejected) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    tree.defs.server_state().set_server_owner(OwnerAttr::create("alice"));
    const std::string net = ecf::as_string(tree.defs, PrintStyle::NET);
    BOOST_REQUIRE(net.find("server_owner:alice") != std::string::npos);

    for (const std::string invalid : {"server_owner:", "server_owner:1000", "server_owner:alice:"}) {
        std::string text = net;
        text.replace(text.find("server_owner:alice"), std::string("server_owner:alice").size(), invalid);
        Defs client;
        std::string error, warning;
        BOOST_CHECK_MESSAGE(!client.restore_from_string(text, error, warning),
                            "Expected '" << invalid << "' to be rejected");
    }
}

BOOST_AUTO_TEST_CASE(test_server_owner_copy_and_assignment) {
    ECF_NAME_THIS_TEST();

    ServerState state;
    state.set_server_owner(OwnerAttr::create("alice:ops"));

    ServerState copy(state);
    BOOST_REQUIRE(copy.server_owner() != nullptr);
    BOOST_CHECK_EQUAL(copy.server_owner()->value(), "alice:ops");

    ServerState assigned;
    assigned = state;
    BOOST_REQUIRE(assigned.server_owner() != nullptr);
    BOOST_CHECK_EQUAL(assigned.server_owner()->value(), "alice:ops");

    BOOST_CHECK_THROW(assigned.set_server_owner(OwnerAttr{}), std::runtime_error);
    BOOST_CHECK_EQUAL(assigned.server_owner()->value(), "alice:ops");
}

BOOST_AUTO_TEST_CASE(test_resolve_detached_node) {
    ECF_NAME_THIS_TEST();

    // A node outside any definitions has no server to fall back on
    Task detached("t");
    BOOST_CHECK(!ownership::resolve(detached).has_value());
    detached.addOwner(OwnerAttr::create("bob"));
    auto resolved = ownership::resolve(detached);
    BOOST_REQUIRE(resolved.has_value());
    BOOST_CHECK_EQUAL(resolved->owner.value(), "bob");
}

BOOST_AUTO_TEST_CASE(test_no_server_owner_written_when_not_set) {
    ECF_NAME_THIS_TEST();

    Tree tree;
    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        BOOST_CHECK(ecf::as_string(tree.defs, style).find("server_owner") == std::string::npos);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

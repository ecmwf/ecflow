// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/stc/SSyncCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/AbstractObserver.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_SSyncCmdOwner)

namespace {

struct AspectRecorder : public AbstractObserver
{
    std::vector<ecf::Aspect::Type> start_aspects;
    std::vector<ecf::Aspect::Type> aspects;

    void update_start(const Node*, const std::vector<ecf::Aspect::Type>& a) override {
        start_aspects.insert(start_aspects.end(), a.begin(), a.end());
    }
    void update_start(const Defs*, const std::vector<ecf::Aspect::Type>&) override {}
    void update(const Node*, const std::vector<ecf::Aspect::Type>& a) override {
        aspects.insert(aspects.end(), a.begin(), a.end());
    }
    void update(const Defs*, const std::vector<ecf::Aspect::Type>&) override {}

    [[nodiscard]] bool has(ecf::Aspect::Type a) const {
        return std::find(aspects.begin(), aspects.end(), a) != aspects.end();
    }
};

defs_ptr make_defs(bool with_owner) {
    auto defs    = Defs::create();
    family_ptr f = defs->add_suite("s")->add_family("f");
    f->add_task("t");
    if (with_owner) {
        f->addOwner(OwnerAttr::create("alice"));
    }
    defs->server_state().set_state(SState::HALTED);
    return defs;
}

/// Applies a change to the server definitions, synchronises the client incrementally, and returns the aspects
/// notified on the client family /s/f; checks that the same aspects are announced before the update
std::vector<ecf::Aspect::Type>
sync_change(bool with_owner, const std::function<void(Family&)>& change, defs_ptr& server, defs_ptr& client) {
    server = make_defs(with_owner);
    client = make_defs(with_owner);
    BOOST_REQUIRE(*server == *client);

    AspectRecorder on_f;
    client->findAbsNode("/s/f")->attach(&on_f);

    ServerReply reply;
    reply.set_client_defs(client);
    unsigned int state_change_no  = Ecf::state_change_no();
    unsigned int modify_change_no = Ecf::modify_change_no();

    Ecf::set_server(true);
    {
        // As in the server commands, mark the suite as changed so that its changes are collected
        Family& f = *server->findAbsNode("/s/f")->isFamily();
        SuiteChanged1 changed(f.suite());
        change(f);
    }
    Ecf::set_server(false);
    BOOST_REQUIRE(!(*server == *client));

    MockServer mock_server(server);
    SSyncCmd cmd(0, state_change_no, modify_change_no, &mock_server);
    BOOST_CHECK(cmd.do_sync(reply));
    BOOST_CHECK(reply.in_sync());
    BOOST_CHECK_MESSAGE(!reply.full_sync(), "Expected an incremental synchronisation");

    client->findAbsNode("/s/f")->detach(&on_f);
    BOOST_CHECK_MESSAGE(*server == *client, "Server and client should be the same after the synchronisation");
    BOOST_CHECK(on_f.start_aspects == on_f.aspects);
    return on_f.aspects;
}

bool has(const std::vector<ecf::Aspect::Type>& aspects, ecf::Aspect::Type a) {
    return std::find(aspects.begin(), aspects.end(), a) != aspects.end();
}

} // namespace

BOOST_AUTO_TEST_CASE(test_owner_added_reaches_client) {
    ECF_NAME_THIS_TEST();
    ecf::test::scaffold::TestLog test_log("test_owner_added_reaches_client.log");

    defs_ptr server, client;
    auto aspects = sync_change(false, [](Family& f) { f.addOwner(OwnerAttr::create("bob:ops")); }, server, client);

    auto owner = client->findAbsNode("/s/f")->get_owner();
    BOOST_REQUIRE(owner != nullptr);
    BOOST_CHECK_EQUAL(owner->value(), "bob:ops");
    BOOST_CHECK(has(aspects, ecf::Aspect::ADD_REMOVE_ATTR));
    BOOST_CHECK(has(aspects, ecf::Aspect::OWNER));
}

BOOST_AUTO_TEST_CASE(test_owner_changed_reaches_client) {
    ECF_NAME_THIS_TEST();
    ecf::test::scaffold::TestLog test_log("test_owner_changed_reaches_client.log");

    defs_ptr server, client;
    auto aspects = sync_change(true, [](Family& f) { f.changeOwner(OwnerAttr::create("charlie")); }, server, client);

    auto owner = client->findAbsNode("/s/f")->get_owner();
    BOOST_REQUIRE(owner != nullptr);
    BOOST_CHECK_EQUAL(owner->value(), "charlie");
    BOOST_CHECK(has(aspects, ecf::Aspect::ADD_REMOVE_ATTR));
    BOOST_CHECK(has(aspects, ecf::Aspect::OWNER));
}

BOOST_AUTO_TEST_CASE(test_owner_deleted_reaches_client) {
    ECF_NAME_THIS_TEST();
    ecf::test::scaffold::TestLog test_log("test_owner_deleted_reaches_client.log");

    defs_ptr server, client;
    auto aspects = sync_change(true, [](Family& f) { f.deleteOwner(); }, server, client);

    BOOST_CHECK(client->findAbsNode("/s/f")->get_owner() == nullptr);
    BOOST_CHECK(has(aspects, ecf::Aspect::ADD_REMOVE_ATTR));
    BOOST_CHECK(has(aspects, ecf::Aspect::OWNER));
}

BOOST_AUTO_TEST_CASE(test_owner_survives_resend_of_other_attributes) {
    ECF_NAME_THIS_TEST();
    ecf::test::scaffold::TestLog test_log("test_owner_survives_resend_of_other_attributes.log");

    // Adding another attribute resends all attributes of the node, the Owner included
    defs_ptr server, client;
    auto aspects = sync_change(true, [](Family& f) { f.addEvent(Event("e")); }, server, client);

    auto owner = client->findAbsNode("/s/f")->get_owner();
    BOOST_REQUIRE(owner != nullptr);
    BOOST_CHECK_EQUAL(owner->value(), "alice");
    BOOST_CHECK(has(aspects, ecf::Aspect::ADD_REMOVE_ATTR));
    BOOST_CHECK(has(aspects, ecf::Aspect::OWNER));
}

BOOST_AUTO_TEST_CASE(test_no_owner_memento_without_owner) {
    ECF_NAME_THIS_TEST();
    ecf::test::scaffold::TestLog test_log("test_no_owner_memento_without_owner.log");

    // Adding another attribute resends all attributes of the node: none is an Owner
    defs_ptr server, client;
    auto aspects = sync_change(false, [](Family& f) { f.addEvent(Event("e")); }, server, client);

    BOOST_CHECK(client->findAbsNode("/s/f")->get_owner() == nullptr);
    BOOST_CHECK(has(aspects, ecf::Aspect::ADD_REMOVE_ATTR));
    BOOST_CHECK(!has(aspects, ecf::Aspect::OWNER));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

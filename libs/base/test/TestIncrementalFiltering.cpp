// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/NOrder.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The filtering of incremental synchronisations (R5.2, R5.5): after a change, the copy of a client that cannot read
/// every node equals a fresh full synchronisation, and the reply names no node the client cannot read.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_incremental_filtering.log"};
};

///
/// @brief Builds the definitions of the tests: suite pub without attribute, suite privsuite readable by alice only,
///        and suite m holding families seen, covertfam (readable by alice only) and seen2.
///
defs_ptr reference() {
    auto defs = Defs::create();
    defs->add_suite("pub")->add_task("t");
    auto priv = defs->add_suite("privsuite");
    priv->add_task("t")->addLabel(Label("l", "v"));
    priv->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    auto m = defs->add_suite("m");
    m->add_family("seen")->add_task("t")->addLabel(Label("l", "v"));
    auto covert = m->add_family("covertfam");
    covert->add_task("t")->addLabel(Label("l", "v"));
    covert->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    m->add_family("seen2")->add_task("t");
    // Task /m/seen/t holds alias0 and alias1, readable, and coveralias, hidden, between them
    auto task = defs->findAbsNode("/m/seen/t")->isTask();
    task->add_alias_only(); // alias0
    task->add_alias("coveralias")->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    task->add_alias_only(); // alias1
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:r,bob:r"));
    return defs;
}

std::set<std::string> paths(const Defs& defs) {
    std::set<std::string> result;
    for (const auto* node : ecf::get_all_nodes(defs)) {
        result.insert(node->absNodePath());
    }
    return result;
}

///
/// @brief Holds the copy of the definitions of a client, and the text of the last reply it received.
///
struct Client
{
    std::string user;
    unsigned int handle{0};
    defs_ptr defs;
    std::string last_reply; ///< The last reply, as sent over the wire
    bool last_full{false};  ///< Whether the last synchronisation was full
};

///
/// @brief Sends a synchronisation request for the client, and applies the reply to its copy.
///
/// The client side builds nodes, which moves the change numbers of the process; they are restored afterwards, since
/// the client and the server of these tests share the process, unlike real ones.
///
void synchronise(MockServer& server, Client& client, bool full) {
    const auto request =
        full ? std::make_shared<CSyncCmd>(client.handle)
             : std::make_shared<CSyncCmd>(
                   CSyncCmd::SYNC, client.handle, client.defs->state_change_no(), client.defs->modify_change_no());
    const auto reply = TestHelper::invokeRequestAs(&server, request, client.user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());

    ServerToClientResponse response;
    response.set_cmd(reply);
    client.last_reply.clear();
    ecf::save_as_string(client.last_reply, response);

    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    ServerToClientResponse received;
    ecf::restore_from_string(client.last_reply, received);
    ServerReply server_reply;
    if (!full) {
        server_reply.set_client_defs(client.defs);
    }
    received.get_cmd()->handle_server_response(server_reply, request, false);
    client.defs      = server_reply.client_defs();
    client.last_full = server_reply.full_sync();
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
}

Client connect(MockServer& server, const std::string& user, const std::vector<std::string>& suites = {}) {
    Client client;
    client.user = user;
    if (!suites.empty()) {
        const auto reply =
            TestHelper::invokeRequestAs(&server, std::make_shared<ClientHandleCmd>(0, suites, false), user);
        BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
        client.handle = server.defs()->client_suite_mgr().clientSuites().back().handle();
    }
    synchronise(server, client, true);
    BOOST_REQUIRE(client.defs);
    return client;
}

///
/// @brief Returns the names of the aliases of task /m/seen/t, in their order.
///
std::vector<std::string> alias_names(const Defs& defs) {
    std::vector<std::string> result;
    for (const auto& alias : defs.findAbsNode("/m/seen/t")->isTask()->aliases()) {
        result.push_back(alias->name());
    }
    return result;
}

///
/// @brief Checks that an incremental synchronisation of the client leaves it equal to a fresh full synchronisation,
///        and that its reply names none of the hidden nodes.
///
void check_incremental(MockServer& server, Client& client, const std::vector<std::string>& hidden) {
    synchronise(server, client, false);
    BOOST_CHECK_MESSAGE(!client.last_full, "the synchronisation was full");
    for (const auto& name : hidden) {
        BOOST_CHECK_MESSAGE(client.last_reply.find(name) == std::string::npos,
                            "the reply to " << client.user << " names " << name);
    }

    Client fresh        = connect(server, client.user);
    const auto received = paths(*client.defs);
    const auto expected = paths(*fresh.defs);
    BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
    std::vector<std::string> received_order;
    for (const auto& n : client.defs->findSuite("m")->children()) {
        received_order.push_back(n->name());
    }
    std::vector<std::string> expected_order;
    for (const auto& n : fresh.defs->findSuite("m")->children()) {
        expected_order.push_back(n->name());
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(
        received_order.begin(), received_order.end(), expected_order.begin(), expected_order.end());
    const auto received_aliases = alias_names(*client.defs);
    const auto expected_aliases = alias_names(*fresh.defs);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        received_aliases.begin(), received_aliases.end(), expected_aliases.begin(), expected_aliases.end());
}

const std::vector<std::string> hidden_names{"privsuite", "covertfam", "coveralias"};

///
/// @brief Changes the label of a task, recording the change on its suite as the commands of the server do.
///
void change_label(MockServer& server, const std::string& path) {
    auto node = server.defs()->findAbsNode(path);
    ecf::SuiteChanged1 changed(node->suite());
    node->changeLabel("l", "changed");
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_IncrementalFiltering, LogFixture)

///
/// @brief Verifies that a change of an attribute of a hidden node is sent only to the users who can read it.
///
BOOST_AUTO_TEST_CASE(test_attribute_change_of_hidden_node) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto alice = connect(server, "alice");
    auto bob   = connect(server, "bob");

    change_label(server, "/m/covertfam/t");
    change_label(server, "/privsuite/t");
    change_label(server, "/m/seen/t");

    check_incremental(server, bob, hidden_names);
    BOOST_CHECK_EQUAL(bob.defs->findAbsNode("/m/seen/t")->find_label("l").new_value(), "changed");

    synchronise(server, alice, false);
    BOOST_CHECK(!alice.last_full);
    BOOST_CHECK_EQUAL(alice.defs->findAbsNode("/m/covertfam/t")->find_label("l").new_value(), "changed");
}

///
/// @brief Verifies that adding a child to a readable node sends its readable children only, hidden siblings and
///        hidden descendants left out.
///
BOOST_AUTO_TEST_CASE(test_children_added_under_readable_node) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");

    {
        auto m = server.defs()->findSuite("m");
        ecf::SuiteChanged1 changed(m.get());
        auto extra = m->add_family("extra");
        extra->add_task("t");
        auto inner = extra->add_family("covertfam");
        inner->add_task("t");
        inner->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    }

    check_incremental(server, bob, hidden_names);
    BOOST_CHECK(bob.defs->findAbsNode("/m/extra/t"));
    BOOST_CHECK(!bob.defs->findAbsNode("/m/extra/covertfam"));
    BOOST_CHECK(server.defs()->findAbsNode("/m/extra/covertfam/t")); // the server keeps its nodes
    BOOST_CHECK(server.defs()->findAbsNode("/m/covertfam/t"));
}

///
/// @brief Verifies that a change of order under a readable node sends the order of the readable children only.
///
BOOST_AUTO_TEST_CASE(test_order_under_readable_node) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");

    auto m = server.defs()->findSuite("m");
    m->order(m->find_immediate_child("seen").get(), NOrder::BOTTOM);

    check_incremental(server, bob, hidden_names);
}

///
/// @brief Verifies that a change of the order of the suites sends the readable suites only.
///
BOOST_AUTO_TEST_CASE(test_order_of_suites) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");

    server.defs()->order(server.defs()->findSuite("pub").get(), NOrder::BOTTOM);

    check_incremental(server, bob, hidden_names);
    std::vector<std::string> order;
    for (const auto& s : bob.defs->suites()) {
        order.push_back(s->name());
    }
    const std::vector<std::string> expected{"m", "pub"};
    BOOST_CHECK_EQUAL_COLLECTIONS(order.begin(), order.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that the changes of a hidden alias are left out, and those of a readable alias are sent.
///
BOOST_AUTO_TEST_CASE(test_alias_attribute_changes) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");
    {
        auto task = server.defs()->findAbsNode("/m/seen/t");
        ecf::SuiteChanged1 changed(task->suite());
        task->isTask()->find_alias("coveralias")->addVariable(Variable("HIDDEN", "1"));
        task->isTask()->find_alias("alias0")->addVariable(Variable("SHOWN", "1"));
    }
    check_incremental(server, bob, hidden_names);
    BOOST_CHECK(!bob.defs->findAbsNode("/m/seen/t/alias0")->findVariable("SHOWN").empty());
}

///
/// @brief Verifies that adding an alias sends the readable aliases only, the server keeping its own.
///
BOOST_AUTO_TEST_CASE(test_alias_added) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");
    {
        auto task = server.defs()->findAbsNode("/m/seen/t");
        ecf::SuiteChanged1 changed(task->suite());
        task->isTask()->add_alias_only();
    }
    check_incremental(server, bob, hidden_names);
    BOOST_CHECK(server.defs()->findAbsNode("/m/seen/t/coveralias"));
    const std::vector<std::string> expected{"alias0", "alias1", "alias2"};
    const auto received = alias_names(*bob.defs);
    BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that a change of the order of aliases sends the order of the readable aliases only.
///
BOOST_AUTO_TEST_CASE(test_alias_order) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");
    {
        auto task = server.defs()->findAbsNode("/m/seen/t");
        ecf::SuiteChanged1 changed(task->suite());
        task->order(task->isTask()->find_alias("alias0").get(), NOrder::BOTTOM);
    }
    check_incremental(server, bob, hidden_names);
    const std::vector<std::string> expected{"alias1", "alias0"};
    const auto received = alias_names(*bob.defs);
    BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
}

///
/// @brief Verifies that a task resent with the children of its parent is copied without its hidden aliases, the
///        server keeping them.
///
BOOST_AUTO_TEST_CASE(test_copied_task_without_hidden_alias) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob");
    {
        auto seen = server.defs()->findAbsNode("/m/seen");
        ecf::SuiteChanged1 changed(seen->suite());
        seen->isFamily()->add_task("u");
    }
    check_incremental(server, bob, hidden_names);
    BOOST_CHECK(bob.defs->findAbsNode("/m/seen/t/alias0"));
    BOOST_CHECK(bob.defs->findAbsNode("/m/seen/u"));
    BOOST_CHECK(server.defs()->findAbsNode("/m/seen/t/coveralias"));
}

///
/// @brief Verifies that the changes collected for a client handle leave out the hidden nodes of its suites.
///
BOOST_AUTO_TEST_CASE(test_changes_through_handle) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    auto bob = connect(server, "bob", {"m"});

    change_label(server, "/m/covertfam/t");
    change_label(server, "/m/seen/t");

    synchronise(server, bob, false);
    BOOST_CHECK(!bob.last_full);
    BOOST_CHECK_MESSAGE(bob.last_reply.find("covertfam") == std::string::npos, "the reply names covertfam");
    BOOST_CHECK_EQUAL(bob.defs->findAbsNode("/m/seen/t")->find_label("l").new_value(), "changed");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

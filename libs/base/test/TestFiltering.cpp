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
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/stc/DefsCache.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The filtering of full replies (R5.2, R5.6): --get, --get_state and --migrate, with and without a path, and full
/// synchronisations, with and without a client handle, on a mock server.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_filtering.log"};
};

///
/// @brief Builds the definitions of the tests: suite open without attribute, suite own readable by alice only, and
///        suite m whose family hidden is readable by alice only; with the given server attribute, if any.
///
defs_ptr reference(const std::string& server = "admin:rwxlop,alice:r,bob:r,carol:r") {
    auto defs = Defs::create();
    defs->add_suite("open")->add_task("t");
    auto own = defs->add_suite("own");
    own->add_task("t");
    own->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    auto m = defs->add_suite("m");
    m->add_family("seen")->add_task("t");
    auto hidden = m->add_family("hidden");
    hidden->add_task("t");
    hidden->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    if (!server.empty()) {
        defs->server_state().set_permissions(Permissions::parse_cli(server));
    }
    return defs;
}

const std::set<std::string>
    everything{"/open", "/open/t", "/own", "/own/t", "/m", "/m/seen", "/m/seen/t", "/m/hidden", "/m/hidden/t"};
const std::set<std::string> without_alice_only{"/open", "/open/t", "/m", "/m/seen", "/m/seen/t"};

///
/// @brief Sends the reply through the wire format, and lets the client side handle it.
///
/// The client side builds nodes, which moves the change numbers of the process; they are restored afterwards, since
/// the client and the server of these tests share the process, unlike real ones.
///
void receive(const STC_Cmd_ptr& reply, const Cmd_ptr& request, ServerReply& server_reply) {
    ServerToClientResponse response;
    response.set_cmd(reply);
    std::string data;
    ecf::save_as_string(data, response);
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    ServerToClientResponse received;
    ecf::restore_from_string(data, received);
    BOOST_REQUIRE_MESSAGE(received.get_cmd()->error().empty(), received.get_cmd()->error());
    received.get_cmd()->handle_server_response(server_reply, request, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
}

std::set<std::string> paths(const Defs& defs) {
    std::set<std::string> result;
    for (const auto* node : ecf::get_all_nodes(defs)) {
        result.insert(node->absNodePath());
    }
    return result;
}

std::set<std::string> paths(const node_ptr& node) {
    std::set<std::string> result;
    for (const auto* n : ecf::get_all_nodes(*node)) {
        result.insert(n->absNodePath());
    }
    return result;
}

///
/// @brief Returns the nodes received by a user for a request answered with the full definitions.
///
std::set<std::string> received_defs(MockServer& server, const Cmd_ptr& request, const std::string& user) {
    const auto reply = TestHelper::invokeRequestAs(&server, request, user);
    ServerReply server_reply;
    receive(reply, request, server_reply);
    const auto defs = server_reply.client_defs();
    BOOST_REQUIRE(defs);
    return paths(*defs);
}

///
/// @brief Returns the nodes received by a user for a request answered with a node and its subtree.
///
std::set<std::string> received_node(MockServer& server, const Cmd_ptr& request, const std::string& user) {
    const auto reply = TestHelper::invokeRequestAs(&server, request, user);
    ServerReply server_reply;
    receive(reply, request, server_reply);
    const auto node = server_reply.client_node();
    BOOST_REQUIRE(node);
    return paths(node);
}

///
/// @brief Has the server compute, and keep, the visibility of a user, without the reply being received.
///
/// Receiving a reply builds nodes in this process, which records a change of structure and resets the visibilities
/// kept by the server; the tests of the server's own invalidation therefore prime it after the last reception.
///
void prime(MockServer& server, const std::string& user) {
    const auto reply = TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(0), user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
}

///
/// @brief Registers a client handle for the user, holding the given suites, and returns its number.
///
int register_handle(MockServer& server, const std::string& user, const std::vector<std::string>& suites) {
    const auto reply = TestHelper::invokeRequestAs(&server, std::make_shared<ClientHandleCmd>(0, suites, false), user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    return static_cast<int>(server.defs()->client_suite_mgr().clientSuites().back().handle());
}

#define CHECK_PATHS(actual, expected)                                          \
    do {                                                                       \
        const auto a = (actual);                                               \
        const auto e = (expected);                                             \
        BOOST_CHECK_EQUAL_COLLECTIONS(a.begin(), a.end(), e.begin(), e.end()); \
    } while (false)

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_Filtering, LogFixture)

///
/// @brief Verifies that --get, --get_state and --migrate without a path return only the readable nodes.
///
BOOST_AUTO_TEST_CASE(test_get_without_path) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    for (const auto api : {CtsNodeCmd::GET, CtsNodeCmd::GET_STATE, CtsNodeCmd::MIGRATE}) {
        BOOST_TEST_CONTEXT("api " << api) {
            CHECK_PATHS(received_defs(server, std::make_shared<CtsNodeCmd>(api), "alice"), everything);
            CHECK_PATHS(received_defs(server, std::make_shared<CtsNodeCmd>(api), "bob"), without_alice_only);
            CHECK_PATHS(received_defs(server, std::make_shared<CtsNodeCmd>(api), "alice"), everything);
        }
    }
}

///
/// @brief Verifies that --get, --get_state and --migrate of a readable node omit its unreadable descendants.
///
BOOST_AUTO_TEST_CASE(test_get_with_path) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    for (const auto api : {CtsNodeCmd::GET, CtsNodeCmd::GET_STATE, CtsNodeCmd::MIGRATE}) {
        BOOST_TEST_CONTEXT("api " << api) {
            CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(api, "/m"), "bob"),
                        (std::set<std::string>{"/m", "/m/seen", "/m/seen/t"}));
            CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(api, "/m"), "alice"),
                        (std::set<std::string>{"/m", "/m/seen", "/m/seen/t", "/m/hidden", "/m/hidden/t"}));
            // A node received alone has no parent: its paths start from it
            CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(api, "/m/seen"), "bob"),
                        (std::set<std::string>{"/seen", "/seen/t"}));
        }
    }
}

///
/// @brief Verifies that a full synchronisation without a handle returns only the readable nodes, whatever the order
///        of the requesters, and that requesters with the same readable nodes share one cache entry (R5.6).
///
BOOST_AUTO_TEST_CASE(test_full_sync_without_handle) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    const auto sync = [] { return std::make_shared<CSyncCmd>(0); };
    for (int round = 0; round < 2; ++round) {
        CHECK_PATHS(received_defs(server, sync(), "alice"), everything);
        CHECK_PATHS(received_defs(server, sync(), "bob"), without_alice_only);
        CHECK_PATHS(received_defs(server, sync(), "carol"), without_alice_only);
        CHECK_PATHS(received_defs(server, sync(), "alice"), everything);
    }
    BOOST_CHECK_EQUAL(DefsCache::entries(), 2U);

    // After a change of state, every entry is rebuilt: the copies received hold the change
    server.defs()->findAbsNode("/m/seen/t")->suspend();
    for (const auto* user : {"bob", "alice"}) {
        const auto reply = TestHelper::invokeRequestAs(&server, sync(), user);
        ServerReply server_reply;
        receive(reply, sync(), server_reply);
        BOOST_REQUIRE(server_reply.client_defs());
        BOOST_CHECK_MESSAGE(server_reply.client_defs()->findAbsNode("/m/seen/t")->isSuspended(),
                            "the copy of " << user << " misses the change");
    }
}

///
/// @brief Verifies that a node added below a readable node, without a modify change, is filtered as soon as it is
///        added: a hidden one is never sent, a readable one is.
///
BOOST_AUTO_TEST_CASE(test_full_sync_follows_added_nodes) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    const auto sync = [] { return std::make_shared<CSyncCmd>(0); };
    CHECK_PATHS(received_defs(server, sync(), "bob"), without_alice_only);
    prime(server, "bob");

    const auto modify = Ecf::modify_change_no();
    auto m            = server.defs()->findSuite("m");
    auto later        = m->add_family("later");
    later->add_task("t");
    later->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    m->add_family("open_later");
    BOOST_REQUIRE_EQUAL(Ecf::modify_change_no(), modify); // a change of state only

    auto expected = without_alice_only;
    expected.insert("/m/open_later");
    CHECK_PATHS(received_defs(server, sync(), "bob"), expected);
    CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/m"), "bob"),
                (std::set<std::string>{"/m", "/m/seen", "/m/seen/t", "/m/open_later"}));
}

///
/// @brief Verifies that a hidden node moved under a readable node (as a local --plug does, a change of state only)
///        stays hidden from the requesters who read the definitions before the move.
///
BOOST_AUTO_TEST_CASE(test_full_sync_after_hidden_node_moved) {
    ECF_NAME_THIS_TEST();

    auto defs  = reference();
    auto inner = defs->findAbsNode("/m/hidden")->isFamily()->add_family("inner");
    inner->add_task("t");
    inner->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    MockServer server(defs);
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), without_alice_only);
    prime(server, "bob");

    // Move /m/hidden/inner under /m/seen, as a local plug does
    const auto modify = Ecf::modify_change_no();
    auto moved        = defs->findAbsNode("/m/hidden")->removeChild(inner.get());
    BOOST_REQUIRE(defs->findAbsNode("/m/seen")->addChild(moved));
    BOOST_REQUIRE_EQUAL(Ecf::modify_change_no(), modify);

    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), without_alice_only);
    CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/m"), "bob"),
                (std::set<std::string>{"/m", "/m/seen", "/m/seen/t"}));
    BOOST_CHECK(received_defs(server, std::make_shared<CSyncCmd>(0), "alice").count("/m/seen/inner/t") == 1);
}

///
/// @brief Verifies that a Permissions attribute added to a readable node hides it from the requesters who read the
///        definitions before.
///
BOOST_AUTO_TEST_CASE(test_full_sync_after_attribute_added) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), without_alice_only);
    prime(server, "bob");

    server.defs()->findAbsNode("/m/seen")->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"),
                (std::set<std::string>{"/open", "/open/t", "/m"}));
}

///
/// @brief Verifies that new definitions loaded in the server are filtered for the requesters who read the previous
///        ones.
///
BOOST_AUTO_TEST_CASE(test_full_sync_after_definitions_loaded) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), without_alice_only);

    auto replacement = Defs::create();
    auto m           = replacement->add_suite("m");
    m->add_family("seen")->add_task("t");
    auto hidden = m->add_family("hidden");
    hidden->add_task("t");
    hidden->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    auto newer = m->add_family("newer");
    newer->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    const auto load = TestHelper::invokeRequestAs(&server, std::make_shared<LoadDefsCmd>(replacement, true), "admin");
    BOOST_REQUIRE_MESSAGE(load->error().empty(), load->error());

    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), without_alice_only);
}

///
/// @brief Verifies that the aliases of a readable task are sent only when readable, and that a task whose aliases
///        are all hidden is sent well formed, for full definitions and for the task requested alone.
///
BOOST_AUTO_TEST_CASE(test_aliases) {
    ECF_NAME_THIS_TEST();

    auto defs = reference();
    auto task = defs->findAbsNode("/open/t")->isTask();
    task->add_alias_only();                                                                  // alias0, readable
    task->add_alias_only()->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r")); // alias1, hidden
    auto only_hidden = defs->findSuite("open")->add_task("u");
    only_hidden->add_alias_only()->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    MockServer server(defs);

    auto expected = without_alice_only;
    expected.insert({"/open/t/alias0", "/open/u"});
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), expected);
    CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/open/t"), "bob"),
                (std::set<std::string>{"/t", "/t/alias0"}));
    CHECK_PATHS(received_node(server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/open/u"), "bob"),
                (std::set<std::string>{"/u"}));
    BOOST_CHECK(received_defs(server, std::make_shared<CSyncCmd>(0), "alice").count("/open/u/alias0") == 1);
}

///
/// @brief Verifies that a full synchronisation through a handle returns only the readable nodes, for handles holding
///        some suites and for a handle holding all suites.
///
BOOST_AUTO_TEST_CASE(test_full_sync_with_handle) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());

    const auto subset = register_handle(server, "bob", {"m"});
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(subset), "bob"),
                (std::set<std::string>{"/m", "/m/seen", "/m/seen/t"}));

    // A handle cannot hold a suite its user cannot read (see T_HandleFiltering)
    const auto readable = register_handle(server, "bob", {"open", "m"});
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(readable), "bob"), without_alice_only);

    const auto alice = register_handle(server, "alice", {"open", "own", "m"});
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(alice), "alice"), everything);
}

///
/// @brief Verifies that, without a server attribute, every user receives every node from one cache entry (R5.4).
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server_is_unfiltered) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference(""));
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "bob"), everything);
    CHECK_PATHS(received_defs(server, std::make_shared<CSyncCmd>(0), "carol"), everything);
    CHECK_PATHS(received_defs(server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET), "bob"), everything);
    BOOST_CHECK_EQUAL(DefsCache::entries(), 1U);
}

///
/// @brief Verifies that, without a server attribute, every user shares one visibility, so that the server keeps one
///        whatever the number of users; with Permissions, each user has its own.
///
BOOST_AUTO_TEST_CASE(test_visibility_shared_without_permissions) {
    ECF_NAME_THIS_TEST();

    const auto visibility_of = [](MockServer& server, const std::string& user) {
        const auto identity = ecf::Identity::make_user(user, "");
        return ecf::RequestContext(identity, server).visibility();
    };
    MockServer unrestricted(reference(""));
    BOOST_CHECK(visibility_of(unrestricted, "bob") == visibility_of(unrestricted, "carol"));
    MockServer restricted(reference());
    BOOST_CHECK(visibility_of(restricted, "bob") != visibility_of(restricted, "carol"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

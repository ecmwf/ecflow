// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/ZombieCtrl.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/QueryCmd.hpp"
#include "ecflow/base/stc/SStatsCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The filtering of listings and aggregates (R5.2): --suites, --zombie_get, --stats and the aggregated state of the
/// root, in full and incremental synchronisations.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_listing_filtering.log"};
};

///
/// @brief Builds the definitions of the tests: suite pub without attribute, and suite privsuite readable by alice
///        only; both hold an active task t.
///
defs_ptr reference() {
    auto defs = Defs::create();
    defs->add_suite("pub")->add_task("t");
    auto priv = defs->add_suite("privsuite");
    priv->add_task("t");
    priv->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:r"));
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlops,alice:r,bob:r"));
    return defs;
}

///
/// @brief Handles a request as a user, and returns the reply as sent over the wire, and as received by the client.
///
/// The client side builds nodes, which moves the change numbers of the process; they are restored afterwards, since
/// the client and the server of these tests share the process, unlike real ones.
///
STC_Cmd_ptr request(MockServer& server, const Cmd_ptr& command, const std::string& user, std::string& wire) {
    const auto reply = TestHelper::invokeRequestAs(&server, command, user);
    ServerToClientResponse response;
    response.set_cmd(reply);
    wire.clear();
    ecf::save_as_string(wire, response);
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    ServerToClientResponse received;
    ecf::restore_from_string(wire, received);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    BOOST_REQUIRE_MESSAGE(received.get_cmd()->error().empty(), received.get_cmd()->error());
    return received.get_cmd();
}

///
/// @brief Sets the state of a task, recording the change on its suite as the server does, and aggregates the states.
///
void set_state(MockServer& server, const std::string& path, NState::State state) {
    auto node = server.defs()->findAbsNode(path);
    {
        ecf::SuiteChanged1 changed(node->suite());
        node->set_state(state);
    }
    server.defs()->set_most_significant_state();
}

///
/// @brief Returns the state of the root, as received by a full synchronisation of the user.
///
NState::State root_state(MockServer& server, const std::string& user) {
    std::string wire;
    const auto command = std::make_shared<CSyncCmd>(0);
    const auto reply   = request(server, command, user, wire);
    ServerReply server_reply;
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    reply->handle_server_response(server_reply, command, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    return server_reply.client_defs()->state();
}

///
/// @brief Synchronises the copy of a client through a handle (0 for none): fully when there is no copy, otherwise
///        incrementally, and returns whether the synchronisation was full.
///
bool synchronise(MockServer& server, const std::string& user, unsigned int handle, defs_ptr& copy) {
    const auto command =
        copy ? std::make_shared<CSyncCmd>(CSyncCmd::SYNC, handle, copy->state_change_no(), copy->modify_change_no())
             : std::make_shared<CSyncCmd>(handle);
    std::string wire;
    const auto reply = request(server, command, user, wire);
    ServerReply server_reply;
    if (copy) {
        server_reply.set_client_defs(copy);
    }
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    reply->handle_server_response(server_reply, command, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    copy = server_reply.client_defs();
    return server_reply.full_sync();
}

///
/// @brief Returns whether the news of the copy of a client, through a handle (0 for none), reports a change.
///
bool news(MockServer& server, const std::string& user, unsigned int handle, const defs_ptr& copy) {
    const auto command =
        std::make_shared<CSyncCmd>(CSyncCmd::NEWS, handle, copy->state_change_no(), copy->modify_change_no());
    std::string wire;
    const auto reply = request(server, command, user, wire);
    ServerReply server_reply;
    reply->handle_server_response(server_reply, command, false);
    return server_reply.get_news() != ServerReply::NO_NEWS;
}

///
/// @brief Registers a handle holding the given suites for a user, and returns its number.
///
unsigned int register_handle(MockServer& server, const std::string& user, const std::vector<std::string>& suites) {
    std::string wire;
    const auto reply = request(server, std::make_shared<ClientHandleCmd>(0, suites, false), user, wire);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    return server.defs()->client_suite_mgr().clientSuites().back().handle();
}

///
/// @brief Returns the paths of the zombies listed to a user.
///
std::vector<std::string> zombies_of(MockServer& server, const std::string& user) {
    std::string wire;
    const auto command = std::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES);
    const auto reply   = request(server, command, user, wire);
    ServerReply server_reply;
    reply->handle_server_response(server_reply, command, false);
    std::vector<std::string> result;
    for (const auto& zombie : server_reply.zombies()) {
        result.push_back(zombie.path_to_task());
    }
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_ListingFiltering, LogFixture)

///
/// @brief Verifies that --suites lists the readable suites only.
///
BOOST_AUTO_TEST_CASE(test_suites) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    std::string wire;
    request(server, std::make_shared<CtsCmd>(CtsCmd::SUITES), "bob", wire);
    BOOST_CHECK_MESSAGE(wire.find("privsuite") == std::string::npos, "--suites names privsuite to bob");
    BOOST_CHECK_MESSAGE(wire.find("pub") != std::string::npos, "--suites misses pub");
    request(server, std::make_shared<CtsCmd>(CtsCmd::SUITES), "alice", wire);
    BOOST_CHECK_MESSAGE(wire.find("privsuite") != std::string::npos, "--suites misses privsuite for alice");
}

///
/// @brief Verifies that --query state / returns the state of the root aggregated from the readable suites.
///
BOOST_AUTO_TEST_CASE(test_query_root_state) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    set_state(server, "/pub/t", NState::COMPLETE);
    set_state(server, "/privsuite/t", NState::ABORTED);
    std::string wire;
    for (const auto& [user, state] :
         std::vector<std::pair<std::string, std::string>>{{"bob", "complete"}, {"alice", "aborted"}}) {
        const auto reply = request(server, std::make_shared<QueryCmd>("state", "/", "", ""), user, wire);
        BOOST_CHECK_EQUAL(reply->get_string(), state);
    }
}

///
/// @brief Verifies that --zombie_get lists the zombies of readable tasks only.
///
BOOST_AUTO_TEST_CASE(test_zombies) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    for (const auto* path : {"/pub/t", "/privsuite/t"}) {
        auto task = server.defs()->findAbsNode(path)->isTask();
        task->set_state(NState::ACTIVE);
        server.zombie_ctrl().add_user_zombies(task, "test");
    }
    std::string wire;
    request(server, std::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES), "bob", wire);
    BOOST_CHECK_MESSAGE(wire.find("privsuite") == std::string::npos, "--zombie_get names privsuite to bob");
    BOOST_CHECK_MESSAGE(wire.find("/pub/t") != std::string::npos, "--zombie_get misses /pub/t");
    request(server, std::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES), "alice", wire);
    BOOST_CHECK_MESSAGE(wire.find("/privsuite/t") != std::string::npos, "--zombie_get misses /privsuite/t");
}

///
/// @brief Verifies that --stats counts the readable suites only.
///
BOOST_AUTO_TEST_CASE(test_stats) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    std::string wire;
    for (const auto& [user, suites] : std::vector<std::pair<std::string, int>>{{"bob", 1}, {"alice", 2}}) {
        BOOST_TEST_CONTEXT(user) {
            // The statistics, as returned to the REST API (--stats_server)
            const auto reply  = request(server, std::make_shared<CtsCmd>(CtsCmd::STATS_SERVER), user, wire);
            const auto* stats = dynamic_cast<const SStatsCmd*>(reply.get());
            BOOST_REQUIRE(stats);
            BOOST_CHECK_EQUAL(stats->stats().no_of_suites_, suites);
            // The report (--stats)
            const auto report = request(server, std::make_shared<CtsCmd>(CtsCmd::STATS), user, wire);
            const auto& text  = report->get_string();
            const auto at     = text.find("Number of Suites");
            BOOST_REQUIRE_MESSAGE(at != std::string::npos, text);
            const auto line = text.substr(at, text.find('\n', at) - at);
            BOOST_CHECK_MESSAGE(line.find(std::to_string(suites)) != std::string::npos, line);
        }
    }
}

///
/// @brief Verifies that the state of the root is aggregated from the readable suites, in full and incremental
///        synchronisations.
///
BOOST_AUTO_TEST_CASE(test_root_state) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    set_state(server, "/pub/t", NState::COMPLETE);
    set_state(server, "/privsuite/t", NState::ABORTED);
    BOOST_REQUIRE_EQUAL(server.defs()->state(), NState::ABORTED);

    BOOST_CHECK_EQUAL(root_state(server, "bob"), NState::COMPLETE);
    BOOST_CHECK_EQUAL(root_state(server, "alice"), NState::ABORTED);

    // Incremental: a readable suite changes while the aggregate of every suite does not
    std::string wire;
    const auto full = std::make_shared<CSyncCmd>(0);
    auto reply      = request(server, full, "bob", wire);
    ServerReply server_reply;
    {
        const auto state_change_no  = Ecf::state_change_no();
        const auto modify_change_no = Ecf::modify_change_no();
        reply->handle_server_response(server_reply, full, false);
        Ecf::set_state_change_no(state_change_no);
        Ecf::set_modify_change_no(modify_change_no);
    }
    auto copy = server_reply.client_defs();

    set_state(server, "/pub/t", NState::ACTIVE);
    BOOST_REQUIRE_EQUAL(server.defs()->state(), NState::ABORTED);
    const auto sync = std::make_shared<CSyncCmd>(CSyncCmd::SYNC, 0, copy->state_change_no(), copy->modify_change_no());
    reply           = request(server, sync, "bob", wire);
    ServerReply incremental;
    incremental.set_client_defs(copy);
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    reply->handle_server_response(incremental, sync, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    BOOST_CHECK(!incremental.full_sync());
    BOOST_CHECK_EQUAL(incremental.client_defs()->state(), NState::ACTIVE);
}

///
/// @brief Verifies that, through handles holding some, all or none of the readable suites, the state of the root
///        received incrementally equals the one of a fresh full synchronisation, after changes inside and outside
///        the suites of the handle; that the news reports every change of that state; and that it reports nothing
///        once synchronised.
///
BOOST_AUTO_TEST_CASE(test_root_state_through_handles) {
    ECF_NAME_THIS_TEST();

    auto defs = reference();
    defs->add_suite("pub2")->add_task("t");
    MockServer server(defs);
    set_state(server, "/pub/t", NState::COMPLETE);
    set_state(server, "/pub2/t", NState::COMPLETE);
    set_state(server, "/privsuite/t", NState::ABORTED);

    const std::vector<std::vector<std::string>> selections{{"pub"}, {"pub", "pub2"}, {"future"}};
    std::vector<std::pair<unsigned int, defs_ptr>> clients;
    for (const auto& suites : selections) {
        defs_ptr copy;
        const auto handle = register_handle(server, "bob", suites);
        synchronise(server, "bob", handle, copy);
        // the readable suites of the handle: complete, or none (unknown)
        BOOST_CHECK_EQUAL(copy->state(), suites.front() == "future" ? NState::UNKNOWN : NState::COMPLETE);
        clients.emplace_back(handle, copy);
    }

    for (const auto& [path, state] : std::vector<std::pair<std::string, NState::State>>{
             {"/pub/t", NState::ACTIVE}, {"/pub2/t", NState::ABORTED}, {"/privsuite/t", NState::COMPLETE}}) {
        set_state(server, path, state);
        for (auto& [handle, copy] : clients) {
            BOOST_TEST_CONTEXT("change of " << path << ", handle " << handle) {
                defs_ptr fresh;
                synchronise(server, "bob", handle, fresh);
                if (fresh->state() != copy->state()) {
                    BOOST_CHECK_MESSAGE(news(server, "bob", handle, copy), "the news misses a change of the root");
                }
                BOOST_CHECK(!synchronise(server, "bob", handle, copy));
                BOOST_CHECK_EQUAL(copy->state(), fresh->state());
                BOOST_CHECK_MESSAGE(!news(server, "bob", handle, copy), "news reported once synchronised");
            }
        }
    }
}

///
/// @brief Verifies that the zombie of a task that no longer exists is listed only when its closest existing ancestor
///        below the server is readable.
///
BOOST_AUTO_TEST_CASE(test_zombies_of_deleted_tasks) {
    ECF_NAME_THIS_TEST();

    auto defs = reference();
    defs->findSuite("pub")->add_family("f")->add_family("g")->add_task("u");
    defs->findSuite("privsuite")->add_family("f")->add_task("u");
    defs->add_suite("gone")->add_task("t");
    MockServer server(defs);
    for (const auto* path : {"/pub/t", "/pub/f/g/u", "/privsuite/t", "/privsuite/f/u", "/gone/t"}) {
        auto task = server.defs()->findAbsNode(path)->isTask();
        task->set_state(NState::ACTIVE);
        server.zombie_ctrl().add_user_zombies(task, "test");
    }

    // Tasks removed: below a readable family (two levels), below a hidden suite, and a whole suite
    server.defs()->findAbsNode("/pub/f")->isFamily()->removeChild(server.defs()->findAbsNode("/pub/f/g").get());
    server.defs()
        ->findAbsNode("/privsuite/f")
        ->isFamily()
        ->removeChild(server.defs()->findAbsNode("/privsuite/f/u").get());
    BOOST_REQUIRE(server.defs()->deleteChild(server.defs()->findSuite("gone").get()));

    BOOST_CHECK(zombies_of(server, "bob") == (std::vector<std::string>{"/pub/f/g/u", "/pub/t"}));
    BOOST_CHECK(zombies_of(server, "alice") ==
                (std::vector<std::string>{"/privsuite/f/u", "/privsuite/t", "/pub/f/g/u", "/pub/t"}));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

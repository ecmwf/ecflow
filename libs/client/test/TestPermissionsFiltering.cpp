// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ReadableSet.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/NodeState.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The filtering of the replies carrying definitions or nodes (R5.2, R5.6), against a live server, over TCP and HTTP.
///

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};
const Credentials bob{"bob", "bob-secret"};
const Credentials carol{"carol", "carol-secret"};

const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"},)"
    R"( {"user": "bob", "permissions": "r"}, {"user": "carol", "permissions": "r"}])";

const std::set<std::string>
    everything{"/open", "/open/t", "/own", "/own/t", "/m", "/m/seen", "/m/seen/t", "/m/hidden", "/m/hidden/t"};
const std::set<std::string> without_alice_only{"/open", "/open/t", "/m", "/m/seen", "/m/seen/t"};

///
/// @brief Loads, from a definition file, suite open without attribute, suite own readable by alice only, and suite m
///        whose family hidden is readable by alice only.
///
void load_suites(const PermissionsServer& server) {
    // Every task has a script, so that the requests reading scripts succeed where allowed
    const auto home = server.home().string();
    for (const auto* task : {"open/t", "own/t", "m/seen/t", "m/hidden/t"}) {
        const auto script = server.home() / (std::string(task) + ".ecf");
        fs::create_directories(script.parent_path());
        std::ofstream(script) << "echo hello\n";
    }
    const auto file = server.directory() / "filtering.def";
    std::ofstream(file)
        << "suite open\n  edit ECF_HOME '" << home << "'\n  task t\nendsuite\n"
        << "suite own\n"
        << R"(  permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
        << "\n  edit ECF_HOME '" << home << "'\n  task t\nendsuite\n"
        << "suite m\n  edit ECF_HOME '" << home << "'\n  family seen\n    task t\n  endfamily\n  family hidden\n"
        << R"(    permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
        << "\n    task t\n  endfamily\nendsuite\n";
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->loadDefs(file.string()) == 0, client->errorMsg());
}

std::string settings_of(bool http) {
    return http ? R"("http": true)" : "";
}

#define CHECK_PATHS(actual, expected)                                          \
    do {                                                                       \
        const auto a = (actual);                                               \
        const auto e = (expected);                                             \
        BOOST_CHECK_EQUAL_COLLECTIONS(a.begin(), a.end(), e.begin(), e.end()); \
    } while (false)

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsFiltering)

///
/// @brief Verifies that two identities with different readable nodes each receive exactly their own through every
///        channel carrying full definitions, whatever the order of their requests.
///
BOOST_AUTO_TEST_CASE(test_full_replies_hold_the_readable_nodes) {
    ECF_NAME_THIS_TEST();

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(admin,
                                     {alice, bob, carol},
                                     server_attribute,
                                     std::chrono::seconds(30),
                                     std::nullopt,
                                     std::nullopt,
                                     false,
                                     settings_of(http));
            load_suites(server);

            const std::vector<std::pair<Credentials, std::set<std::string>>> expected{
                {alice, everything}, {bob, without_alice_only}, {carol, without_alice_only}, {alice, everything}};
            for (int round = 0; round < 2; ++round) {
                for (const auto& entry : expected) {
                    const auto& user  = entry.first;
                    const auto& nodes = entry.second;
                    const auto suites = user.user == "alice" ? std::vector<std::string>{"open", "own", "m"}
                                                             : std::vector<std::string>{"open", "m"};
                    for (const auto& channel : ecf::test::full_channels(suites)) {
                        BOOST_TEST_CONTEXT(channel.name << " by " << user.user << ", round " << round) {
                            CHECK_PATHS(channel.receive([&] { return server.client(user); }), nodes);
                        }
                    }
                }
                // A change of state between the rounds, so that the second round rebuilds the cache entries
                auto client = server.administrator();
                BOOST_REQUIRE_MESSAGE(client->suspend("/m/seen/t") == 0, client->errorMsg());
            }
        }
    }
}

///
/// @brief Verifies that, after a change, the cache entry rebuilt by a full synchronisation alone (no --get before it)
///        carries the change to each identity.
///
BOOST_AUTO_TEST_CASE(test_full_synchronisation_carries_changes) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load_suites(server);
    for (const auto& user : {alice, bob}) {
        auto client = server.client(user);
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
    }

    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->suspend("/m/seen/t") == 0, administrator->errorMsg());

    for (const auto& user : {bob, alice}) {
        BOOST_TEST_CONTEXT(user.user) {
            auto client = server.client(user);
            BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
            BOOST_REQUIRE(client->defs()->findAbsNode("/m/seen/t"));
            BOOST_CHECK(client->defs()->findAbsNode("/m/seen/t")->isSuspended());
        }
    }
}

///
/// @brief Verifies that, after changes of attributes, of order and of children under readable nodes holding hidden
///        ones, an incremental synchronisation leaves the copy of each identity equal to a fresh full one (R5.5).
///
BOOST_AUTO_TEST_CASE(test_incremental_synchronisation_holds_the_readable_nodes) {
    ECF_NAME_THIS_TEST();

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(admin,
                                     {alice, bob},
                                     server_attribute,
                                     std::chrono::seconds(30),
                                     std::nullopt,
                                     std::nullopt,
                                     false,
                                     settings_of(http));
            load_suites(server);
            {
                const auto file = server.directory() / "src.def";
                std::ofstream(file)
                    << "suite src\n  family extra\n    task t\n    family inner\n"
                    << R"(      permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
                    << "\n      task t\n    endfamily\n  endfamily\nendsuite\n";
                auto administrator = server.administrator();
                BOOST_REQUIRE_MESSAGE(administrator->loadDefs(file.string()) == 0, administrator->errorMsg());
            }
            std::vector<std::pair<Credentials, std::unique_ptr<ClientInvoker>>> clients;
            for (const auto& user : {alice, bob}) {
                clients.emplace_back(user, server.client(user));
                BOOST_REQUIRE_MESSAGE(clients.back().second->sync_local() == 0, clients.back().second->errorMsg());
            }

            auto administrator = server.administrator();
            const auto request = [&](const std::vector<std::string>& args) {
                BOOST_REQUIRE_MESSAGE(administrator->invoke(args) == 0, administrator->errorMsg());
            };
            request({"--alter", "add", "variable", "H", "1", "/m/hidden/t"});
            request({"--alter", "add", "variable", "V", "1", "/m/seen/t"});
            request({"--order=/m/seen", "bottom"});
            request({"--order=/open", "bottom"}); // the order of the suites
            request({"--suspend=/own/t"});
            // A family moved by a local plug below a readable suite, holding a hidden family: a change of state,
            // sent as the children of /m, the hidden family left out
            BOOST_REQUIRE_MESSAGE(administrator->plug("/src/extra", "/m") == 0, administrator->errorMsg());

            for (auto& [user, client] : clients) {
                BOOST_TEST_CONTEXT(user.user) {
                    BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
                    BOOST_CHECK(!client->server_reply().full_sync());
                    auto fresh = server.client(user);
                    BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
                    CHECK_PATHS(ecf::test::node_paths(*client->defs()), ecf::test::node_paths(*fresh->defs()));
                    std::vector<std::string> received;
                    std::vector<std::string> expected;
                    for (const auto& s : client->defs()->suites()) {
                        received.push_back(s->name());
                    }
                    for (const auto& s : fresh->defs()->suites()) {
                        expected.push_back(s->name());
                    }
                    BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
                    BOOST_CHECK(client->defs()->findAbsNode("/m/seen/t")->findVariable("V").value() == "1");
                    std::vector<std::string> received_children;
                    std::vector<std::string> expected_children;
                    for (const auto& n : client->defs()->findSuite("m")->children()) {
                        received_children.push_back(n->name());
                    }
                    for (const auto& n : fresh->defs()->findSuite("m")->children()) {
                        expected_children.push_back(n->name());
                    }
                    BOOST_CHECK_EQUAL_COLLECTIONS(received_children.begin(),
                                                  received_children.end(),
                                                  expected_children.begin(),
                                                  expected_children.end());
                    BOOST_CHECK(client->defs()->findAbsNode("/m/extra/t"));
                    BOOST_CHECK_EQUAL(static_cast<bool>(client->defs()->findAbsNode("/m/extra/inner")),
                                      user.user == "alice");
                }
            }
        }
    }
}

///
/// @brief Returns the names of the suites held by the copy of a client.
///
std::set<std::string> suites_of(const ClientInvoker& client) {
    std::set<std::string> result;
    for (const auto& s : client.defs()->suites()) {
        result.insert(s->name());
    }
    return result;
}

///
/// @brief Verifies the client handles of two users: a suite the requester cannot read cannot be registered; a
///        registered suite that loses read through --replace is no longer sent, and is sent again once readable;
///        suites added automatically are sent only when readable.
///
BOOST_AUTO_TEST_CASE(test_client_handles) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load_suites(server);

    auto for_bob = server.client(bob);
    BOOST_CHECK(for_bob->ch_register(false, {"open", "own"}) != 0);
    BOOST_CHECK(for_bob->server_reply().permission_denied());

    BOOST_REQUIRE_MESSAGE(for_bob->ch_register(true, {"open", "m"}) == 0, for_bob->errorMsg());
    BOOST_REQUIRE_MESSAGE(for_bob->sync_local() == 0, for_bob->errorMsg());
    auto for_alice = server.client(alice);
    BOOST_REQUIRE_MESSAGE(for_alice->ch_register(false, {"open", "own", "m"}) == 0, for_alice->errorMsg());
    BOOST_REQUIRE_MESSAGE(for_alice->sync_local() == 0, for_alice->errorMsg());
    BOOST_CHECK(suites_of(*for_bob) == (std::set<std::string>{"open", "m"}));
    BOOST_CHECK(suites_of(*for_alice) == (std::set<std::string>{"open", "own", "m"}));

    auto administrator   = server.administrator();
    const auto replace_m = [&](bool hidden) {
        const auto file = server.directory() / "m.def";
        std::ofstream(file)
            << "suite m\n"
            << (hidden
                    ? R"(  permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
                      "\n"
                    : "")
            << "  task t\nendsuite\n";
        BOOST_REQUIRE_MESSAGE(administrator->replace("/m", file.string(), false, true) == 0, administrator->errorMsg());
    };

    replace_m(true); // bob loses read on m
    BOOST_REQUIRE_MESSAGE(for_bob->sync_local() == 0, for_bob->errorMsg());
    BOOST_CHECK(suites_of(*for_bob) == (std::set<std::string>{"open"}));
    BOOST_REQUIRE_MESSAGE(for_alice->sync_local() == 0, for_alice->errorMsg());
    BOOST_CHECK(suites_of(*for_alice) == (std::set<std::string>{"open", "own", "m"}));

    replace_m(false); // bob regains read on m
    BOOST_REQUIRE_MESSAGE(for_bob->sync_local() == 0, for_bob->errorMsg());
    BOOST_CHECK(suites_of(*for_bob) == (std::set<std::string>{"open", "m"}));

    // Suites added automatically to bob's handle
    const auto file = server.directory() / "more.def";
    std::ofstream(file)
        << "suite newpub\n  task t\nendsuite\nsuite newpriv\n"
        << R"(  permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
        << "\n  task t\nendsuite\n";
    BOOST_REQUIRE_MESSAGE(administrator->loadDefs(file.string()) == 0, administrator->errorMsg());
    BOOST_REQUIRE_MESSAGE(for_bob->sync_local() == 0, for_bob->errorMsg());
    BOOST_CHECK(suites_of(*for_bob) == (std::set<std::string>{"open", "m", "newpub"}));
}

///
/// @brief Verifies, channel by channel, what two identities receive (R5.2, R7.6): the full replies and the listings
///        hold or count exactly the readable suites and nodes, and the requests reading an unreadable node are
///        denied, over TCP and HTTP.
///
/// The incremental synchronisation, client handles, zombies and the aggregated state of the root are verified by
/// the tests above and by u_base (T_IncrementalFiltering, T_HandleFiltering, T_ListingFiltering).
///
BOOST_AUTO_TEST_CASE(test_channel_matrix) {
    ECF_NAME_THIS_TEST();

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(admin,
                                     {alice, bob},
                                     server_attribute,
                                     std::chrono::seconds(30),
                                     std::nullopt,
                                     std::nullopt,
                                     false,
                                     settings_of(http));
            load_suites(server);
            {
                // Zombies: tasks made active, then completed by hand
                auto administrator = server.administrator();
                for (const auto* task : {"/open/t", "/own/t", "/m/hidden/t"}) {
                    BOOST_REQUIRE_MESSAGE(administrator->force(task, "active") == 0, administrator->errorMsg());
                    BOOST_REQUIRE_MESSAGE(administrator->force(task, "complete") == 0, administrator->errorMsg());
                }
            }

            const std::vector<std::tuple<Credentials, std::set<std::string>, std::set<std::string>>> identities{
                {alice, everything, {"/open", "/own", "/m", "/#3"}}, {bob, without_alice_only, {"/open", "/m", "/#2"}}};
            for (const auto& [user, nodes, suites] : identities) {
                const auto make = [&, u = user] { return server.client(u); };
                for (const auto& channel : ecf::test::full_channels({"open", "m"})) {
                    BOOST_TEST_CONTEXT(channel.name << " by " << user.user) {
                        auto expected = nodes;
                        if (channel.name == "handle") {
                            expected.erase("/own");
                            expected.erase("/own/t");
                        }
                        CHECK_PATHS(channel.receive(make), expected);
                    }
                }
                for (const auto& channel : ecf::test::listing_channels()) {
                    BOOST_TEST_CONTEXT(channel.name << " by " << user.user) {
                        auto received = channel.receive(make);
                        std::set<std::string> expected;
                        for (const auto& s : suites) {
                            if ((channel.name == "--stats") == (s.rfind("/#", 0) == 0)) {
                                expected.insert(s);
                            }
                        }
                        CHECK_PATHS(received, expected);
                    }
                }
                for (const auto& request : ecf::test::node_reading_requests()) {
                    for (const auto* path : {"/own/t", "/m/hidden/t"}) {
                        BOOST_TEST_CONTEXT(request.name << " " << path << " by " << user.user) {
                            auto client       = make();
                            const auto status = request.send(*client, path);
                            if (user.user == "bob") {
                                BOOST_CHECK(status != 0);
                                BOOST_CHECK(client->server_reply().permission_denied());
                            }
                            else {
                                BOOST_REQUIRE_MESSAGE(status == 0, client->errorMsg());
                                std::string content = client->get_string();
                                for (const auto& line : client->server_reply().get_string_vec()) {
                                    content += line + "\n";
                                }
                                if (const auto node = client->server_reply().client_node(); node) {
                                    content += node->name();
                                }
                                BOOST_CHECK_MESSAGE(content.find(request.expected) != std::string::npos, content);
                            }
                        }
                    }
                }

                // --zombie_get lists the zombies of the readable tasks only
                BOOST_TEST_CONTEXT("--zombie_get by " << user.user) {
                    auto client = make();
                    BOOST_REQUIRE_MESSAGE(client->zombieGet() == 0, client->errorMsg());
                    std::set<std::string> listed;
                    for (const auto& zombie : client->server_reply().zombies()) {
                        listed.insert(zombie.path_to_task());
                    }
                    const auto zombies = user.user == "bob" ? std::set<std::string>{"/open/t"}
                                                            : std::set<std::string>{"/open/t", "/own/t", "/m/hidden/t"};
                    CHECK_PATHS(listed, zombies);
                }
            }
        }
    }
}

///
/// @brief Verifies that the state shown at the root is aggregated from the readable suites a client receives, with
///        and without a handle, and that the news reports its changes.
///
BOOST_AUTO_TEST_CASE(test_root_state) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load_suites(server);
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->force("/own/t", "aborted") == 0, administrator->errorMsg());
    BOOST_REQUIRE_MESSAGE(administrator->force("/open/t", "complete") == 0, administrator->errorMsg());

    const auto aggregate = [](const ClientInvoker& client) {
        return ecf::theComputedNodeState(client.defs()->suites(), true);
    };
    auto for_alice = server.client(alice);
    BOOST_REQUIRE_MESSAGE(for_alice->sync_local() == 0, for_alice->errorMsg());
    BOOST_CHECK_EQUAL(for_alice->defs()->state(), NState::ABORTED);

    auto for_bob = server.client(bob);
    BOOST_REQUIRE_MESSAGE(for_bob->sync_local() == 0, for_bob->errorMsg());
    BOOST_CHECK_NE(for_bob->defs()->state(), NState::ABORTED);
    BOOST_CHECK_EQUAL(for_bob->defs()->state(), aggregate(*for_bob));

    // --query state / gives the same root
    for (const auto& user : {alice, bob}) {
        auto client = server.client(user);
        BOOST_REQUIRE_MESSAGE(client->query("state", "/", "") == 0, client->errorMsg());
        BOOST_CHECK_EQUAL(client->get_string(),
                          user.user == "alice" ? std::string("aborted") : NState::toString(for_bob->defs()->state()));
    }

    auto bob_handle = server.client(bob);
    BOOST_REQUIRE_MESSAGE(bob_handle->ch_register(false, {"open"}) == 0, bob_handle->errorMsg());
    BOOST_REQUIRE_MESSAGE(bob_handle->sync_local() == 0, bob_handle->errorMsg());
    BOOST_CHECK_EQUAL(bob_handle->defs()->state(), NState::COMPLETE);

    // A change of a readable suite of the handle: news, then the root follows
    BOOST_REQUIRE_MESSAGE(administrator->force("/open/t", "aborted") == 0, administrator->errorMsg());
    for (auto* client : {for_bob.get(), bob_handle.get()}) {
        BOOST_REQUIRE_MESSAGE(client->news_local() == 0, client->errorMsg());
        BOOST_CHECK(client->get_news());
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
        BOOST_CHECK_EQUAL(client->defs()->state(), NState::ABORTED);
        BOOST_CHECK_EQUAL(client->defs()->state(), aggregate(*client));
    }
}

///
/// @brief Verifies that --get, --get_state and --migrate of a readable node omit its unreadable descendants.
///
BOOST_AUTO_TEST_CASE(test_node_replies_omit_unreadable_descendants) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load_suites(server);

    for (const auto& request : {CtsApi::get("/m"), CtsApi::get_state("/m"), CtsApi::migrate("/m")}) {
        BOOST_TEST_CONTEXT(request) {
            for (const auto& [user, nodes] : std::vector<std::pair<Credentials, std::set<std::string>>>{
                     {bob, {"/m", "/m/seen", "/m/seen/t"}},
                     {alice, {"/m", "/m/seen", "/m/seen/t", "/m/hidden", "/m/hidden/t"}}}) {
                auto client = server.client(user);
                BOOST_REQUIRE_MESSAGE(client->invoke(request) == 0, client->errorMsg());
                BOOST_REQUIRE(client->server_reply().client_node());
                CHECK_PATHS(ecf::test::node_paths(*client->server_reply().client_node()), nodes);
            }
        }
    }
}

///
/// @brief Verifies that the errors refusing to delete, archive or begin a suite with active tasks never name a task the
///        requester cannot read (R5.2).
///
BOOST_AUTO_TEST_CASE(test_precondition_errors_name_readable_tasks_only) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(
        admin, {alice}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "rwxop"}])");
    auto defs = Defs::create();
    auto s    = defs->add_suite("s");
    s->add_family("f")->add_task("t");
    auto g = s->add_family("g");
    g->add_task("t");
    g->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->load(defs) == 0, administrator->errorMsg());
    BOOST_REQUIRE_MESSAGE(administrator->force("/s/g/t", "active") == 0, administrator->errorMsg());

    auto client = server.client(alice);
    BOOST_CHECK(client->delete_node("/s") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("active tasks") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("/s/g/t") == std::string::npos, client->errorMsg());
    BOOST_CHECK(client->begin("s") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("not readable") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("/s/g/t") == std::string::npos, client->errorMsg());

    // The administrator, who can read the task, is told which it is
    BOOST_CHECK(administrator->delete_node("/s") != 0);
    BOOST_CHECK_MESSAGE(administrator->errorMsg().find("/s/g/t") != std::string::npos, administrator->errorMsg());

    // A suite the requester cannot read, with an active task, is neither named nor described by --begin
    BOOST_REQUIRE_MESSAGE(administrator->force("/s/g/t", "complete") == 0, administrator->errorMsg());
    auto hidden = Defs::create();
    auto h      = hidden->add_suite("hidden_suite");
    h->add_task("t");
    h->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
    BOOST_REQUIRE_MESSAGE(administrator->load(hidden) == 0, administrator->errorMsg());
    BOOST_REQUIRE_MESSAGE(administrator->force("/hidden_suite/t", "active") == 0, administrator->errorMsg());
    BOOST_CHECK(client->begin_all_suites() != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("not readable") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("hidden_suite") == std::string::npos, client->errorMsg());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

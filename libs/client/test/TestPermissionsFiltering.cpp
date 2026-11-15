// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ReadableSet.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/client/ClientInvoker.hpp"
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
    const auto file = server.directory() / "filtering.def";
    std::ofstream(file)
        << "suite open\n  task t\nendsuite\n"
        << "suite own\n"
        << R"(  permissions [{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "r"}])"
        << "\n  task t\nendsuite\n"
        << "suite m\n  family seen\n    task t\n  endfamily\n  family hidden\n"
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

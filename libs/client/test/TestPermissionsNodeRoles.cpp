// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The roles of the commands that operate on nodes (D2.10, R5.12, D5.19, D11.1, R11.3), in Permissions mode and in
/// white-list mode (D13.4), over TCP and HTTP.
///
/// Each command is sent by users holding different roles; the test checks only whether the request is denied, since an
/// allowed request may still fail for other reasons (e.g. --run of a task without script).
///

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials reader{"reader", "reader-secret"};        // r
const Credentials writer{"writer", "writer-secret"};        // rw
const Credentials executor{"executor", "executor-secret"};  // rx
const Credentials granter{"granter", "granter-secret"};     // rwp
const Credentials operator_{"operator", "operator-secret"}; // rwx
const Credentials stranger{"stranger", "stranger-secret"};  // not listed

const Credentials node_writer{"nodewriter", "nodewriter-secret"};       // white list: write access to /s/f/t only
const Credentials scoped_reader{"scopedreader", "scopedreader-secret"}; // white list: read access to /s only

const std::vector<Credentials>
    users{reader, writer, executor, granter, operator_, stranger, node_writer, scoped_reader};

///
/// @brief A command, as its command-line options, the roles it requires, and the nodes it is checked at.
///
struct Case
{
    std::vector<std::string> options; ///< The options
    std::string roles;                ///< The roles required, all of them
    std::vector<std::string> targets; ///< The nodes checked ("/" for the server)

    /// @brief Returns whether a target lies in the suite secret, open to the administrator only.
    [[nodiscard]] bool secret() const {
        return std::any_of(targets.begin(), targets.end(), [](const auto& t) { return t.rfind("/secret", 0) == 0; });
    }
};

///
/// @brief Returns the commands of this step, each on the suite s (open to every listed user) and, where meaningful,
///        on the suite secret.
///
/// @param[in] script A script on the client, for --edit_script ... submit, submit_file and pre_process_file
///
std::vector<Case> cases(const std::string& script) {
    std::vector<Case> result;
    const auto in_secret = [](std::string text) {
        for (std::size_t at; (at = text.find("/s/")) != std::string::npos;) {
            text.replace(at, 3, "/secret/");
        }
        return text;
    };
    const auto both = [&](const std::vector<std::string>& options, const std::string& roles, const std::string& node) {
        result.push_back(Case{options, roles, {node}});
        std::vector<std::string> secret;
        for (const auto& option : options) {
            secret.push_back(in_secret(option));
        }
        result.push_back(Case{secret, roles, {in_secret(node)}});
    };

    // Reads: scripts, jobs and outputs, checks (D2.10, R11.3, D11.3)
    for (const auto* kind : {"script", "job", "jobout", "manual", "kill", "stat"}) {
        both({"--file=/s/f/t", kind}, "r", "/s/f/t");
    }
    both({"--edit_script=/s/f/t", "edit"}, "r", "/s/f/t");
    both({"--edit_script=/s/f/t", "pre_process"}, "r", "/s/f/t");
    both({"--edit_script=/s/f/t", "pre_process_file", script}, "r", "/s/f/t");
    both({"--check=/s/f/t"}, "r", "/s/f/t");
    result.push_back(Case{{"--check=_all_"}, "r", {"/"}});
    // The edit history is read at server level, whatever the node (D5.28)
    result.push_back(Case{{"--edit_history=/s/f/t"}, "r", {"/"}});
    result.push_back(Case{{"--edit_history=/secret/f/t"}, "r", {"/"}});

    // Changes of attributes and definitions (D2.10, D11.1)
    both({"--alter", "add", "variable", "X", "y", "/s/f/t"}, "w", "/s/f/t");
    both({"--alter", "change", "label", "l", "w", "/s/f/t"}, "w", "/s/f/t");
    both({"--alter", "delete", "variable", "X", "/s/f/t"}, "w", "/s/f/t");
    both({"--alter", "set_flag", "late", "/s/f/t"}, "w", "/s/f/t");
    both({"--alter", "sort", "variable", "/s/f/t"}, "w", "/s/f/t");
    both({"--order=/s/f/t", "top"}, "w", "/s/f/t");
    result.push_back(Case{{"--alter", "add", "variable", "X", "y", "/"}, "w", {"/"}});
    result.push_back(Case{{"--edit_history=clear"}, "w", {"/"}});

    // Operations (D2.10); an event named as path:event is checked at its node
    both({"--suspend=/s/f/t"}, "x", "/s/f/t");
    both({"--resume=/s/f/t"}, "x", "/s/f/t");
    both({"--force=complete", "/s/f/t"}, "x", "/s/f/t");
    both({"--force=set", "/s/f/t:e"}, "x", "/s/f/t");
    both({"--requeue=/s/f/t"}, "x", "/s/f/t");
    both({"--free-dep=/s/f/t"}, "x", "/s/f/t");
    both({"--kill=/s/f/t"}, "x", "/s/f/t");
    both({"--status=/s/f/t"}, "x", "/s/f/t");
    both({"--run=/s/f/t"}, "x", "/s/f/t");
    both({"--edit_script=/s/f/t", "submit", script}, "x", "/s/f/t");
    both({"--edit_script=/s/f/t", "submit_file", script}, "x", "/s/f/t");
    // Creating an alias also defines a node
    both({"--edit_script=/s/f/t", "submit_file", script, "create_alias", "no_run"}, "xw", "/s/f/t");

    // Every target is checked, in any order (R5.12)
    for (const auto& [first, second] : std::vector<std::pair<std::string, std::string>>{
             {"/s/f/t", "/secret/f/t"}, {"/secret/f/t", "/s/f/t"}, {"/", "/secret/f/t"}}) {
        result.push_back(Case{{"--alter", "add", "variable", "Y", "z", first, second}, "w", {first, second}});
        if (first != "/") {
            result.push_back(Case{{"--suspend", first, second}, "x", {first, second}});
        }
    }

    // Archive changes the tree below the node; restore may bring Permissions attributes back (R4.5)
    both({"--archive=/s/f"}, "w", "/s/f");
    both({"--restore=/s/f"}, "wp", "/s/f");
    return result;
}

///
/// @brief Loads the suites s and secret, each holding family f with task t (event e, label l).
///
void load_suites(const PermissionsServer& server, bool attributes) {
    auto defs = Defs::create();
    for (const auto* name : {"s", "secret"}) {
        auto suite  = defs->add_suite(name);
        auto family = suite->add_family("f");
        auto task   = family->add_task("t");
        task->addEvent(Event("e"));
        task->addLabel(Label("l", "v"));
        if (attributes && std::string(name) == "secret") {
            suite->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
        }
    }
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

bool denied(const PermissionsServer& server, const Credentials& user, const Case& c) {
    auto client = server.client(user);
    std::vector<std::string> arguments{"ClientInvoker"};
    arguments.insert(arguments.end(), c.options.begin(), c.options.end());
    (void)client->invoke(arguments);
    return client->server_reply().permission_denied();
}

std::string describe(const Case& c) {
    std::string text;
    for (const auto& option : c.options) {
        text += (text.empty() ? "" : " ") + option;
    }
    return text;
}

///
/// @brief Runs every case for every user, checking the expected denials.
///
/// @tparam Allowed A callable `bool(const Credentials&, const Case&)`, returning whether the user may send the case
/// @param[in] server The server
/// @param[in] script A script on the client
/// @param[in] allowed Returns whether a user may send a case
///
template <typename Allowed>
void check_cases(const PermissionsServer& server, const std::string& script, Allowed allowed) {
    for (const auto& c : cases(script)) {
        for (const auto& user : users) {
            BOOST_TEST_CONTEXT(describe(c) << " by " << user.user) {
                BOOST_CHECK_EQUAL(denied(server, user, c), !allowed(user, c));
            }
        }
    }
}

std::string settings_of(bool http) {
    return http ? R"("http": true)" : "";
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsNodeRoles)

///
/// @brief Provides a script on the client, uniquely named, for the duration of a test.
///
struct ScriptFixture
{
    ecf::test::scaffold::WithTestFile file{ecf::test::scaffold::AutomaticTestFile{"permissions_script.ecf"},
                                           "echo %ECF_NAME%\n"};
    [[nodiscard]] std::string script() const { return file.path().string(); }
};

///
/// @brief Verifies the roles each node command requires, in Permissions mode.
///
BOOST_FIXTURE_TEST_CASE(test_node_roles_in_permissions_mode, ScriptFixture) {
    ECF_NAME_THIS_TEST();

    const std::map<std::string, std::string> roles{
        {"reader", "r"}, {"writer", "rw"}, {"executor", "rx"}, {"granter", "rwp"}, {"operator", "rwx"}};
    std::string attribute = R"([{"user": "admin", "permissions": "rwxlops"})";
    for (const auto& [user, letters] : roles) {
        attribute += R"(, {"user": ")" + user + R"(", "permissions": ")" + letters + R"("})";
    }
    attribute += "]";

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(admin,
                                     users,
                                     attribute,
                                     std::chrono::seconds(30),
                                     std::nullopt,
                                     std::nullopt,
                                     false,
                                     settings_of(http));
            load_suites(server, true);
            check_cases(server, script(), [&](const Credentials& user, const Case& c) {
                if (c.secret() || !roles.count(user.user)) {
                    return false;
                }
                for (const char role : c.roles) {
                    if (roles.at(user.user).find(role) == std::string::npos) {
                        return false;
                    }
                }
                return true;
            });
        }
    }
}

///
/// @brief Verifies that, in white-list mode, node commands require read or write access by their effects (D13.4).
///
BOOST_FIXTURE_TEST_CASE(test_node_roles_in_whitelist_mode, ScriptFixture) {
    ECF_NAME_THIS_TEST();

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(
                admin, users, "", std::chrono::seconds(30), std::nullopt, std::nullopt, false, settings_of(http));
            std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists"))
                << "4.5.0\nadmin\nwriter\nexecutor\ngranter\noperator\n-reader\nnodewriter /s/f/t\n"
                << "-scopedreader /s\n";
            BOOST_REQUIRE_EQUAL(server.administrator()->reloadwsfile(), 0);
            load_suites(server, false);
            check_cases(server, script(), [](const Credentials& user, const Case& c) {
                if (user.user == "stranger") {
                    return false;
                }
                // The users restricted to some nodes are allowed only when every target lies there
                const auto within = [&](const std::string& scope) {
                    return std::all_of(c.targets.begin(), c.targets.end(), [&](const std::string& target) {
                        return target == scope || target.rfind(scope + "/", 0) == 0;
                    });
                };
                if (user.user == "nodewriter") {
                    return within("/s/f/t");
                }
                if (user.user == "scopedreader") {
                    return c.roles == "r" && within("/s");
                }
                return c.roles == "r" || user.user != "reader";
            });
        }
    }
}

///
/// @brief Verifies that node roles are independent: a user holding w but not r on a suite changes it, and neither
///        reads nor operates it.
///
BOOST_AUTO_TEST_CASE(test_write_without_read) {
    ECF_NAME_THIS_TEST();

    const Credentials blind{"blind", "blind-secret"};
    PermissionsServer server(
        admin, {blind}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "blind", "permissions": "r"}])");
    {
        auto defs  = Defs::create();
        auto suite = defs->add_suite("w_only");
        suite->add_task("t")->addLabel(Label("l", "v"));
        suite->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,blind:w"));
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }

    auto client = server.client(blind);
    BOOST_CHECK_MESSAGE(client->alter(std::vector<std::string>{"/w_only/t"}, "change", "label", "l", "changed") == 0,
                        client->errorMsg());
    BOOST_CHECK(client->getDefs() == 0); // the server level: r
    BOOST_CHECK(client->query("label", "/w_only/t", "l") != 0);
    BOOST_CHECK(client->server_reply().permission_denied());
    BOOST_CHECK(client->suspend("/w_only/t") != 0);
    BOOST_CHECK(client->server_reply().permission_denied());

    auto check = server.administrator();
    BOOST_REQUIRE_MESSAGE(check->getDefs() == 0, check->errorMsg());
    BOOST_CHECK_EQUAL(check->defs()->findAbsNode("/w_only/t")->find_label("l").new_value(), "changed");
}

///
/// @brief Verifies that a request denied on one of its targets changes none of them, whatever their order (R5.12).
///
BOOST_AUTO_TEST_CASE(test_denied_targets_change_nothing) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(
        admin, users, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "operator", "permissions": "rwx"}])");
    load_suites(server, true);
    auto client = server.client(operator_);
    for (const auto& order :
         std::vector<std::vector<std::string>>{{"/s/f/t", "/secret/f/t"}, {"/secret/f/t", "/s/f/t"}}) {
        BOOST_TEST_CONTEXT("order: " << order.front() << ", " << order.back()) {
            BOOST_CHECK(client->suspend(order) != 0);
            BOOST_CHECK(client->server_reply().permission_denied());
            BOOST_CHECK(client->alter(order, "add", "variable", "Y", "z") != 0);
            BOOST_CHECK(client->server_reply().permission_denied());

            auto check = server.administrator();
            BOOST_REQUIRE_MESSAGE(check->getDefs() == 0, check->errorMsg());
            for (const auto* path : {"/s/f/t", "/secret/f/t"}) {
                const auto node = check->defs()->findAbsNode(path);
                BOOST_CHECK(!node->isSuspended());
                BOOST_CHECK(node->findVariable("Y").empty());
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

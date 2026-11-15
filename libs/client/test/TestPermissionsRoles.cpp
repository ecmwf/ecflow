// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The roles of the server-level, read and client handle commands (D10.1, D10.6, D2.21, R11.3, D10.7), in Permissions
/// mode and in white-list mode (D13.4), over TCP and HTTP.
///
/// Each command is sent by users holding different roles; the test checks only whether the request is denied, since an
/// allowed request may still fail for other reasons (e.g. --restore_from_checkpt on a server holding suites).
///

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials reader{"reader", "reader-secret"};       // r
const Credentials writer{"writer", "writer-secret"};       // rw
const Credentials executor{"executor", "executor-secret"}; // rx
const Credentials logger{"logger", "logger-secret"};       // rl
const Credentials stranger{"stranger", "stranger-secret"}; // not listed by the server attribute, nor the white list
const Credentials scoped{"scoped", "scoped-secret"};       // in white-list mode, write access to the suite s only

const std::vector<Credentials> users{reader, writer, executor, logger, stranger, scoped};

///
/// @brief A command, as its command-line options, and the role it requires.
///
struct Case
{
    std::vector<std::string> options; ///< The options, e.g. {"--halt=yes"}
    char role;                        ///< The role required at the target: r, w, x or l
    std::string target;               ///< "/" for the server, or the node addressed
};

///
/// @brief Returns the commands of this step, in an order that leaves the server usable by the next ones.
///
std::vector<Case> cases() {
    return {
        // Reads (D10.1)
        {{"--server_version"}, 'r', "/"},
        {{"--stats"}, 'r', "/"},
        {{"--stats_server"}, 'r', "/"},
        {{"--server_load"}, 'r', "/"},
        {{"--suites"}, 'r', "/"},
        {{"--zombie_get"}, 'r', "/"},
        {{"--get"}, 'r', "/"},
        {{"--show"}, 'r', "/"},
        {{"--get_state"}, 'r', "/"},
        {{"--migrate"}, 'r', "/"},
        {{"--get=/s"}, 'r', "/s"},
        {{"--get_state=/s/t"}, 'r', "/s"},
        {{"--migrate=/s"}, 'r', "/s"},
        {{"--why=/s/t"}, 'r', "/s"},
        {{"--query", "state", "/s/t"}, 'r', "/s"},
        {{"--get=/secret"}, 'r', "/secret"},
        {{"--get_state=/secret/t"}, 'r', "/secret"},
        {{"--query", "state", "/secret/t"}, 'r', "/secret"},
        {{"--query", "variable", "/:ECF_HOME"}, 'r', "/"},
        {{"--log=get"}, 'r', "/"},
        {{"--log=path"}, 'r', "/"},
        {{"--ch_suites"}, 'r', "/"},
        {{"--news=0", "0", "0"}, 'r', "/"},
        {{"--sync=0", "0", "0"}, 'r', "/"},
        {{"--sync_full=0"}, 'r', "/"},
        {{"--sync_clock=0", "0", "0"}, 'r', "/"},
        // Changes of the server state (D10.6)
        {{"--msg=hello"}, 'w', "/"},
        {{"--stats_reset"}, 'w', "/"},
        {{"--debug_server_on"}, 'w', "/"},
        {{"--debug_server_off"}, 'w', "/"},
        {{"--zombie_fob=/s/t"}, 'w', "/"},
        {{"--zombie_kill=/s/t"}, 'w', "/"},
        {{"--zombie_remove=/s/t"}, 'w', "/"},
        // Changes of the server log (D2.21)
        {{"--log=flush"}, 'l', "/"},
        {{"--log=clear"}, 'l', "/"},
        {{"--log=new"}, 'l', "/"},
        // Server administration (D10.1)
        {{"--check_pt"}, 'x', "/"},
        {{"--check_pt=never"}, 'x', "/"},
        {{"--check_pt=on_time"}, 'x', "/"},
        {{"--reloadwsfile"}, 'x', "/"},
        {{"--reloadpasswdfile"}, 'x', "/"},
        {{"--reloadcustompasswdfile"}, 'x', "/"},
        {{"--force-dep-eval"}, 'x', "/"},
        {{"--job_gen=/s"}, 'x', "/s"},
        {{"--checkJobGenOnly=/secret"}, 'x', "/secret"},
        {{"--checkJobGenOnly=/s"}, 'x', "/s"},
        {{"--shutdown=yes"}, 'x', "/"},
        {{"--halt=yes"}, 'x', "/"},
        {{"--restore_from_checkpt"}, 'x', "/"},
        {{"--restart"}, 'x', "/"},
    };
}

///
/// @brief Loads the suites: s, without attribute, and secret, readable and changeable by the administrator only.
///
void load_suites(const PermissionsServer& server, bool attributes) {
    auto defs = Defs::create();
    defs->add_suite("s")->add_task("t");
    auto secret = defs->add_suite("secret");
    secret->add_task("t");
    if (attributes) {
        secret->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
    }
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

///
/// @brief Sends a command, and returns whether it was denied.
///
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
/// @param[in] allowed Returns whether a user may send a case
///
template <typename Allowed>
void check_cases(const PermissionsServer& server, Allowed allowed) {
    for (const auto& c : cases()) {
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

BOOST_AUTO_TEST_SUITE(T_PermissionsRoles)

///
/// @brief Verifies the role each server-level and read command requires, in Permissions mode.
///
BOOST_AUTO_TEST_CASE(test_roles_in_permissions_mode) {
    ECF_NAME_THIS_TEST();

    const std::map<std::string, std::string> roles{
        {"reader", "r"}, {"writer", "rw"}, {"executor", "rx"}, {"logger", "rl"}};
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
            check_cases(server, [&](const Credentials& user, const Case& c) {
                // Nobody but the administrator may act on secret; the stranger may do nothing
                if (c.target == "/secret" || !roles.count(user.user)) {
                    return false;
                }
                return roles.at(user.user).find(c.role) != std::string::npos;
            });
        }
    }
}

///
/// @brief Verifies that, in white-list mode, the same commands require read or write access by their effects (D13.4,
///        R11.3).
///
BOOST_AUTO_TEST_CASE(test_roles_in_whitelist_mode) {
    ECF_NAME_THIS_TEST();

    for (const bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            PermissionsServer server(
                admin, users, "", std::chrono::seconds(30), std::nullopt, std::nullopt, false, settings_of(http));
            std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists"))
                << "4.5.0\nadmin\nwriter\nexecutor\nlogger\n-reader\nscoped /s\n";
            BOOST_REQUIRE_EQUAL(server.administrator()->reloadwsfile(), 0);
            load_suites(server, false);
            check_cases(server, [](const Credentials& user, const Case& c) {
                if (user.user == "stranger") {
                    return false;
                }
                if (user.user == "scoped") {
                    return c.target == "/s"; // the targets of the command, not the server, are checked
                }
                return c.role == 'r' || user.user != "reader";
            });
        }
    }
}

///
/// @brief Verifies that client handles are bound to their creator (D10.7), in Permissions and white-list modes, over
///        TCP and HTTP, whether the commands are sent alone or in a group.
///
BOOST_AUTO_TEST_CASE(test_handles_bound_to_their_creator) {
    ECF_NAME_THIS_TEST();

    const std::string attribute = R"([{"user": "admin", "permissions": "rwxlops"}, )"
                                  R"({"user": "reader", "permissions": "r"}, {"user": "writer", "permissions": "rw"}])";
    for (const bool whitelist : {false, true}) {
        for (const bool http : {false, true}) {
            BOOST_TEST_CONTEXT("whitelist: " << whitelist << ", http: " << http) {
                PermissionsServer server(admin,
                                         users,
                                         whitelist ? "" : attribute,
                                         std::chrono::seconds(30),
                                         std::nullopt,
                                         std::nullopt,
                                         false,
                                         settings_of(http));
                if (whitelist) {
                    std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists"))
                        << "4.5.0\nadmin\nwriter\n-reader\n";
                    BOOST_REQUIRE_EQUAL(server.administrator()->reloadwsfile(), 0);
                }
                load_suites(server, false);

                auto owner = server.client(reader);
                BOOST_REQUIRE_MESSAGE(owner->ch_register(false, {"s"}) == 0, owner->errorMsg());
                const int handle = owner->client_handle();
                BOOST_REQUIRE_GT(handle, 0);
                const auto h = std::to_string(handle);

                // Another user, whatever their roles, can neither change, replace nor drop the handle, alone or in a
                // group
                auto other = server.client(writer);
                const std::vector<std::vector<std::string>> attempts{
                    {"--ch_add=" + h, "secret"},
                    {"--ch_rem=" + h, "s"},
                    {"--ch_auto_add=" + h, "true"},
                    {"--ch_drop=" + h},
                    {"--ch_drop_user=reader"},
                    {"--ch_register=" + h, "false", "secret"},
                    {"--group=ch_drop " + h + "; ping"},
                    {"--group=ch_register " + h + " false secret"},
                };
                for (const auto& attempt : attempts) {
                    BOOST_TEST_CONTEXT(attempt.front()) {
                        std::vector<std::string> arguments{"ClientInvoker"};
                        arguments.insert(arguments.end(), attempt.begin(), attempt.end());
                        BOOST_CHECK(other->invoke(arguments) != 0);
                        BOOST_CHECK(other->server_reply().permission_denied());
                    }
                }

                // The handle is intact, and its creator may use it
                BOOST_CHECK_EQUAL(owner->ch_add(handle, {"secret"}), 0);
                BOOST_CHECK_EQUAL(owner->ch_auto_add(handle, true), 0);

                // A handle that does not exist is reported as such, not as a denial
                BOOST_CHECK(other->ch_drop(handle + 1000) != 0);
                BOOST_CHECK(!other->server_reply().permission_denied());

                // A user may drop their own handles, named or not
                BOOST_REQUIRE_EQUAL(other->ch_register(false, {"s"}), 0);
                BOOST_CHECK_EQUAL(other->ch_drop_user("writer"), 0);
                BOOST_REQUIRE_EQUAL(other->ch_register(false, {"s"}), 0);
                BOOST_CHECK_EQUAL(other->ch_drop_user(""), 0);

                // The creator may replace and drop the handle
                std::vector<std::string> replace{"ClientInvoker", "--ch_register=" + h, "false", "s"};
                BOOST_CHECK_EQUAL(owner->invoke(replace), 0);
                BOOST_CHECK_EQUAL(owner->ch_drop(owner->client_handle()), 0);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

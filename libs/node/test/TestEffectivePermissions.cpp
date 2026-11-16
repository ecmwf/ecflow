// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;
using ecf::Roles;

namespace {

///
/// @brief Builds definitions from the given text, with the given server attribute (command-line form), if any.
///
defs_ptr make(const std::string& server, const std::string& text) {
    auto defs = Defs::create();
    std::string error;
    std::string warning;
    BOOST_REQUIRE_MESSAGE(defs->restore_from_string(text, error, warning), error);
    if (!server.empty()) {
        defs->server_state().set_permissions(Permissions::parse_cli(server));
    }
    return defs;
}

std::string attribute(const std::string& cli) {
    return "permissions " + Permissions::parse_cli(cli).to_json() + "\n";
}

///
/// @brief Returns the letters of the effective roles of a user at a path, or "-" when Permissions do not apply.
///
std::string roles(const defs_ptr& defs, const std::string& path, const std::string& user) {
    const auto result = ecf::effective_permissions(*defs, path, user);
    return result ? result->to_string() : "-";
}

struct Case
{
    std::string name;                                          ///< What the case shows, and the decision it relies on
    std::string server;                                        ///< The server attribute, command-line form
    std::string text;                                          ///< The definitions
    std::vector<std::pair<std::string, std::string>> expected; ///< (path, roles of alice)
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_EffectivePermissions)

///
/// @brief Verifies the effective permissions of a user on the cases of the decisions D5.5 to D5.13.
///
BOOST_AUTO_TEST_CASE(test_effective_permissions_cases) {
    ECF_NAME_THIS_TEST();

    const std::vector<Case> cases{
        {"D5.8: without a server attribute, Permissions do not apply",
         "",
         "suite s\n" + attribute("alice:r") + "endsuite\n",
         {{"/", "-"}, {"/s", "-"}}},
        {"D5.5: a user not listed by the server has nothing, whatever the suites grant",
         "bob:r",
         "suite s\n" + attribute("alice:rwx,bob:r") + "endsuite\n",
         {{"/", ""}, {"/s", ""}}},
        {"D5.9: without r at server level, a user has nothing",
         "alice:wx,bob:r",
         "suite s\n" + attribute("alice:rwx") + "endsuite\n",
         {{"/", ""}, {"/s", ""}}},
        {"D5.5: the server level gives the roles of the server entry",
         "alice:rwxlop",
         "suite s\nendsuite\n",
         {{"/", "rwxlop"}}},
        {"D5.7: a suite without attribute takes the server entries",
         "alice:rwx",
         "suite s\n  family f\n    task t\n  endfamily\nendsuite\n",
         {{"/s", "rwx"}, {"/s/f", "rwx"}, {"/s/f/t", "rwx"}}},
        {"D5.6, D5.12: a suite replaces the non-sticky server entry, and may broaden it",
         "alice:r",
         "suite s\n" + attribute("alice:rwxo") + "endsuite\n",
         {{"/", "r"}, {"/s", "rwxo"}}},
        {"D5.6: a suite that does not list the user removes every right, and nothing below restores them",
         "alice:rwx,bob:r",
         "suite s\n" + attribute("bob:r") + "  family f\n" + attribute("alice:rwx") +
             "    task t\n  endfamily\nendsuite\n",
         {{"/s", ""}, {"/s/f", ""}, {"/s/f/t", ""}}},
        {"D5.10: below the suite, rights are intersected",
         "alice:rwx",
         "suite s\n" + attribute("alice:r") + "  family f\n" + attribute("alice:rwx") +
             "    task t\n  endfamily\nendsuite\n",
         {{"/s", "r"}, {"/s/f", "r"}, {"/s/f/t", "r"}}},
        {"D5.10: a node below the suite removes roles, and a node further down cannot restore them",
         "alice:rwx",
         "suite s\n" + attribute("alice:rwx") + "  family f\n" + attribute("alice:r") + "    task t\n" +
             attribute("alice:rw") + "    task u\n" + attribute("alice:w") + "  endfamily\nendsuite\n",
         {{"/s", "rwx"}, {"/s/f", "r"}, {"/s/f/t", "r"}, {"/s/f/u", ""}}},
        {"D5.6: a node below the suite that does not list the user removes every right below it",
         "alice:rwx,bob:r",
         "suite s\n" + attribute("alice:rwx,bob:r") + "  family f\n" + attribute("bob:r") +
             "    task t\n  endfamily\n  task u\nendsuite\n",
         {{"/s", "rwx"}, {"/s/f", ""}, {"/s/f/t", ""}, {"/s/u", "rwx"}}},
        {"D5.11, D5.12: a sticky server entry is granted everywhere, united with the rights below",
         "alice:rxs,bob:r",
         "suite s\n" + attribute("bob:r") + "endsuite\nsuite t\n" + attribute("alice:w,bob:r") + "endsuite\n",
         {{"/", "rx"}, {"/s", "rx"}, {"/t", "rwx"}}},
        {"D5.12: a sticky entry below the server locks the set reached there",
         "alice:rwx",
         "suite s\n" + attribute("alice:rw") + "  family f\n" + attribute("alice:rwxs") + "    task t\n" +
             attribute("alice:r") + "    task u\n" + attribute("bob:r") + "  endfamily\nendsuite\n",
         {{"/s", "rw"}, {"/s/f", "rw"}, {"/s/f/t", "rw"}, {"/s/f/u", "rw"}}},
        {"D5.13: a sticky entry on a suite locks the suite's set",
         "alice:r",
         "suite s\n" + attribute("alice:rws") + "  family f\n" + attribute("alice:r") +
             "    task t\n  endfamily\nendsuite\n",
         {{"/s", "rw"}, {"/s/f", "rw"}, {"/s/f/t", "rw"}}},
        {"D5.13: a sticky entry on a task is intersected, then locked",
         "alice:rwx",
         "suite s\n" + attribute("alice:rw") + "  task t\n" + attribute("alice:rxs") + "endsuite\n",
         {{"/s/t", "r"}}},
    };

    for (const auto& c : cases) {
        BOOST_TEST_CONTEXT(c.name) {
            const auto defs = make(c.server, c.text);
            for (const auto& [path, expected] : c.expected) {
                BOOST_TEST_CONTEXT("path " << path) {
                    BOOST_CHECK_EQUAL(roles(defs, path, "alice"), expected);
                }
            }
        }
    }
}

///
/// @brief Verifies that a path without a node is refused.
///
BOOST_AUTO_TEST_CASE(test_effective_permissions_unknown_path) {
    ECF_NAME_THIS_TEST();

    const auto defs = make("alice:r", "suite s\nendsuite\n");
    BOOST_CHECK_THROW((void)ecf::effective_permissions(*defs, "/s/missing", "alice"), std::invalid_argument);
}

///
/// @brief Verifies that the top-down walk gives, on every node, the same result as the walk along the path.
///
BOOST_AUTO_TEST_CASE(test_effective_permissions_top_down_walk) {
    ECF_NAME_THIS_TEST();

    const auto defs =
        make("alice:rxs,bob:rwx,carol:r",
             "suite s\n" + attribute("alice:w,bob:rw,carol:r") + "  family f\n" + attribute("bob:rws,carol:r") +
                 "    task t\n" + attribute("carol:r") + "  endfamily\n  task u\nendsuite\nsuite v\n  task w\n" +
                 attribute("bob:r") + "endsuite\n");

    for (const std::string user : {"alice", "bob", "carol", "dave"}) {
        BOOST_TEST_CONTEXT("user " << user) {
            const ecf::EffectivePermissions walker(*defs, user);
            BOOST_REQUIRE(walker.applies());

            // Carries the state from parent to child, as filtering a whole tree does
            std::function<void(const Node&, const ecf::EffectivePermissions::State&)> visit =
                [&](const Node& node, const ecf::EffectivePermissions::State& parent) {
                    const auto state = walker.descend(parent, node);
                    BOOST_CHECK_EQUAL(walker.roles(state).to_string(), roles(defs, node.absNodePath(), user));
                    std::vector<node_ptr> children;
                    node.immediateChildren(children);
                    for (const auto& child : children) {
                        visit(*child, state);
                    }
                };
            for (const auto& suite : defs->suites()) {
                visit(*suite, walker.at_server());
            }
            BOOST_CHECK_EQUAL(walker.roles(walker.at_server()).to_string(), roles(defs, "/", user));
        }
    }

    const ecf::EffectivePermissions unrestricted(*make("", "suite s\nendsuite\n"), "alice");
    BOOST_CHECK(!unrestricted.applies());
}

///
/// @brief Verifies the effective roles of every user at a path, as listed by ecFlowUI's Info panel.
///
BOOST_AUTO_TEST_CASE(test_effective_permissions_of_every_user) {
    ECF_NAME_THIS_TEST();

    // The layout of the manual ecFlowUI check, with a sticky entry and a user not admitted (no r at server level)
    const auto defs = make("admin:rwxlops,bob:r,mamb:rwxp,carol:w,dave:rs",
                           "suite s\n" + attribute("admin:rwxlop,bob:r,mamb:rwxp") + "  family f\n    task t\n" +
                               "  endfamily\n  family g\n" + attribute("admin:rwxlop,mamb:r") +
                               "    task t\n  endfamily\nendsuite\n");

    const auto listed = [&](const std::string& path) {
        const auto result = ecf::effective_permissions_of_every_user(*defs, path);
        BOOST_REQUIRE(result.has_value());
        std::string text;
        for (const auto& [user, roles] : *result) {
            text += (text.empty() ? "" : ",") + user + ":" + roles.to_string();
        }
        return text;
    };

    // Users in the order of the server attribute (by name); carol (not admitted) never listed; dave listed through the
    // sticky entry, even where the attribute below does not name the user
    BOOST_CHECK_EQUAL(listed("/"), "admin:rwxlop,bob:r,dave:r,mamb:rwxp");
    BOOST_CHECK_EQUAL(listed("/s"), "admin:rwxlop,bob:r,dave:r,mamb:rwxp");
    BOOST_CHECK_EQUAL(listed("/s/f"), "admin:rwxlop,bob:r,dave:r,mamb:rwxp");
    BOOST_CHECK_EQUAL(listed("/s/f/t"), "admin:rwxlop,bob:r,dave:r,mamb:rwxp");
    BOOST_CHECK_EQUAL(listed("/s/g"), "admin:rwxlop,dave:r,mamb:r");
    BOOST_CHECK_EQUAL(listed("/s/g/t"), "admin:rwxlop,dave:r,mamb:r");

    // Each entry agrees with effective_permissions() for the same user
    for (const std::string path : {"/", "/s", "/s/f/t", "/s/g/t"}) {
        const auto users = ecf::effective_permissions_of_every_user(*defs, path);
        BOOST_REQUIRE(users.has_value());
        for (const auto& [user, r] : *users) {
            BOOST_CHECK_EQUAL(r.to_string(), roles(defs, path, user));
        }
    }

    // Without a server attribute, Permissions do not apply; an unknown path is refused
    BOOST_CHECK(!ecf::effective_permissions_of_every_user(*make("", "suite s\nendsuite\n"), "/s").has_value());
    BOOST_CHECK_THROW((void)ecf::effective_permissions_of_every_user(*defs, "/s/missing"), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

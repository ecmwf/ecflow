// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <set>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Visibility.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;
using ecf::Visibility;

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
/// @brief Returns the paths of the nodes a user can read, according to the visibility.
///
std::set<std::string> readable(const Defs& defs, const Visibility& visibility) {
    std::set<std::string> result;
    for (const auto* node : ecf::get_all_nodes(defs)) {
        if (visibility.readable(*node)) {
            result.insert(node->absNodePath());
        }
    }
    return result;
}

///
/// @brief The definitions of the tests: suite open without attribute, suite own readable by alice only, and suite m
///        whose family hidden is readable by alice only.
///
defs_ptr reference() {
    return make("admin:rwxlop,alice:r,bob:r,carol:r",
                "suite open\n  task t\nendsuite\n"
                "suite own\n" +
                    attribute("admin:rwxlop,alice:r") +
                    "  task t\nendsuite\n"
                    "suite m\n  family seen\n    task t\n  endfamily\n  family hidden\n" +
                    attribute("admin:rwxlop,alice:r") + "    task t\n  endfamily\nendsuite\n");
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Visibility)

///
/// @brief Verifies that a node is readable when the user holds r there, and that its subtree is not otherwise.
///
BOOST_AUTO_TEST_CASE(test_readable_nodes) {
    ECF_NAME_THIS_TEST();

    const auto defs = reference();

    const Visibility alice(*defs, "alice");
    BOOST_CHECK(alice.applies());
    BOOST_CHECK_EQUAL(readable(*defs, alice).size(), ecf::get_all_nodes(*defs).size());

    const Visibility bob(*defs, "bob");
    const std::set<std::string> expected{"/open", "/open/t", "/m", "/m/seen", "/m/seen/t"};
    const auto actual = readable(*defs, bob);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    BOOST_CHECK(!bob.readable(*defs->findAbsNode("/m/hidden/t")));
}

///
/// @brief Verifies that users with the same readable nodes share a key, and users with different ones do not (R5.6).
///
BOOST_AUTO_TEST_CASE(test_key_identifies_the_readable_nodes) {
    ECF_NAME_THIS_TEST();

    const auto defs = reference();
    const Visibility alice(*defs, "alice");
    const Visibility bob(*defs, "bob");
    const Visibility carol(*defs, "carol");

    BOOST_CHECK_EQUAL(bob.key(), carol.key());
    BOOST_CHECK_NE(alice.key(), bob.key());
}

///
/// @brief Verifies that a user not admitted by the server attribute reads nothing.
///
BOOST_AUTO_TEST_CASE(test_user_not_admitted_reads_nothing) {
    ECF_NAME_THIS_TEST();

    const auto defs = reference();
    const Visibility stranger(*defs, "stranger");
    BOOST_CHECK(readable(*defs, stranger).empty());
}

///
/// @brief Verifies that, without a server attribute, every node is readable and every user shares one key (R5.4).
///
BOOST_AUTO_TEST_CASE(test_without_server_attribute_everything_is_readable) {
    ECF_NAME_THIS_TEST();

    const auto defs = make("", "suite s\n" + attribute("admin:rwxlop") + "  task t\nendsuite\n");
    const Visibility anyone(*defs, "anyone");
    BOOST_CHECK(!anyone.applies());
    BOOST_CHECK_EQUAL(readable(*defs, anyone).size(), ecf::get_all_nodes(*defs).size());
    BOOST_CHECK_EQUAL(anyone.key(), Visibility(*defs, "someone").key());
}

///
/// @brief Verifies that a sticky entry keeps a subtree readable below an attribute that omits the user (D5.12).
///
BOOST_AUTO_TEST_CASE(test_sticky_entry_keeps_subtree_readable) {
    ECF_NAME_THIS_TEST();

    const auto defs = make("admin:rwxlop,bob:r",
                           "suite s\n" + attribute("admin:rwxlop,bob:rs") + "  family f\n" + attribute("admin:rwxlop") +
                               "    task t\n  endfamily\nendsuite\n");
    const Visibility bob(*defs, "bob");
    BOOST_CHECK(bob.readable(*defs->findAbsNode("/s/f/t")));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

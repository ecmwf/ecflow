// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;
using ecf::Role;
using ecf::Roles;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Permissions)

///
/// @brief Verifies the letter of each role and the canonical order in which roles are listed.
///
BOOST_AUTO_TEST_CASE(test_permissions_role_letters) {
    ECF_NAME_THIS_TEST();

    std::string letters;
    for (const auto& role : ecf::all_roles()) {
        letters += ecf::letter_of(role);
    }
    BOOST_CHECK_EQUAL(letters, "rwxlop");
    BOOST_CHECK_EQUAL(ecf::letter_of(Role::Read), 'r');
    BOOST_CHECK_EQUAL(ecf::letter_of(Role::Permissions), 'p');
}

///
/// @brief Verifies that a set of roles supports membership, union and intersection.
///
BOOST_AUTO_TEST_CASE(test_permissions_roles_set_operations) {
    ECF_NAME_THIS_TEST();

    const Roles rw{Role::Read, Role::Write};
    const Roles rx{Role::Read, Role::Execute};

    BOOST_CHECK(rw.has(Role::Read));
    BOOST_CHECK(!rw.has(Role::Execute));
    BOOST_CHECK(Roles{}.empty());
    BOOST_CHECK(!rw.empty());
    BOOST_CHECK((rw & rx) == Roles{Role::Read});
    BOOST_CHECK((rw | rx) == (Roles{Role::Read, Role::Write, Role::Execute}));
    BOOST_CHECK(rw.contains(Roles{Role::Write}));
    BOOST_CHECK(!rw.contains(rx));
    BOOST_CHECK_EQUAL(rw.to_string(), "rw");
}

///
/// @brief Verifies the user name rule: no white space, '#', ',', ':' or ';', no leading '-', not '*', not empty.
///
BOOST_AUTO_TEST_CASE(test_permissions_user_name_rule) {
    ECF_NAME_THIS_TEST();

    for (const std::string valid : {"alice",
                                    "a",
                                    "bob.smith",
                                    "user_1",
                                    "x-y",
                                    "Ana",
                                    "o'neil",
                                    "o\"neil",
                                    "a\\b",
                                    "jos\xc3\xa9",
                                    "\xe2\x82\xac",
                                    "\xf0\x9f\x98\x80",
                                    "\xf4\x8f\xbf\xbf"}) {
        std::string error;
        BOOST_CHECK_MESSAGE(ecf::is_valid_user_name(valid, error), valid << ": " << error);
    }
    for (const std::string invalid : {"",
                                      "al ice",
                                      "al\tice",
                                      "al#ice",
                                      "al,ice",
                                      "al:ice",
                                      "al;ice",
                                      "-alice",
                                      "*",
                                      "al\xffice",           // byte never valid in UTF-8
                                      "al\xc3",              // truncated sequence
                                      "\xc3\x28",            // bad continuation
                                      "\xc0\xaf",            // overlong form
                                      "\xed\xa0\x80",        // surrogate
                                      "\xf4\x90\x80\x80"}) { // above U+10FFFF
        std::string error;
        BOOST_CHECK_MESSAGE(!ecf::is_valid_user_name(invalid, error), "accepted: '" << invalid << "'");
        BOOST_CHECK_MESSAGE(!error.empty(), "no error for: '" << invalid << "'");
    }
}

///
/// @brief Verifies that the command-line form is parsed into entries in canonical order, and printed canonically.
///
BOOST_AUTO_TEST_CASE(test_permissions_parse_cli_form) {
    ECF_NAME_THIS_TEST();

    const auto p = Permissions::parse_cli("bob:r,alice:sxwr,carol:pol");

    BOOST_REQUIRE_EQUAL(p.entries().size(), 3u);
    BOOST_CHECK_EQUAL(p.entries()[0].user, "alice");
    BOOST_CHECK(p.entries()[0].roles == (Roles{Role::Read, Role::Write, Role::Execute}));
    BOOST_CHECK(p.entries()[0].sticky);
    BOOST_CHECK_EQUAL(p.entries()[1].user, "bob");
    BOOST_CHECK(!p.entries()[1].sticky);
    BOOST_CHECK(p.entries()[2].roles == (Roles{Role::Log, Role::Owner, Role::Permissions}));

    BOOST_CHECK_EQUAL(p.to_cli(), "alice:rwxs,bob:r,carol:lop");
}

///
/// @brief Verifies that every invalid command-line form is rejected, with a message naming the problem.
///
BOOST_AUTO_TEST_CASE(test_permissions_parse_cli_form_rejects_invalid) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> invalid{
        "",                // empty list
        "alice",           // no roles separator
        "alice:",          // entry without roles
        "alice:s",         // only the sticky flag
        "alice:rq",        // unknown letter
        "alice:R",         // uppercase letter
        "alice:rr",        // repeated letter
        "alice:r w",       // white space
        "alice:r,alice:w", // user listed twice
        "alice:r,",        // empty entry
        ",alice:r",        // empty entry
        "alice:r,,bob:r",  // empty entry
        ":r",              // empty user
        "*:r",             // wildcard
        "-alice:r",        // leading dash
        "al#ice:r",        // reserved character
        "al;ice:r",        // reserved character
        "alice:r:w",       // separator in the roles
        " alice:r",        // white space
    };
    for (const auto& value : invalid) {
        BOOST_CHECK_THROW(Permissions::parse_cli(value), std::runtime_error);
    }

    try {
        Permissions::parse_cli("alice:r,alice:w");
        BOOST_FAIL("expected an exception");
    }
    catch (const std::runtime_error& e) {
        BOOST_CHECK_MESSAGE(std::string(e.what()).find("alice") != std::string::npos, e.what());
    }
}

///
/// @brief Verifies that the JSON form is parsed and printed, and that both forms describe the same value.
///
BOOST_AUTO_TEST_CASE(test_permissions_json_form) {
    ECF_NAME_THIS_TEST();

    const auto p = Permissions::parse_json(R"([{"user": "bob", "permissions": "r"},
                                               {"user": "alice", "permissions": "srw"}])");

    BOOST_CHECK_EQUAL(p.to_json(), R"([{"permissions":"rws","user":"alice"},{"permissions":"r","user":"bob"}])");
    BOOST_CHECK(p == Permissions::parse_cli(p.to_cli()));
    BOOST_CHECK(p == Permissions::parse_json(p.to_json()));
}

///
/// @brief Verifies that user names holding quotes, backslashes and non-ASCII characters survive both forms.
///
BOOST_AUTO_TEST_CASE(test_permissions_round_trip_of_special_user_names) {
    ECF_NAME_THIS_TEST();

    const Permissions p({{"o\"neil", Roles{Role::Read}, false},
                         {"a\\b", Roles{Role::Write}, false},
                         {"jos\xc3\xa9", Roles{Role::Execute}, true},
                         {"\xf4\x8f\xbf\xbf", Roles{Role::Read}, false},
                         {"\xe2\x82\xac", Roles{Role::Read}, false}});

    BOOST_CHECK(p == Permissions::parse_cli(p.to_cli()));
    BOOST_CHECK(p == Permissions::parse_json(p.to_json()));
    BOOST_CHECK(Permissions::parse_json(R"([{"user": "jos\u00e9", "permissions": "xs"}])").find("jos\xc3\xa9") !=
                nullptr);
    BOOST_CHECK_THROW(Permissions::parse_cli("al\xffice:r"), std::runtime_error);
}

///
/// @brief Verifies that every invalid JSON form is rejected.
///
BOOST_AUTO_TEST_CASE(test_permissions_json_form_rejects_invalid) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> invalid{
        R"([])",
        R"({})",
        R"("alice:r")",
        R"([{"user": "alice"}])",
        R"([{"permissions": "r"}])",
        R"([{"user": "alice", "permissions": "r", "extra": 1}])",
        R"([{"user": 1, "permissions": "r"}])",
        R"([{"user": "alice", "permissions": ["r"]}])",
        R"([{"user": "alice", "permissions": ""}])",
        R"([{"user": "alice", "permissions": "s"}])",
        R"([{"user": "alice", "permissions": "rq"}])",
        R"([{"user": "*", "permissions": "r"}])",
        R"([{"user": "alice", "permissions": "r"}, {"user": "alice", "permissions": "w"}])",
        R"(["alice"])",
        R"([{"user": "alice", "permissions": "r"})",
    };
    for (const auto& value : invalid) {
        BOOST_CHECK_THROW(Permissions::parse_json(value), std::runtime_error);
    }
}

///
/// @brief Verifies that construction from entries validates them and orders them canonically.
///
BOOST_AUTO_TEST_CASE(test_permissions_construction_from_entries) {
    ECF_NAME_THIS_TEST();

    const Permissions p({{"bob", Roles{Role::Read}, false}, {"alice", Roles{Role::Write}, true}});
    BOOST_CHECK_EQUAL(p.to_cli(), "alice:ws,bob:r");

    BOOST_CHECK_THROW(Permissions(std::vector<Permissions::Entry>{}), std::runtime_error);
    BOOST_CHECK_THROW(Permissions({{"alice", Roles{}, true}}), std::runtime_error);
    BOOST_CHECK_THROW(Permissions({{"alice", Roles{Role::Read}, false}, {"alice", Roles{Role::Read}, false}}),
                      std::runtime_error);
    BOOST_CHECK_THROW(Permissions({{"a b", Roles{Role::Read}, false}}), std::runtime_error);
}

///
/// @brief Verifies the lookup of the entry of a user.
///
BOOST_AUTO_TEST_CASE(test_permissions_find_entry) {
    ECF_NAME_THIS_TEST();

    const auto p = Permissions::parse_cli("alice:rw,bob:rs");

    BOOST_REQUIRE(p.find("bob") != nullptr);
    BOOST_CHECK(p.find("bob")->sticky);
    BOOST_CHECK(p.find("carol") == nullptr);
}

///
/// @brief Verifies equality, ordering independence and hashing.
///
BOOST_AUTO_TEST_CASE(test_permissions_equality_and_hashing) {
    ECF_NAME_THIS_TEST();

    const auto a = Permissions::parse_cli("alice:rw,bob:r");
    const auto b = Permissions::parse_cli("bob:r,alice:wr");
    const auto c = Permissions::parse_cli("alice:rw,bob:rs");
    const auto d = Permissions::parse_cli("alice:rw");

    BOOST_CHECK(a == b);
    BOOST_CHECK(a != c);
    BOOST_CHECK(a != d);
    BOOST_CHECK_EQUAL(std::hash<Permissions>{}(a), std::hash<Permissions>{}(b));

    std::unordered_set<Permissions> set{a, b, c, d};
    BOOST_CHECK_EQUAL(set.size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

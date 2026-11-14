// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ServerState.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;

namespace {

defs_ptr make_defs() {
    auto defs = Defs::create();
    defs->server_state().add_or_update_user_variables("U", "1");
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlops,ops:r"));
    defs->add_suite("s")->add_permissions(Permissions::parse_cli("ops:rx"));
    return defs;
}

// The cereal form of a server state holding a user variable, no server variable and no attribute, as written before the
// attribute existed (captured from the code before this change)
const char* server_state_fixture =
    R"({"11ServerState":{"cereal_class_version":0,"server_state_":2,"server_variables_":[],"user_variables_":[{"n_":"U","v_":"1"}]}})";

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_ServerPermissions)

///
/// @brief Verifies that the server state holds at most one attribute, which can be set, replaced and cleared.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_set_and_clear) {
    ECF_NAME_THIS_TEST();

    ServerState state;
    BOOST_CHECK(state.permissions() == nullptr);

    state.set_permissions(Permissions::parse_cli("alice:r"));
    BOOST_REQUIRE(state.permissions() != nullptr);
    BOOST_CHECK_EQUAL(state.permissions()->to_cli(), "alice:r");

    state.set_permissions(Permissions::parse_cli("bob:rw"));
    BOOST_CHECK_EQUAL(state.permissions()->to_cli(), "bob:rw");

    state.clear_permissions();
    BOOST_CHECK(state.permissions() == nullptr);
}

///
/// @brief Verifies that server states, and definitions, differing only in the attribute compare unequal.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_equality) {
    ECF_NAME_THIS_TEST();

    ServerState a;
    ServerState b;
    BOOST_CHECK(a == b);
    BOOST_CHECK(a.compare(b));

    b.set_permissions(Permissions::parse_cli("alice:r"));
    BOOST_CHECK(!(a == b));
    BOOST_CHECK(!(b == a));
    BOOST_CHECK(!a.compare(b));
    BOOST_CHECK(!b.compare(a));

    a.set_permissions(Permissions::parse_cli("alice:rw"));
    BOOST_CHECK(!(a == b));
    BOOST_CHECK(!a.compare(b));

    auto d1 = make_defs();
    auto d2 = make_defs();
    BOOST_CHECK(*d1 == *d2);
    d2->server_state().set_permissions(Permissions::parse_cli("admin:rwxlops"));
    BOOST_CHECK(!(*d1 == *d2));
}

///
/// @brief Verifies that the attribute is written in the state styles only, and read back from them.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_state_styles) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs();

    const auto defs_style = ecf::as_string(*defs, PrintStyle::DEFS);
    BOOST_CHECK_MESSAGE(defs_style.find("admin") == std::string::npos, defs_style);

    for (auto style : {PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        const auto text = ecf::as_string(*defs, style);
        BOOST_TEST_CONTEXT("style " << PrintStyle::to_string(style)) {
            auto copy = Defs::create();
            std::string error;
            std::string warning;
            BOOST_REQUIRE_MESSAGE(copy->restore_from_string(text, error, warning), error << "\n" << text);
            BOOST_REQUIRE(copy->server_state().permissions() != nullptr);
            BOOST_CHECK(*copy->server_state().permissions() == *defs->server_state().permissions());
            BOOST_REQUIRE(copy->findAbsNode("/s")->permissions() != nullptr);
            BOOST_CHECK_EQUAL(copy->findAbsNode("/s")->permissions()->to_cli(), "ops:rx");
        }
    }
}

///
/// @brief Verifies that definitions, as opposed to state files, cannot set the server attribute.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_refused_in_definitions) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    std::string error;
    std::string warning;
    BOOST_CHECK(!defs->restore_from_string(
        "permissions [{\"user\":\"alice\",\"permissions\":\"r\"}]\nsuite s\nendsuite\n", error, warning));
    BOOST_CHECK_MESSAGE(error.find("permissions") != std::string::npos, error);
    BOOST_CHECK(defs->server_state().permissions() == nullptr);
}

///
/// @brief Verifies that an invalid server attribute in a state file is rejected.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_invalid_in_state_file) {
    ECF_NAME_THIS_TEST();

    auto text     = ecf::as_string(*make_defs(), PrintStyle::MIGRATE);
    const auto at = text.find("\"rwxlops\"");
    BOOST_REQUIRE_MESSAGE(at != std::string::npos, text);
    text.replace(at, 9, "\"rwxlqs\"");

    auto copy = Defs::create();
    std::string error;
    std::string warning;
    BOOST_CHECK(!copy->restore_from_string(text, error, warning));
    BOOST_CHECK_MESSAGE(error.find("permissions") != std::string::npos, error);
}

///
/// @brief Verifies that a state file is restored again into the same definitions, its server attribute included.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_restored_twice) {
    ECF_NAME_THIS_TEST();

    const auto text = ecf::as_string(*make_defs(), PrintStyle::MIGRATE);
    auto copy       = Defs::create();
    for (int round = 1; round <= 2; ++round) {
        BOOST_TEST_CONTEXT("round " << round) {
            std::string error;
            std::string warning;
            BOOST_REQUIRE_MESSAGE(copy->restore_from_string(text, error, warning), error);
            BOOST_REQUIRE(copy->server_state().permissions() != nullptr);
            BOOST_CHECK_EQUAL(copy->server_state().permissions()->to_cli(),
                              make_defs()->server_state().permissions()->to_cli());
            BOOST_CHECK_EQUAL(copy->suites().size(), make_defs()->suites().size());
        }
    }
}

///
/// @brief Verifies that a state file holding the server attribute twice is rejected, naming the line.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_repeated_in_state_file) {
    ECF_NAME_THIS_TEST();

    auto text     = ecf::as_string(*make_defs(), PrintStyle::MIGRATE);
    const auto at = text.find("\npermissions ");
    BOOST_REQUIRE_MESSAGE(at != std::string::npos, text);
    const auto end  = text.find('\n', at + 1);
    const auto line = text.substr(at + 1, end - at); // the attribute, with its line break
    text.insert(end + 1, line);

    auto copy = Defs::create();
    std::string error;
    std::string warning;
    BOOST_CHECK(!copy->restore_from_string(text, error, warning));
    BOOST_CHECK_MESSAGE(error.find("only have one permissions attribute") != std::string::npos, error);
    BOOST_CHECK_MESSAGE(error.find("line") != std::string::npos || error.find("Line") != std::string::npos, error);
}

///
/// @brief Verifies that copies of definitions, including those built for client handles, carry the attribute.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_copies) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs();

    Defs copy(*defs);
    BOOST_REQUIRE(copy.server_state().permissions() != nullptr);
    BOOST_CHECK(*copy.server_state().permissions() == *defs->server_state().permissions());

    auto handle_defs = Defs::create();
    handle_defs->copy_defs_state_only(defs);
    BOOST_REQUIRE(handle_defs->server_state().permissions() != nullptr);
    BOOST_CHECK(*handle_defs->server_state().permissions() == *defs->server_state().permissions());

    auto without = Defs::create();
    handle_defs->copy_defs_state_only(without);
    BOOST_CHECK(handle_defs->server_state().permissions() == nullptr);
}

///
/// @brief Verifies the cereal form of the server state: unchanged without the attribute, lossless with it, last member.
///
BOOST_AUTO_TEST_CASE(test_server_permissions_cereal) {
    ECF_NAME_THIS_TEST();

    ServerState without;
    without.set_server_variables({}); // the defaults depend on the host and the version
    without.add_or_update_user_variables("U", "1");
    std::string data;
    ecf::save_as_string(data, without);
    BOOST_CHECK(nlohmann::ordered_json::parse(data).front() ==
                nlohmann::ordered_json::parse(server_state_fixture).front());

    ServerState restored_without;
    ecf::restore_from_string(server_state_fixture, restored_without);
    BOOST_CHECK(restored_without.permissions() == nullptr);
    BOOST_CHECK(restored_without == without);

    // Loading into a state that already holds an attribute leaves none, as in the data loaded
    ServerState reused;
    reused.set_permissions(Permissions::parse_cli("mallory:rwxlops"));
    ecf::restore_from_string(server_state_fixture, reused);
    BOOST_CHECK(reused.permissions() == nullptr);

    ServerState with = without;
    with.set_permissions(Permissions::parse_cli("alice:rs,bob:r"));
    ecf::save_as_string(data, with);
    const auto json = nlohmann::ordered_json::parse(data).front();
    BOOST_CHECK_EQUAL(std::prev(json.end()).key(), "permissions_");

    ServerState restored;
    ecf::restore_from_string(data, restored);
    BOOST_REQUIRE(restored.permissions() != nullptr);
    BOOST_CHECK(restored == with);

    auto corrupted = data;
    const auto at  = corrupted.find("\\\"rs\\\"");
    BOOST_REQUIRE_MESSAGE(at != std::string::npos, data);
    corrupted.replace(at, 6, "\\\"rq\\\"");
    ServerState rejected;
    BOOST_CHECK_THROW(ecf::restore_from_string(corrupted, rejected), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

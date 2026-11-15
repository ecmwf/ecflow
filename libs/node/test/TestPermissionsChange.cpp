// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Visibility.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;
using ecf::PermissionsChange;

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
/// @brief Returns the command-line form of the attribute at a path ("/" for the server), or "-" when there is none.
///
std::string at(const defs_ptr& defs, const std::string& path) {
    const Permissions* value =
        path == "/" ? defs->server_state().permissions() : defs->findAbsNode(path)->permissions();
    return value ? value->to_cli() : "-";
}

///
/// @brief Requests a node attribute (command-line form; empty to remove it) and commits, returning the error, if any.
///
std::string change(const defs_ptr& defs, const std::string& path, const std::string& value) {
    PermissionsChange change(*defs);
    if (path == "/") {
        change.set_server(Permissions::parse_cli(value));
    }
    else {
        change.set(*defs->findAbsNode(path),
                   value.empty() ? std::nullopt : std::optional<Permissions>(Permissions::parse_cli(value)));
    }
    try {
        change.commit("alice@host");
    }
    catch (const std::runtime_error& e) {
        return e.what();
    }
    return "";
}

///
/// @brief Returns the textual form of definitions, used to show that a rejected change leaves them unchanged.
///
std::string text_of(const defs_ptr& defs) {
    std::string text;
    defs->write_to_string(text, PrintStyle::MIGRATE);
    return text + (defs->server_state().permissions() ? defs->server_state().permissions()->to_cli() : "");
}

///
/// @brief Sets the process as a server for the duration of a test, so that change numbers advance.
///
struct ServerSide
{
    ServerSide() { Ecf::set_server(true); }
    ~ServerSide() { Ecf::set_server(false); }
    ServerSide(const ServerSide&)            = delete;
    ServerSide& operator=(const ServerSide&) = delete;
};

const std::string server_attribute = "alice:rwxop,bob:rwx,carol:r";

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_PermissionsChange)

///
/// @brief Verifies that a requested attribute may name only users listed above its node (D2.15, D2.18).
///
BOOST_AUTO_TEST_CASE(test_users_listed_above) {
    ECF_NAME_THIS_TEST();

    // At a suite, the users listed above are those of the server attribute
    auto defs = make(server_attribute, "suite s\n  family f\n    task t\n  endfamily\nendsuite\n");
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp,bob:rw"), "");
    BOOST_CHECK_EQUAL(at(defs, "/s"), "alice:rwxp,bob:rw");
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp,dave:r"),
                      "The Permissions attribute of /s names the user 'dave', who is not listed above it");
    BOOST_CHECK_EQUAL(at(defs, "/s"), "alice:rwxp,bob:rw");

    // Below the suite, the users holding effective roles at the parent (D2.18)
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "alice:rwxp,carol:r"),
                      "The Permissions attribute of /s/f names the user 'carol', who is not listed above it");
    BOOST_CHECK_EQUAL(at(defs, "/s/f"), "-");
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "alice:rwxp"), "");
    BOOST_CHECK_EQUAL(change(defs, "/s/f/t", "bob:r"),
                      "The Permissions attribute of /s/f/t names the user 'bob', who is not listed above it");
    BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "-");

    // Without an attribute on the suite, the server attribute lists the users of every node of the suite
    auto plain = make(server_attribute, "suite s\n  family f\n  endfamily\nendsuite\n");
    BOOST_CHECK_EQUAL(change(plain, "/s/f", "alice:rwxp,carol:r"), "");
}

///
/// @brief Verifies that below a suite, the users listed above a node are those holding effective roles at its parent
///        (D2.18): a sticky server entry lists its user everywhere, and an empty intersection lists nobody.
///
BOOST_AUTO_TEST_CASE(test_users_listed_by_effective_roles) {
    ECF_NAME_THIS_TEST();

    // alice, sticky at the server, holds roles in /s, which does not name her
    auto sticky =
        make("alice:rwxps,bob:rwxp", "suite s\n" + attribute("bob:rwxp") + "  family f\n  endfamily\nendsuite\n");
    BOOST_CHECK_EQUAL(change(sticky, "/s/f", "alice:r,bob:rwxp"), "");

    // bob holds r at /s, and nothing at /s/f, where w is intersected with r
    auto defs = make("admin:rwxops,bob:rwxp",
                     "suite s\n" + attribute("admin:rwxp,bob:r") + "  family f\n    task t\n  endfamily\nendsuite\n");
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "admin:rwxp,bob:w"), "");
    BOOST_CHECK_EQUAL(change(defs, "/s/f/t", "admin:rwxp,bob:r"),
                      "The Permissions attribute of /s/f/t names the user 'bob', who is not listed above it");

    // The cascade follows the effective roles: bob, left with nothing at /s/f, is removed from /s/f/t
    auto cascade = make("admin:rwxops,bob:rwxp",
                        "suite s\n" + attribute("admin:rwxp,bob:rw") + "  family f\n" + attribute("admin:rwxp,bob:r") +
                            "    task t\n" + attribute("admin:rwxp,bob:r") + "  endfamily\nendsuite\n");
    BOOST_CHECK_EQUAL(change(cascade, "/s", "admin:rwxp,bob:x"), "");
    BOOST_CHECK_EQUAL(at(cascade, "/s/f"), "admin:rwxp,bob:r");
    BOOST_CHECK_EQUAL(at(cascade, "/s/f/t"), "admin:rwxp");
}

///
/// @brief Verifies a change made of several requests: validated against the candidate as a whole, whatever the
///        order of the requests, and rejected as a whole, leaving attributes, change numbers, the generation of
///        visibilities and the log unchanged.
///
BOOST_AUTO_TEST_CASE(test_several_requests) {
    ECF_NAME_THIS_TEST();

    ServerSide server_side;
    ecf::test::scaffold::TestLog log("test_permissions_change_several.log");
    auto defs = make("alice:rwxop,bob:rwx",
                     "suite s\n" + attribute("alice:rwxp") + "  family f\n  endfamily\nendsuite\nsuite u\n" +
                         attribute("alice:rwxp") + "endsuite\n");

    // The child is requested first; bob is listed above it by the value requested for its parent
    {
        PermissionsChange pipeline(*defs);
        pipeline.set(*defs->findAbsNode("/s/f"), Permissions::parse_cli("alice:rp,bob:r"));
        pipeline.set(*defs->findAbsNode("/s"), Permissions::parse_cli("alice:rwxp,bob:rw"));
        BOOST_CHECK_NO_THROW(pipeline.commit("alice@host"));
    }
    BOOST_CHECK_EQUAL(at(defs, "/s"), "alice:rwxp,bob:rw");
    BOOST_CHECK_EQUAL(at(defs, "/s/f"), "alice:rp,bob:r");

    // The server attribute and a suite together: carol is listed at the server by the value requested for it
    const auto s_modify_no = defs->findSuite("s")->modify_change_no();
    const auto u_modify_no = defs->findSuite("u")->modify_change_no();
    {
        PermissionsChange pipeline(*defs);
        pipeline.set(*defs->findAbsNode("/u"), Permissions::parse_cli("alice:rwxp,carol:r"));
        pipeline.set_server(Permissions::parse_cli("alice:rwxop,bob:rwx,carol:r"));
        BOOST_CHECK_NO_THROW(pipeline.commit("alice@host"));
    }
    BOOST_CHECK_EQUAL(at(defs, "/"), "alice:rwxop,bob:rwx,carol:r");
    BOOST_CHECK_EQUAL(at(defs, "/u"), "alice:rwxp,carol:r");
    BOOST_CHECK_EQUAL(defs->findSuite("s")->modify_change_no(), s_modify_no);
    BOOST_CHECK_GT(defs->findSuite("u")->modify_change_no(), u_modify_no);

    // Two suites: both advance
    {
        const auto s_before = defs->findSuite("s")->modify_change_no();
        const auto u_before = defs->findSuite("u")->modify_change_no();
        PermissionsChange pipeline(*defs);
        pipeline.set(*defs->findAbsNode("/s"), Permissions::parse_cli("alice:rwxp,bob:rwx"));
        pipeline.set(*defs->findAbsNode("/u"), Permissions::parse_cli("alice:rwxp"));
        BOOST_CHECK_NO_THROW(pipeline.commit("alice@host"));
        BOOST_CHECK_GT(defs->findSuite("s")->modify_change_no(), s_before);
        BOOST_CHECK_GT(defs->findSuite("u")->modify_change_no(), u_before);
    }

    // A valid request followed by an invalid one: nothing changes
    { ecf::LogFlusher flusher; }
    const auto log_size   = fs::file_size(ecf::Log::instance()->path());
    const auto unchanged  = text_of(defs);
    const auto modify_no  = Ecf::modify_change_no();
    const auto state_no   = Ecf::state_change_no();
    const auto server_no  = defs->server_modify_change_no();
    const auto s_no       = defs->findSuite("s")->modify_change_no();
    const auto generation = ecf::Visibility::structure_generation();
    for (const bool server_first : {true, false}) {
        PermissionsChange pipeline(*defs);
        if (server_first) {
            pipeline.set_server(Permissions::parse_cli("alice:rwxop,bob:rwx,dave:r"));
        }
        pipeline.set(*defs->findAbsNode("/s"), Permissions::parse_cli("alice:rwxp"));
        pipeline.set(*defs->findAbsNode("/u"), Permissions::parse_cli("alice:rwxp,erin:r"));
        BOOST_CHECK_THROW(pipeline.commit("alice@host"), std::runtime_error);
    }
    BOOST_CHECK_EQUAL(text_of(defs), unchanged);
    BOOST_CHECK_EQUAL(Ecf::modify_change_no(), modify_no);
    BOOST_CHECK_EQUAL(Ecf::state_change_no(), state_no);
    BOOST_CHECK_EQUAL(defs->server_modify_change_no(), server_no);
    BOOST_CHECK_EQUAL(defs->findSuite("s")->modify_change_no(), s_no);
    BOOST_CHECK_EQUAL(ecf::Visibility::structure_generation(), generation);
    { ecf::LogFlusher flusher; }
    BOOST_CHECK_EQUAL(fs::file_size(ecf::Log::instance()->path()), log_size);
}

///
/// @brief Verifies that node attributes require a server attribute (D2.20), while removing one does not.
///
BOOST_AUTO_TEST_CASE(test_node_attributes_need_a_server_attribute) {
    ECF_NAME_THIS_TEST();

    auto defs = make("", "suite s\nendsuite\nsuite u\n" + attribute("alice:rwxp") + "endsuite\n");
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp"),
                      "The Permissions attribute of /s requires a server Permissions attribute");
    BOOST_CHECK_EQUAL(at(defs, "/s"), "-");

    BOOST_CHECK_EQUAL(change(defs, "/u", ""), "");
    BOOST_CHECK_EQUAL(at(defs, "/u"), "-");
}

///
/// @brief Verifies the cascade of a removed user (D2.17): entries removed below, and emptied attributes removed.
///
BOOST_AUTO_TEST_CASE(test_cascade_of_a_removed_user) {
    ECF_NAME_THIS_TEST();

    auto defs = make(server_attribute,
                     "suite s\n" + attribute("alice:rwxp,bob:rwxp") + "  family f\n" + attribute("alice:rp,bob:rwx") +
                         "    task t\n" + attribute("bob:r") + "  endfamily\n  family g\n" + attribute("bob:rwx") +
                         "  endfamily\nendsuite\n");

    PermissionsChange pipeline(*defs);
    pipeline.set(*defs->findAbsNode("/s"), Permissions::parse_cli("alice:rwxp"));
    pipeline.validate();

    // Validating changes nothing
    BOOST_CHECK_EQUAL(at(defs, "/s/f"), "alice:rp,bob:rwx");

    const auto& changes = pipeline.changes();
    BOOST_REQUIRE_EQUAL(changes.size(), 4U);
    BOOST_CHECK_EQUAL(changes[0].path, "/s");
    BOOST_CHECK(!changes[0].cascade);
    BOOST_CHECK_EQUAL(changes[1].path, "/s/f");
    BOOST_CHECK(changes[1].cascade);
    BOOST_CHECK_EQUAL(changes[1].after->to_cli(), "alice:rp");
    BOOST_CHECK_EQUAL(changes[2].path, "/s/f/t");
    BOOST_CHECK(!changes[2].after);
    BOOST_CHECK_EQUAL(changes[3].path, "/s/g");
    BOOST_CHECK(!changes[3].after);

    pipeline.commit("alice@host");
    BOOST_CHECK_EQUAL(at(defs, "/s"), "alice:rwxp");
    BOOST_CHECK_EQUAL(at(defs, "/s/f"), "alice:rp");
    BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "-");
    BOOST_CHECK_EQUAL(at(defs, "/s/g"), "-");
}

///
/// @brief Verifies that removing an attribute cascades below it, against the users listed above it.
///
BOOST_AUTO_TEST_CASE(test_cascade_of_a_removed_attribute) {
    ECF_NAME_THIS_TEST();

    auto defs = make(server_attribute,
                     "suite s\n" + attribute("alice:rwxp,bob:rwx") + "  family f\n" + attribute("alice:rwxp,bob:rwx") +
                         "    task t\n" + attribute("bob:rwx") + "  endfamily\nendsuite\n");

    // Removing the attribute of /s/f leaves /s above /s/f/t, which lists bob: nothing cascades
    BOOST_CHECK_EQUAL(change(defs, "/s/f", ""), "");
    BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "bob:rwx");

    // Updating the server attribute without bob removes bob from the suite and, emptied, the attribute of /s/f/t
    BOOST_CHECK_EQUAL(change(defs, "/", "alice:rwxop,carol:r"), "");
    BOOST_CHECK_EQUAL(at(defs, "/s"), "alice:rwxp");
    BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "-");
}

///
/// @brief Verifies the self-lockout check (D4.6) at the node changed, below it, and at the server.
///
BOOST_AUTO_TEST_CASE(test_self_lockout) {
    ECF_NAME_THIS_TEST();

    const std::string text = "suite s\n" + attribute("alice:rwxp,bob:rwxp") + "  family f\n" +
                             attribute("alice:r,bob:rwxp") + "    task t\n  endfamily\nendsuite\n" + "suite locked\n" +
                             attribute("bob:rwx") + "endsuite\n";
    auto defs            = make("alice:rwxop,bob:rwxp", text);
    const auto unchanged = text_of(defs);

    // At the node changed
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwx,bob:rwx"),
                      "The change would leave nobody able to change the Permissions attribute of /s");
    BOOST_CHECK_EQUAL(text_of(defs), unchanged);

    // Below it, through inheritance: /s/f keeps alice:r, and bob loses p by intersection
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp,bob:rwx"),
                      "The change would leave nobody able to change the Permissions attribute of /s/f");
    BOOST_CHECK_EQUAL(text_of(defs), unchanged);

    // Below it, through the cascade: removing bob leaves alice:r at /s/f
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp"),
                      "The change would leave nobody able to change the Permissions attribute of /s/f");
    BOOST_CHECK_EQUAL(text_of(defs), unchanged);

    // At the server
    BOOST_CHECK_EQUAL(change(defs, "/", "alice:rwxo,bob:rwx"),
                      "The change would leave nobody able to change the Permissions attribute of /");
    BOOST_CHECK_EQUAL(text_of(defs), unchanged);

    // A node already without any holder of p (/locked) does not block other changes
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "alice:rp,bob:rwxp"), "");
    BOOST_CHECK_EQUAL(change(defs, "/", "alice:rwxop,bob:rwxp,carol:r"), "");

    // A sticky server entry holding p keeps every node changeable
    auto sticky = make("alice:rwxops,bob:rwx", "suite s\n" + attribute("alice:rwxp,bob:rwx") + "endsuite\n");
    BOOST_CHECK_EQUAL(change(sticky, "/s", "bob:rwx"), "");
}

///
/// @brief Verifies that a commit is a modify change of the suites concerned, or of the server (D3.7).
///
BOOST_AUTO_TEST_CASE(test_commit_is_a_modify_change) {
    ECF_NAME_THIS_TEST();

    ServerSide server_side;
    auto defs    = make(server_attribute,
                     "suite s\n" + attribute("alice:rwxp,bob:rwx") + "  family f\n  endfamily\nendsuite\nsuite u\n" +
                         "endsuite\n");
    const auto s = defs->findSuite("s");
    const auto u = defs->findSuite("u");

    // A handle on /u only
    const auto handle      = defs->client_suite_mgr().create_client_suite(false, {"u"}, "carol");
    unsigned int state_no  = 0;
    unsigned int modify_no = 0;
    defs->client_suite_mgr().max_change_no(handle, state_no, modify_no);
    const auto handle_modify_no = modify_no;

    // A node change advances the modify number of its suite, not of the other suites
    auto modify_no_s = s->modify_change_no();
    auto modify_no_u = u->modify_change_no();
    auto generation  = ecf::Visibility::structure_generation();
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "alice:rwxp"), "");
    BOOST_CHECK_GT(s->modify_change_no(), modify_no_s);
    BOOST_CHECK_EQUAL(s->modify_change_no(), Ecf::modify_change_no());
    BOOST_CHECK_EQUAL(u->modify_change_no(), modify_no_u);
    BOOST_CHECK_GT(ecf::Visibility::structure_generation(), generation);

    // Deleting an attribute is recorded for the visibilities kept for later requests
    generation = ecf::Visibility::structure_generation();
    BOOST_CHECK_EQUAL(change(defs, "/s/f", ""), "");
    BOOST_CHECK_GT(ecf::Visibility::structure_generation(), generation);

    // The handle on /u does not see changes of /s
    defs->client_suite_mgr().max_change_no(handle, state_no, modify_no);
    BOOST_CHECK_EQUAL(modify_no, handle_modify_no);

    // A server change advances the modify number of the server, which every handle compares
    modify_no_s = s->modify_change_no();
    BOOST_CHECK_EQUAL(change(defs, "/", "alice:rwxop,bob:rwx,carol:rw"), "");
    BOOST_CHECK_EQUAL(defs->server_modify_change_no(), Ecf::modify_change_no());
    defs->client_suite_mgr().max_change_no(handle, state_no, modify_no);
    BOOST_CHECK_EQUAL(modify_no, Ecf::modify_change_no());
    BOOST_CHECK_EQUAL(s->modify_change_no(), modify_no_s);

    // A requested value equal to the current one changes nothing
    const auto before = Ecf::modify_change_no();
    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp,bob:rwx"), "");
    BOOST_CHECK_EQUAL(Ecf::modify_change_no(), before);
}

///
/// @brief Verifies the log records of a change: path, old and new values, cascade, and requester (R4.4).
///
BOOST_AUTO_TEST_CASE(test_log_records) {
    ECF_NAME_THIS_TEST();

    ecf::test::scaffold::TestLog log("test_permissions_change.log");
    auto defs = make(server_attribute,
                     "suite s\n" + attribute("alice:rwxp,bob:rwx") + "  family f\n" + attribute("bob:rwx") +
                         "  endfamily\nendsuite\n");

    BOOST_CHECK_EQUAL(change(defs, "/s", "alice:rwxp"), "");
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "alice:rp"), "");
    BOOST_CHECK_EQUAL(change(defs, "/s/f", "bob:r").empty(), false);

    { ecf::LogFlusher flusher; }
    std::ifstream file(ecf::Log::instance()->path());
    std::stringstream content;
    content << file.rdbuf();
    const std::vector<std::string> expected{
        "permissions /s alice:rwxp,bob:rwx -> alice:rwxp by alice@host",
        "permissions /s/f bob:rwx -> (none) (removal of users above) by alice@host",
        "permissions /s/f (none) -> alice:rp by alice@host",
    };
    std::string::size_type position = 0;
    for (const auto& line : expected) {
        const auto found = content.str().find(line, position);
        BOOST_CHECK_MESSAGE(found != std::string::npos, "missing, in order: " << line << "\n" << content.str());
        position = found == std::string::npos ? position : found;
    }
    // A rejected change writes no record
    BOOST_CHECK_EQUAL(content.str().find("-> bob:r "), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

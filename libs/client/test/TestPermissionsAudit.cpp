// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};
const Credentials carol{"carol", "carol-secret"};

/// The server attribute of every test: alice may only read at server level
const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}])";

/// The configuration setting selecting the audit-only mode
const std::string audit_only = R"("authorisation": {"audit_only": true})";

///
/// @brief Starts a server, in audit-only mode or not.
///
/// @param[in] audit True to select the audit-only mode
/// @param[in] attribute The server Permissions attribute, as its JSON form; none when empty (whitelist mode)
/// @return The server, stopped on destruction.
///
std::unique_ptr<PermissionsServer> start(bool audit, const std::string& attribute = server_attribute) {
    return std::make_unique<PermissionsServer>(admin,
                                               std::vector<Credentials>{alice, carol},
                                               attribute,
                                               std::chrono::seconds(30),
                                               std::nullopt,
                                               std::nullopt,
                                               false,
                                               audit ? audit_only : std::string{});
}

///
/// @brief Loads the suites a and b, each holding a task t; b is readable by admin only, when attributes are used.
///
/// @param[in] server The server
/// @param[in] attributes False to load no Permissions attribute (a server without a server attribute refuses them)
///
void load_suites(const PermissionsServer& server, bool attributes = true) {
    auto defs = Defs::create();
    defs->add_suite("a")->add_task("t");
    auto b = defs->add_suite("b");
    b->add_task("t");
    if (attributes) {
        b->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
    }
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

///
/// @brief Tells whether a node is suspended, as the administrator reads it.
///
/// @param[in] server The server
/// @param[in] path The path of the node
/// @return True when the node is suspended.
///
bool is_suspended(const PermissionsServer& server, const std::string& path) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    return client->defs()->findAbsNode(path)->isSuspended();
}

///
/// @brief Returns the lines of the server log containing the given text.
///
/// @param[in] server The server
/// @param[in] text The text searched
/// @return The lines, in the order of the log.
///
std::vector<std::string> log_lines(const PermissionsServer& server, const std::string& text) {
    std::ifstream in(server.home() / "server.log");
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        if (line.find(text) != std::string::npos) {
            lines.push_back(line);
        }
    }
    return lines;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsAudit)

///
/// @brief Verifies that a request denied in enforcing mode is executed, and logged, in audit-only mode (R12.3).
///
BOOST_AUTO_TEST_CASE(test_denied_request_executed_and_logged) {
    ECF_NAME_THIS_TEST();

    { // Enforcing: denied, and nothing changes
        auto server = start(false);
        load_suites(*server);
        auto client = server->client(alice);
        BOOST_CHECK(client->suspend("/a") != 0);
        BOOST_CHECK(client->server_reply().permission_denied());
        BOOST_CHECK(!is_suspended(*server, "/a"));
        BOOST_CHECK(log_lines(*server, "Audit only").empty());
    }
    { // Audit only: executed, and the would-be denial is logged as a warning, not as a refusal
        auto server = start(true);
        load_suites(*server);
        auto client = server->client(alice);
        BOOST_CHECK_MESSAGE(client->suspend("/a") == 0, client->errorMsg());
        BOOST_CHECK(is_suspended(*server, "/a"));

        const auto records = log_lines(*server, "Audit only");
        BOOST_REQUIRE_EQUAL(records.size(), 1U);
        BOOST_CHECK_MESSAGE(records.front().rfind("WAR:[", 0) == 0, records.front());
        BOOST_CHECK_MESSAGE(records.front().find("Audit only, executed although denied: Authorisation (user) failed, "
                                                 "due to: Insufficient permissions (role x required at /a) "
                                                 "[--suspend /a :alice@") != std::string::npos,
                            records.front());
        BOOST_CHECK(log_lines(*server, "Command not accepted").empty());

        // A request the attributes allow is not recorded
        BOOST_CHECK_EQUAL(client->getDefs(), 0);
        BOOST_CHECK_EQUAL(log_lines(*server, "Audit only").size(), 1U);
    }
}

///
/// @brief Verifies that, in audit-only mode, each command of a group is executed and its would-be denial logged.
///
BOOST_AUTO_TEST_CASE(test_group_in_audit_only_mode) {
    ECF_NAME_THIS_TEST();

    auto server = start(true);
    load_suites(*server);
    auto client = server->client(alice);
    BOOST_CHECK_MESSAGE(client->group("suspend /a; suspend /a/t") == 0, client->errorMsg());
    BOOST_CHECK(is_suspended(*server, "/a"));
    BOOST_CHECK(is_suspended(*server, "/a/t"));
    BOOST_CHECK_EQUAL(log_lines(*server, "Audit only").size(), 2U);
}

///
/// @brief Verifies that the audit-only mode leaves the filtering of content unchanged: unreadable suites stay hidden.
///
BOOST_AUTO_TEST_CASE(test_filtering_unchanged) {
    ECF_NAME_THIS_TEST();

    auto server = start(true);
    load_suites(*server);
    auto client = server->client(alice);
    BOOST_REQUIRE_EQUAL(client->getDefs(), 0);
    BOOST_CHECK(client->defs()->findAbsNode("/a"));
    BOOST_CHECK(!client->defs()->findAbsNode("/b"));

    // A readable node is sent
    BOOST_CHECK_MESSAGE(client->invoke("--get=/a") == 0, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->query("state", "/a", "") == 0, client->errorMsg());
}

///
/// @brief Verifies that, in audit-only mode, a request lacking the role r is refused as in Permissions mode, alone or
///        within a group, so that no content of an unreadable node is returned.
///
BOOST_AUTO_TEST_CASE(test_reads_enforced) {
    ECF_NAME_THIS_TEST();

    // carol holds x at server level, without r
    auto server = start(true,
                        R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"},)"
                        R"( {"user": "carol", "permissions": "x"}])");
    load_suites(*server);

    // Content to protect: the script of /b/t, and an entry of the edit history of /b/t
    fs::create_directories(server->home() / "b");
    std::ofstream(server->home() / "b" / "t.ecf") << "echo secret-script\n";
    auto administrator = server->administrator();
    BOOST_REQUIRE_MESSAGE(administrator->alter("/b/t", "add", "variable", "SECRET", "secret-variable") == 0,
                          administrator->errorMsg());

    // Each read of /b/t is refused: alice holds r at server level, but nothing at /b
    auto client        = server->client(alice);
    const auto refused = [&](const std::string& request, int result, const std::string& target) {
        BOOST_TEST_CONTEXT(request) {
            BOOST_CHECK(result != 0);
            BOOST_CHECK(client->server_reply().permission_denied());
            BOOST_CHECK_MESSAGE(client->errorMsg().find("role r required at " + target) != std::string::npos,
                                client->errorMsg());
            BOOST_CHECK_MESSAGE(client->get_string().find("secret") == std::string::npos, client->get_string());
        }
    };
    refused("--file", client->file("/b/t", "script"), "/b/t");
    refused("--why", client->invoke("--why=/b/t"), "/b/t");
    refused("--check", client->check("/b/t"), "/b/t");
    refused("--edit_script edit", client->edit_script_edit("/b/t"), "/b/t");
    refused("--edit_script pre_process", client->edit_script_preprocess("/b/t"), "/b/t");
    for (const std::string option : {"--get", "--get_state", "--migrate"}) {
        for (const std::string path : {"/b", "/b/t"}) {
            refused(option + "=" + path, client->invoke(option + "=" + path), path);
            BOOST_CHECK(!client->server_reply().client_node());
        }
    }
    refused("--query", client->query("state", "/b/t", ""), "/b/t");
    refused("--permissions read", client->permissions("read", "/b", "", false), "/b");
    refused("--permissions effective", client->permissions("effective", "/b", "", false), "/b");

    // The edit history is read at server level (D5.28): refused to carol, who lacks r there
    client = server->client(carol);
    refused("--edit_history", client->edit_history("/b/t"), "/");

    // Within a group, the read is refused and the other commands are executed (and audited)
    client = server->client(alice);
    BOOST_CHECK(client->group("why=/b/t; suspend /a") != 0);
    BOOST_CHECK(client->server_reply().permission_denied());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("role r required at /b/t") != std::string::npos, client->errorMsg());
    BOOST_CHECK(is_suspended(*server, "/a"));

    // Refused reads are logged as refusals, never as audited; only the suspend of the group is audited
    BOOST_CHECK_EQUAL(log_lines(*server, "Command not accepted").size(), 16U);
    const auto audited = log_lines(*server, "Audit only");
    BOOST_REQUIRE_EQUAL(audited.size(), 1U);
    BOOST_CHECK_MESSAGE(audited.front().find("[--suspend /a :alice@") != std::string::npos, audited.front());
}

///
/// @brief Verifies that, in audit-only mode, a request lacking only roles other than r (here w or x) is executed and
///        logged, also at a node the requester cannot read.
///
BOOST_AUTO_TEST_CASE(test_other_roles_audited) {
    ECF_NAME_THIS_TEST();

    auto server = start(true);
    load_suites(*server);
    auto client = server->client(alice);

    // w lacking at /a
    BOOST_CHECK_MESSAGE(client->alter("/a", "add", "variable", "V", "1") == 0, client->errorMsg());
    // x lacking at /b, which alice cannot read either: --suspend requires only x, and returns no content
    BOOST_CHECK_MESSAGE(client->suspend("/b") == 0, client->errorMsg());
    BOOST_CHECK(is_suspended(*server, "/b"));

    const auto audited = log_lines(*server, "Audit only");
    BOOST_REQUIRE_EQUAL(audited.size(), 2U);
    BOOST_CHECK_MESSAGE(audited[0].find("(role w required at /a)") != std::string::npos, audited[0]);
    BOOST_CHECK_MESSAGE(audited[1].find("(role x required at /b)") != std::string::npos, audited[1]);
    BOOST_CHECK(log_lines(*server, "Command not accepted").empty());

    auto administrator = server->administrator();
    BOOST_REQUIRE_EQUAL(administrator->getDefs(), 0);
    BOOST_CHECK_EQUAL(administrator->defs()->findAbsNode("/a")->findVariable("V").value(), "1");
}

///
/// @brief Verifies that the start-up log names the audit-only mode, and that it applies to Permissions mode only.
///
BOOST_AUTO_TEST_CASE(test_mode_logged_and_whitelist_unaffected) {
    ECF_NAME_THIS_TEST();

    {
        auto server = start(true);
        BOOST_CHECK_EQUAL(log_lines(*server, "  Mode = permissions (audit only)").size(), 1U);
    }
    { // Without a server attribute, the whitelist is enforced
        auto server = start(true, "");
        std::ofstream(server->home() / ecf::Host{}.prefix_host_and_port(server->port(), "ecf.lists"))
            << "4.5.0\nadmin\n-alice\n";
        auto administrator = server->administrator();
        BOOST_REQUIRE_MESSAGE(administrator->reloadwsfile() == 0, administrator->errorMsg());
        load_suites(*server, false);
        auto client = server->client(alice);
        BOOST_CHECK(client->suspend("/a") != 0);
        BOOST_CHECK(client->server_reply().permission_denied());
        BOOST_CHECK(!is_suspended(*server, "/a"));
        BOOST_CHECK(log_lines(*server, "Audit only").empty());
        BOOST_CHECK_EQUAL(log_lines(*server, "  Mode = whitelist").size() +
                              log_lines(*server, "  Mode = unrestricted").size(),
                          log_lines(*server, "  Mode = ").size());
    }
}

///
/// @brief Verifies that, in audit-only mode, authentication and the validation of changes are still enforced.
///
BOOST_AUTO_TEST_CASE(test_authentication_and_validation_enforced) {
    ECF_NAME_THIS_TEST();

    auto server = start(true);
    load_suites(*server);

    // A wrong password: refused, nothing executed, nothing audited
    auto impostor = server->client(Credentials{"alice", "wrong-secret"});
    BOOST_CHECK(impostor->suspend("/a") != 0);
    BOOST_CHECK(!is_suspended(*server, "/a"));
    BOOST_CHECK(log_lines(*server, "Audit only").empty());

    // A change leaving nobody with p (D4.6): its authorisation is audited, its validation refuses it
    auto client = server->client(alice);
    BOOST_CHECK(client->permissions("update", "/", "alice:r", false) != 0);
    BOOST_CHECK_EQUAL(log_lines(*server, "Audit only").size(), 1U);
    auto administrator = server->administrator();
    BOOST_REQUIRE_EQUAL(administrator->permissions("read", "/", "", false), 0);
    BOOST_CHECK_EQUAL(administrator->get_string(), "admin:rwxlops,alice:r");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

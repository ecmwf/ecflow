// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};

/// The server attribute of every test: alice may only read at server level
const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}])";

///
/// @brief Starts a server with the server attribute above, with the given further settings.
///
/// @param[in] settings Further members of `/ecflow/server`, as JSON text; none when empty
/// @param[in] attribute The server Permissions attribute, as its JSON form; none when empty
/// @return The server, stopped on destruction.
///
std::unique_ptr<PermissionsServer> start(const std::string& settings  = {},
                                         const std::string& attribute = server_attribute) {
    return std::make_unique<PermissionsServer>(admin,
                                               std::vector<Credentials>{alice},
                                               attribute,
                                               std::chrono::seconds(30),
                                               std::nullopt,
                                               std::nullopt,
                                               false,
                                               settings);
}

///
/// @brief Loads the suites a and b; b is readable by admin only, when attributes are used.
///
/// @param[in] server The server
/// @param[in] attributes False to load no Permissions attribute (a server without a server attribute refuses them)
///
void load_suites(const PermissionsServer& server, bool attributes = true) {
    auto defs = Defs::create();
    defs->add_suite("a")->add_task("t");
    auto b = defs->add_suite("b");
    if (attributes) {
        b->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
    }
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

///
/// @brief Returns the statistics of the server, as the administrator reads them.
///
/// @param[in] server The server
/// @return The statistics, as `--stats_server` returns them.
///
Stats stats_of(const PermissionsServer& server) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->stats_server() == 0, client->errorMsg());
    return client->server_reply().stats();
}

///
/// @brief Returns the report of `--stats`, as the administrator reads it.
///
/// @param[in] server The server
/// @return The report, as printed by `ecflow_client --stats`.
///
std::string report_of(const PermissionsServer& server) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->stats() == 0, client->errorMsg());
    return client->get_string();
}

///
/// @brief Returns the count of a command, or 0 when the command is not counted.
///
/// @param[in] counts The counts, per command
/// @param[in] command The option naming the command, e.g. "--suspend"
/// @return The count.
///
unsigned int count(const std::map<std::string, unsigned int>& counts, const std::string& command) {
    auto found = counts.find(command);
    return found == counts.end() ? 0U : found->second;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsStats)

///
/// @brief Verifies that --stats reports the mode and the denials per command, including those within a group (R12.5).
///
BOOST_AUTO_TEST_CASE(test_mode_and_denials_per_command) {
    ECF_NAME_THIS_TEST();

    auto server = start();
    load_suites(*server);
    BOOST_CHECK_EQUAL(stats_of(*server).authorisation_mode_, "permissions");
    BOOST_CHECK(stats_of(*server).denials_.empty());

    auto client = server->client(alice);
    BOOST_CHECK(client->suspend("/a") != 0);
    BOOST_CHECK(client->suspend("/a") != 0);
    BOOST_CHECK(client->resume("/a") != 0);
    BOOST_CHECK(client->group("suspend /a; get") != 0);

    const auto stats = stats_of(*server);
    BOOST_CHECK_EQUAL(count(stats.denials_, "--suspend"), 3U);
    BOOST_CHECK_EQUAL(count(stats.denials_, "--resume"), 1U);
    BOOST_CHECK_EQUAL(count(stats.denials_, "--get"), 0U);
    BOOST_CHECK(stats.audited_.empty());

    // The report of --stats, as an operator reads it
    auto report = report_of(*server);
    BOOST_CHECK_MESSAGE(report.find("Authorisation\n") != std::string::npos, report);
    BOOST_CHECK_MESSAGE(report.find("  Mode                            permissions\n") != std::string::npos, report);
    BOOST_CHECK_MESSAGE(report.find("  Denied --suspend                3\n") != std::string::npos, report);
    BOOST_CHECK_MESSAGE(report.find("  Denied --resume                 1\n") != std::string::npos, report);

    // --stats_reset clears the counters
    auto administrator = server->administrator();
    BOOST_REQUIRE_EQUAL(administrator->stats_reset(), 0);
    BOOST_CHECK(stats_of(*server).denials_.empty());
    report = report_of(*server);
    BOOST_CHECK_MESSAGE(report.find("Denied") == std::string::npos, report);
    BOOST_CHECK_MESSAGE(report.find("  Mode                            permissions\n") != std::string::npos, report);
}

///
/// @brief Verifies that --stats reports the requests executed although denied, in audit-only mode, and counts the
///        reads it refuses as denied.
///
BOOST_AUTO_TEST_CASE(test_audited_requests) {
    ECF_NAME_THIS_TEST();

    auto server = start(R"("authorisation": {"audit_only": true})");
    load_suites(*server);
    auto client = server->client(alice);
    BOOST_CHECK_EQUAL(client->suspend("/a"), 0);
    BOOST_CHECK_EQUAL(client->group("suspend /a; resume /a"), 0);

    const auto stats = stats_of(*server);
    BOOST_CHECK_EQUAL(stats.authorisation_mode_, "permissions (audit only)");
    BOOST_CHECK_EQUAL(count(stats.audited_, "--suspend"), 2U);
    BOOST_CHECK_EQUAL(count(stats.audited_, "--resume"), 1U);
    const auto report = report_of(*server);
    BOOST_CHECK_MESSAGE(report.find("  Audited --suspend               2\n") != std::string::npos, report);
    BOOST_CHECK(stats.denials_.empty());

    // Reads of a node alice cannot read are refused, alone or within a group, and counted as denied, not audited
    BOOST_CHECK(client->invoke("--why=/b") != 0);
    BOOST_CHECK(client->group("why=/b; resume /a") != 0);
    const auto after = stats_of(*server);
    BOOST_CHECK_EQUAL(count(after.denials_, "--why"), 2U);
    BOOST_CHECK_EQUAL(count(after.audited_, "--why"), 0U);
    BOOST_CHECK_EQUAL(count(after.audited_, "--resume"), 2U);
    const auto refused = report_of(*server);
    BOOST_CHECK_MESSAGE(refused.find("  Denied --why                    2\n") != std::string::npos, refused);
}

///
/// @brief Verifies that --stats reports the cost of the content filtered for each requester.
///
BOOST_AUTO_TEST_CASE(test_cost_of_filtered_content) {
    ECF_NAME_THIS_TEST();

    auto server = start();
    load_suites(*server);
    const auto before = stats_of(*server);

    auto client = server->client(alice);
    BOOST_REQUIRE_EQUAL(client->getDefs(), 0);
    BOOST_CHECK(!client->defs()->findAbsNode("/b"));
    BOOST_REQUIRE_EQUAL(client->sync_local(), 0);

    const auto after = stats_of(*server);
    BOOST_CHECK_GE(after.filtered_replies_, before.filtered_replies_ + 2);
    BOOST_CHECK_GE(after.filtering_time_us_, before.filtering_time_us_);
    BOOST_CHECK_GE(after.readable_sets_, 1U);
}

///
/// @brief Verifies the mode and the denials reported without a server attribute: unrestricted, then whitelist, where a
///        refused group counts as --group.
///
BOOST_AUTO_TEST_CASE(test_whitelist_and_unrestricted_modes) {
    ECF_NAME_THIS_TEST();

    auto server = start({}, "");
    BOOST_CHECK_EQUAL(stats_of(*server).authorisation_mode_, "unrestricted");

    std::ofstream(server->home() / ecf::Host{}.prefix_host_and_port(server->port(), "ecf.lists"))
        << "4.5.0\nadmin\n-alice\n";
    auto administrator = server->administrator();
    BOOST_REQUIRE_MESSAGE(administrator->reloadwsfile() == 0, administrator->errorMsg());
    load_suites(*server, false);
    auto client = server->client(alice);
    BOOST_CHECK(client->suspend("/a") != 0);
    BOOST_CHECK(client->group("suspend /a; resume /a") != 0);

    const auto stats = stats_of(*server);
    BOOST_CHECK_EQUAL(stats.authorisation_mode_, "whitelist");
    BOOST_CHECK_EQUAL(count(stats.denials_, "--suspend"), 1U);
    BOOST_CHECK_EQUAL(count(stats.denials_, "--group"), 1U);
    BOOST_CHECK_EQUAL(count(stats.denials_, "--resume"), 0U);
}

///
/// @brief Verifies that a plug denied by the receiving server counts as a denial of --plug on the sending server.
///
BOOST_AUTO_TEST_CASE(test_plug_denied_by_receiver) {
    ECF_NAME_THIS_TEST();

    auto sender =
        start({}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "rwxp"}])");
    {
        auto defs = Defs::create();
        defs->add_suite("s")->add_task("t");
        auto client = sender->administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    // alice may only read on the receiving server
    auto receiver = start({}, server_attribute);
    {
        auto defs = Defs::create();
        defs->add_suite("d");
        auto client = receiver->administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }

    auto client = sender->client(alice);
    BOOST_CHECK(client->plug("/s/t", "localhost:" + receiver->port() + "/d") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("denied the move") != std::string::npos, client->errorMsg());
    BOOST_CHECK_EQUAL(count(stats_of(*sender).denials_, "--plug"), 1U);
    BOOST_CHECK_EQUAL(count(stats_of(*receiver).denials_, "--move"), 1U);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};
const Credentials carol{"carol", "carol-secret"};

///
/// @brief Writes the white list of the server, and reloads it.
///
/// @return The status of the reload.
///
int reload_whitelist(const PermissionsServer& server, const std::string& content) {
    std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists")) << content;
    return server.administrator()->reloadwsfile();
}

///
/// @brief Loads the suites a, b and c, each holding a task t.
///
void load_suites(const PermissionsServer& server, const std::string& on_b = {}) {
    auto defs = Defs::create();
    for (const auto* name : {"a", "b", "c"}) {
        auto suite = defs->add_suite(name);
        suite->add_task("t");
        if (on_b.size() && std::string(name) == "b") {
            suite->add_permissions(ecf::Permissions::parse_cli(on_b));
        }
    }
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

bool is_suspended(const PermissionsServer& server, const std::string& path) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    return client->defs()->findAbsNode(path)->isSuspended();
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsAuthorisation)

///
/// @brief Verifies the mode selected by a running server: the server attribute, else the white list unless ignored.
///
BOOST_AUTO_TEST_CASE(test_mode_selection) {
    ECF_NAME_THIS_TEST();

    const std::string whitelist = "4.5.0\nadmin\nalice\n"; // alice may read and write

    { // White list, no server attribute: the white list applies
        PermissionsServer server(admin, {alice}, "");
        BOOST_REQUIRE_EQUAL(reload_whitelist(server, "4.5.0\nadmin\n-alice\n"), 0);
        load_suites(server);
        BOOST_CHECK(server.client(alice)->suspend("/a") != 0);
        BOOST_REQUIRE_EQUAL(reload_whitelist(server, whitelist), 0);
        BOOST_CHECK_EQUAL(server.client(alice)->suspend("/a"), 0);
    }
    { // Server attribute: the white list is not used, alice has r only
        PermissionsServer server(
            admin, {alice}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}])");
        BOOST_REQUIRE_EQUAL(reload_whitelist(server, whitelist), 0);
        load_suites(server);
        auto client = server.client(alice);
        BOOST_CHECK(client->suspend("/a") != 0);
        BOOST_CHECK(client->server_reply().permission_denied());
        BOOST_CHECK_EQUAL(client->getDefs(), 0);
    }
    { // White list ignored by the configuration: no restriction
        PermissionsServer server(admin,
                                 {alice},
                                 "",
                                 std::chrono::seconds(30),
                                 std::nullopt,
                                 std::nullopt,
                                 false,
                                 R"("whitelist": {"ignore": true})");
        BOOST_CHECK(reload_whitelist(server, "4.5.0\nadmin\n-alice\n") != 0);
        load_suites(server);
        BOOST_CHECK_EQUAL(server.client(alice)->suspend("/a"), 0);
    }
}

///
/// @brief Verifies that, in Permissions mode, --ping is allowed to every user, and nothing else to an unlisted user.
///
BOOST_AUTO_TEST_CASE(test_ping_and_unlisted_user) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, carol}, R"([{"user": "admin", "permissions": "rwxlops"}])");
    load_suites(server);
    auto client = server.client(carol);
    BOOST_CHECK_EQUAL(client->pingServer(), 0);
    BOOST_CHECK(client->getDefs() != 0);
    BOOST_CHECK(client->server_reply().permission_denied());
}

///
/// @brief Verifies that, in Permissions mode, each sub-command of a group is authorised just before it runs.
///
BOOST_AUTO_TEST_CASE(test_group_per_sub_command) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(
        admin, {alice}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "rw"}])");
    load_suites(server, "admin:rw,alice:r");

    auto client = server.client(alice);
    client->set_throw_on_error(true);
    BOOST_CHECK_THROW((void)client->group("suspend /a; suspend /b; suspend /c"), ecf::PermissionDenied);

    BOOST_CHECK(is_suspended(server, "/a"));
    BOOST_CHECK(!is_suspended(server, "/b"));
    BOOST_CHECK(is_suspended(server, "/c"));
}

///
/// @brief Verifies that a denied --terminate, alone or in a group, leaves the server running, over TCP and HTTP, in
///        Permissions and white-list modes.
///
BOOST_AUTO_TEST_CASE(test_denied_terminate_keeps_the_server) {
    ECF_NAME_THIS_TEST();

    const std::string attribute =
        R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}])";
    for (const auto* settings : {"", R"("http": true)"}) {
        BOOST_TEST_CONTEXT("settings: " << settings) {
            PermissionsServer server(
                admin, {alice}, attribute, std::chrono::seconds(30), std::nullopt, std::nullopt, false, settings);
            auto client = server.client(alice);
            BOOST_CHECK(client->terminateServer() != 0);
            BOOST_CHECK(client->server_reply().permission_denied());
            BOOST_CHECK_EQUAL(server.administrator()->pingServer(), 0);
            BOOST_CHECK(client->group("ping; terminate=yes") != 0);
            BOOST_CHECK_EQUAL(server.administrator()->pingServer(), 0);
            BOOST_CHECK(server.running());
        }
    }

    { // White-list mode
        PermissionsServer server(admin, {alice}, "");
        BOOST_REQUIRE_EQUAL(reload_whitelist(server, "4.5.0\nadmin\n-alice\n"), 0);
        BOOST_CHECK(server.client(alice)->terminateServer() != 0);
        BOOST_CHECK_EQUAL(server.administrator()->pingServer(), 0);
        BOOST_CHECK(server.running());
    }
}

namespace {

///
/// @brief Returns the state of the nodes of the scheduling scenario, and the value of its limit.
///
std::string snapshot(const PermissionsServer& server) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    const auto defs = client->defs();
    std::string text;
    for (const auto* path : {"/hidden/h", "/s/t1", "/s/t2"}) {
        text += std::string(path) + "=" + NState::toString(defs->findAbsNode(path)->state()) + " ";
    }
    return text + "L=" + std::to_string(defs->findSuite("s")->find_limit("L")->value());
}

///
/// @brief Sends the task requests of a job: init, then complete.
///
void run_job(const PermissionsServer& server, const std::string& path) {
    // The job file holds the password of the job, as its script prints it
    std::string password;
    std::ifstream(server.home() / (path.substr(1) + ".job1")) >> password;
    BOOST_REQUIRE_MESSAGE(!password.empty(), "no password in the job of " << path);

    // The task requests carry the identity of the job, which no Permissions attribute lists
    ClientInvoker job(std::string(ecf::string_constants::localhost), server.port());
    job.set_throw_on_error(true);
    job.set_connection_attempts(1);
    job.set_child_path(path);
    job.set_child_password(password);
    job.set_child_pid("4242");
    job.set_child_try_no(1);
    job.set_child_timeout(10);
    job.child_init();
    job.child_complete();
}

///
/// @brief Runs the scheduling scenario: two tasks in a limit of 1, triggered by a task of another suite.
///
/// @param[in] attributes When true, the server and both suites carry Permissions attributes, which grant nothing to
///            the job, and make the trigger unreadable to every user but the administrator
/// @return The snapshots taken along the scenario.
///
std::vector<std::string> run_scheduling_scenario(bool attributes) {
    PermissionsServer server(admin, {alice}, attributes ? R"([{"user": "admin", "permissions": "rwxlops"}])" : "");

    auto defs   = Defs::create();
    auto hidden = defs->add_suite("hidden");
    hidden->add_task("h");
    auto suite = defs->add_suite("s");
    suite->addLimit(Limit("L", 1));
    for (const auto* name : {"t1", "t2"}) {
        auto task = suite->add_task(name);
        task->addInLimit(InLimit("L", "/s"));
        task->add_trigger("/hidden/h == complete");
    }
    for (const auto& s : defs->suites()) {
        s->addVariable(Variable("ECF_HOME", server.home().string()));
        s->addVariable(Variable("ECF_JOB_CMD", "true"));
        if (attributes) {
            s->add_permissions(ecf::Permissions::parse_cli("admin:rwx"));
        }
        fs::create_directories(server.home() / s->name());
    }
    for (const auto* script : {"hidden/h.ecf", "s/t1.ecf", "s/t2.ecf"}) {
        std::ofstream(server.home() / script) << "%ECF_PASS%\n";
    }

    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->restartServer() == 0, client->errorMsg()); // the server starts halted
    BOOST_REQUIRE_MESSAGE(client->begin_all_suites() == 0, client->errorMsg());

    std::vector<std::string> snapshots{snapshot(server)};
    BOOST_REQUIRE_MESSAGE(client->force("/hidden/h", "complete") == 0, client->errorMsg());
    snapshots.push_back(snapshot(server)); // the trigger holds: one task submitted, within the limit
    run_job(server, "/s/t1");
    // Task requests do not generate jobs: the next scheduling cycle does, which --job_gen runs at once
    BOOST_REQUIRE_MESSAGE(client->job_gen("") == 0, client->errorMsg());
    snapshots.push_back(snapshot(server)); // the limit released: the other task submitted
    run_job(server, "/s/t2");
    snapshots.push_back(snapshot(server));
    return snapshots;
}

} // namespace

///
/// @brief Verifies that Permissions affect neither task requests, nor limits, nor dependencies, nor job generation and
///        submission (R5.8).
///
BOOST_AUTO_TEST_CASE(test_scheduling_unaffected) {
    ECF_NAME_THIS_TEST();

    const auto without = run_scheduling_scenario(false);
    const std::vector<std::string> expected{
        "/hidden/h=submitted /s/t1=queued /s/t2=queued L=0",
        "/hidden/h=complete /s/t1=submitted /s/t2=queued L=1",
        "/hidden/h=complete /s/t1=complete /s/t2=submitted L=1",
        "/hidden/h=complete /s/t1=complete /s/t2=complete L=0",
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(without.begin(), without.end(), expected.begin(), expected.end());

    const auto with = run_scheduling_scenario(true);
    BOOST_CHECK_EQUAL_COLLECTIONS(with.begin(), with.end(), without.begin(), without.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

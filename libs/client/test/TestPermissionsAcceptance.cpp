// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The acceptance scenarios of the authorisation (A7.3, and the authorisation and scheduling of A7.4; its filtering
/// assertions belong to the filtering of replies).
///

namespace {

const Credentials admin{"admin", "admin-secret"};

std::string state_of(const PermissionsServer& server, const std::string& path) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    return NState::toString(client->defs()->findAbsNode(path)->state());
}

std::vector<std::string> denied_records(const PermissionsServer& server) {
    std::ifstream log(server.home() / "server.log");
    std::vector<std::string> records;
    for (std::string line; std::getline(log, line);) {
        if (line.find(" DENIED ") != std::string::npos) {
            records.push_back(line);
        }
    }
    return records;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsAcceptance)

///
/// @brief A7.3 Denied operation: an operator with r at server level and r, but not x, on /sB requeues the aborted
///        task /sB/t; the request fails with "permission denied" (C++ client and command-line client), the task stays
///        aborted, and one log record holds identity, command, path and reason.
///
BOOST_AUTO_TEST_CASE(test_a7_3_denied_operation) {
    ECF_NAME_THIS_TEST();

    const Credentials operator_o{"operator", "operator-secret"};
    PermissionsServer server(
        admin,
        {operator_o},
        R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "operator", "permissions": "r"}])");
    {
        auto defs  = Defs::create();
        auto suite = defs->add_suite("sB");
        suite->add_task("t");
        suite->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,operator:r"));
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
        BOOST_REQUIRE_MESSAGE(client->force("/sB/t", "aborted") == 0, client->errorMsg());
    }
    BOOST_REQUIRE_EQUAL(state_of(server, "/sB/t"), "aborted");

    // The C++ client
    auto client = server.client(operator_o);
    client->set_throw_on_error(true);
    BOOST_CHECK_THROW((void)client->requeue("/sB/t"), ecf::PermissionDenied);
    BOOST_CHECK(client->connection_diagnosis().failure == ecf::ConnectionFailure::PermissionDenied);
    BOOST_CHECK_EQUAL(state_of(server, "/sB/t"), "aborted");

    // One record, with identity, command, path and reason
    {
        const auto records = denied_records(server);
        BOOST_REQUIRE_EQUAL(records.size(), 1U);
        for (const auto* field : {"user='operator'", "command='--requeue'", "/sB/t", "reason='"}) {
            BOOST_CHECK_MESSAGE(records.front().find(field) != std::string::npos,
                                "missing " << field << " in " << records.front());
        }
    }

    // The command-line client: exit status 3
    {
        using namespace ecf::test::scaffold;
        const auto passwords = server.directory() / "operator.custom_passwd";
        std::ofstream(passwords) << "4.5.0\noperator localhost " << server.port() << " operator-secret\n";
        WithTestEnvironmentVariable host("ECF_HOST", "localhost");
        WithTestEnvironmentVariable port("ECF_PORT", server.port());
        WithTestEnvironmentVariable user("ECF_USER", "operator");
        WithTestEnvironmentVariable password_file("ECF_CUSTOM_PASSWD", passwords.string());
        Process requeue(fs::path(ecf::File::find_ecf_client_path()), {"--requeue=/sB/t"}, server.directory());
        BOOST_CHECK_EQUAL(requeue.wait(), 3);
        BOOST_CHECK_MESSAGE(requeue.read_stderr().find("Permission denied: --requeue") != std::string::npos,
                            requeue.read_stderr());
    }
    BOOST_CHECK_EQUAL(state_of(server, "/sB/t"), "aborted");
}

///
/// @brief A7.4 Trigger across teams (authorisation and scheduling): B, with r at server level and w on /sB/t but no r
///        on /sA, adds to /sB/t a trigger on /sA/t; the change succeeds, and /sB/t is submitted once /sA/t completes.
///
BOOST_AUTO_TEST_CASE(test_a7_4_trigger_across_teams) {
    ECF_NAME_THIS_TEST();

    const Credentials b{"teamB", "b-secret"};
    PermissionsServer server(
        admin, {b}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "teamB", "permissions": "r"}])");
    {
        auto defs = Defs::create();
        auto sa   = defs->add_suite("sA");
        sa->add_task("t");
        sa->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
        auto sb = defs->add_suite("sB");
        sb->add_task("t");
        sb->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,teamB:rw"));
        for (const auto& suite : defs->suites()) {
            suite->addVariable(Variable("ECF_HOME", server.home().string()));
            suite->addVariable(Variable("ECF_JOB_CMD", "true"));
            fs::create_directories(server.home() / suite->name());
            std::ofstream(server.home() / suite->name() / "t.ecf") << "echo job\n";
        }
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
        // /sA/t waits to be completed by hand; /sB/t is free until it receives its trigger
        BOOST_REQUIRE_MESSAGE(client->alter("/sA/t", "change", "defstatus", "suspended") == 0, client->errorMsg());
        BOOST_REQUIRE_MESSAGE(client->alter("/sB/t", "change", "defstatus", "suspended") == 0, client->errorMsg());
        BOOST_REQUIRE_MESSAGE(client->restartServer() == 0, client->errorMsg());
        BOOST_REQUIRE_MESSAGE(client->begin_all_suites() == 0, client->errorMsg());
    }

    // B adds the trigger: allowed by w on /sB/t, whatever /sA
    auto client = server.client(b);
    BOOST_CHECK_MESSAGE(client->alter("/sB/t", "change", "trigger", "/sA/t == complete") == 0, client->errorMsg());
    BOOST_CHECK(!client->server_reply().permission_denied());

    // Released, /sB/t waits for /sA/t
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->resume("/sB/t") == 0, administrator->errorMsg());
    BOOST_CHECK_EQUAL(state_of(server, "/sB/t"), "queued");

    // Once /sA/t completes, /sB/t is submitted
    BOOST_REQUIRE_MESSAGE(administrator->force("/sA/t", "complete") == 0, administrator->errorMsg());
    // a suspended node shows as suspended to triggers; once complete, it no longer runs
    BOOST_REQUIRE_MESSAGE(administrator->resume("/sA/t") == 0, administrator->errorMsg());
    BOOST_REQUIRE_MESSAGE(administrator->job_gen("") == 0, administrator->errorMsg());
    BOOST_CHECK_EQUAL(state_of(server, "/sB/t"), "submitted");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ReadableSet.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The acceptance scenarios of the authorisation and the filtering: A7.3, A7.4 (authorisation, filtering and
/// scheduling), A7.1, the server side of A7.10, and A7.5 (enabling Permissions by a restart).
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
        if (line.find("Command not accepted, due to: Authorisation") != std::string::npos) {
            records.push_back(line);
        }
    }
    return records;
}

///
/// @brief Returns the definitions of a client copy as text: structure, attributes, states, and the server attribute.
///
/// @param[in] defs The copy
/// @return The text, for comparison with a fresh synchronisation.
///
std::string full_text_of(const Defs& defs) {
    std::string text;
    defs.write_to_string(text, PrintStyle::DEFS);
    for (const auto* node : ecf::get_all_nodes(defs)) {
        text += node->absNodePath() + " " + NState::toString(node->state()) + "\n";
    }
    const auto* server = defs.server_state().permissions();
    return text + "server " + (server ? server->to_cli() : std::string("-"));
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsAcceptance)

///
/// @brief A7.3 Denied operation: an operator with r at server level and r, but not x, on /sB requeues the aborted
///        task /sB/t; the request fails with "permission denied" (C++ client and command-line client), the task stays
///        aborted, and one log line holds identity, command, path and reason.
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
        // Identity, command, path and reason (D5.29)
        for (const auto* field :
             {":operator@", "[--requeue /sB/t", "Insufficient permissions (role x required at /sB/t)"}) {
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
        // The output is taken once: a second read returns nothing
        const auto error = requeue.read_stderr();
        BOOST_CHECK_MESSAGE(error.find("(role x required at /sB/t) [--requeue /sB/t :operator@") != std::string::npos,
                            error);
    }
    BOOST_CHECK_EQUAL(state_of(server, "/sB/t"), "aborted");
}

///
/// @brief A7.4 Trigger across teams: B, with r at server level and w on /sB/t but no r on /sA, adds to /sB/t a trigger
///        on /sA/t; the change succeeds, the expression is shown to B as written while /sA is not, and /sB/t is
///        submitted once /sA/t completes.
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

    // The expression is shown to B as written, and no synchronisation or --get of B holds /sA (D5.20)
    {
        auto copy = server.client(b);
        BOOST_REQUIRE_MESSAGE(copy->sync_local() == 0, copy->errorMsg());
        BOOST_CHECK(!copy->defs()->findAbsNode("/sA"));
        const auto task = copy->defs()->findAbsNode("/sB/t");
        BOOST_REQUIRE(task);
        BOOST_REQUIRE(task->get_trigger());
        BOOST_CHECK_EQUAL(task->get_trigger()->expression(), "/sA/t == complete");
        auto get = server.client(b);
        BOOST_REQUIRE_MESSAGE(get->getDefs() == 0, get->errorMsg());
        BOOST_CHECK(!get->defs()->findAbsNode("/sA"));
    }

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

///
/// @brief A7.1 Two teams share a server: a, with r at server level and on sA only, synchronised, stays equal to a
///        fresh full synchronisation without sB when sB changes; --suites, --stats and --ch_suites name nothing of sB.
///
BOOST_AUTO_TEST_CASE(test_a7_1_two_teams_share_a_server) {
    ECF_NAME_THIS_TEST();

    const Credentials a{"teamA", "a-secret"};
    const Credentials b{"teamB", "b-secret"};
    PermissionsServer server(admin,
                             {a, b},
                             R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "teamA", "permissions": "r"},)"
                             R"( {"user": "teamB", "permissions": "r"}])");
    {
        auto defs = Defs::create();
        defs->add_suite("sA")->add_task("t");
        defs->findSuite("sA")->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,teamA:r"));
        defs->add_suite("sB")->add_task("t");
        defs->findSuite("sB")->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,teamB:r"));
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }

    auto client_of_a = server.client(a);
    BOOST_REQUIRE_MESSAGE(client_of_a->ch_register(false, {"sA"}) == 0, client_of_a->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_a->sync_local() == 0, client_of_a->errorMsg());
    auto plain_client_of_a = server.client(a);
    BOOST_REQUIRE_MESSAGE(plain_client_of_a->sync_local() == 0, plain_client_of_a->errorMsg());

    // sB changes state
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->suspend("/sB/t") == 0, administrator->errorMsg());

    // The news of a may report a change (D5.26); its next synchronisation equals a fresh full one, without sB
    BOOST_REQUIRE_MESSAGE(plain_client_of_a->news_local() == 0, plain_client_of_a->errorMsg());
    for (auto* client : {client_of_a.get(), plain_client_of_a.get()}) {
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
        auto fresh = server.client(a);
        BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
        const auto received = ecf::test::node_paths(*client->defs());
        const auto expected = ecf::test::node_paths(*fresh->defs());
        BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
        BOOST_CHECK(received == (std::set<std::string>{"/sA", "/sA/t"}));
    }

    // --suites, --stats and --ch_suites name nothing of sB
    auto listing = server.client(a);
    BOOST_REQUIRE_MESSAGE(listing->suites() == 0, listing->errorMsg());
    BOOST_CHECK(listing->server_reply().get_string_vec() == (std::vector<std::string>{"sA"}));
    BOOST_REQUIRE_MESSAGE(listing->stats() == 0, listing->errorMsg());
    const auto& stats = listing->get_string();
    const auto at     = stats.find("Number of Suites");
    BOOST_REQUIRE(at != std::string::npos);
    BOOST_CHECK_MESSAGE(stats.substr(at, stats.find('\n', at) - at).find('1') != std::string::npos, stats);
    auto client_of_b = server.client(b);
    BOOST_REQUIRE_MESSAGE(client_of_b->ch_register(false, {"sB"}) == 0, client_of_b->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_a->ch_suites() == 0, client_of_a->errorMsg());
    const auto& handles = client_of_a->server_reply().get_client_handle_suites();
    BOOST_REQUIRE_EQUAL(handles.size(), 1U); // the handle of a only
    BOOST_CHECK_EQUAL(handles.front().first, static_cast<unsigned int>(client_of_a->client_handle()));
    BOOST_CHECK(handles.front().second == (std::vector<std::string>{"sA"}));
}

///
/// @brief A7.10 Read removed during a session (server side): u, synchronised while reading /s/f, loses r on /s/f;
///        after its next synchronisation its copy equals a fresh full synchronisation, without /s/f, and --file on a
///        task of /s/f is denied.
///
BOOST_AUTO_TEST_CASE(test_a7_10_read_removed_during_a_session) {
    ECF_NAME_THIS_TEST();

    const Credentials u{"teamU", "u-secret"};
    PermissionsServer server(
        admin, {u}, R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "teamU", "permissions": "r"}])");
    const auto load = [&](bool hidden) {
        const auto file = server.directory() / "s.def";
        std::ofstream(file) << "suite s\n  family f\n"
                            << (hidden ? R"(    permissions [{"user": "admin", "permissions": "rwxlop"}])"
                                         "\n"
                                       : "")
                            << "    task t\n  endfamily\n  family g\n    task t\n  endfamily\nendsuite\n";
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->loadDefs(file.string(), true) == 0, client->errorMsg());
    };
    load(false);

    auto client_of_u = server.client(u);
    BOOST_REQUIRE_MESSAGE(client_of_u->sync_local() == 0, client_of_u->errorMsg());
    BOOST_REQUIRE(client_of_u->defs()->findAbsNode("/s/f/t"));

    load(true); // u loses r on /s/f
    BOOST_REQUIRE_MESSAGE(client_of_u->sync_local() == 0, client_of_u->errorMsg());
    auto fresh = server.client(u);
    BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
    const auto received = ecf::test::node_paths(*client_of_u->defs());
    const auto expected = ecf::test::node_paths(*fresh->defs());
    BOOST_CHECK_EQUAL_COLLECTIONS(received.begin(), received.end(), expected.begin(), expected.end());
    BOOST_CHECK(received == (std::set<std::string>{"/s", "/s/g", "/s/g/t"}));

    auto file_request = server.client(u);
    BOOST_CHECK(file_request->file("/s/f/t", "script") != 0);
    BOOST_CHECK(file_request->server_reply().permission_denied());
}

///
/// @brief A7.5 Enabling: a server whose configuration file gains a server attribute restarts from its checkpoint; the
///        clients reconnect, and each tree, after synchronisation, equals a fresh full synchronisation for its
///        identity, then and after a change of permissions; the start-up summary names the source of the server
///        attribute.
///
BOOST_AUTO_TEST_CASE(test_a7_5_enabling) {
    ECF_NAME_THIS_TEST();

    const Credentials a{"teamA", "a-secret"};
    const Credentials b{"teamB", "b-secret"};
    std::string checkpoint;
    std::vector<std::unique_ptr<ClientInvoker>> clients;
    {
        // Without a server attribute
        PermissionsServer before(admin, {a, b}, "");
        auto defs = Defs::create();
        defs->add_suite("sA")->add_task("t");
        defs->add_suite("sB")->add_task("t");
        auto administrator = before.administrator();
        BOOST_REQUIRE_MESSAGE(administrator->load(defs) == 0, administrator->errorMsg());
        for (const auto& user : {a, b}) {
            clients.push_back(before.client(user));
            BOOST_REQUIRE_MESSAGE(clients.back()->sync_local() == 0, clients.back()->errorMsg());
            clients.push_back(before.client(user));
            BOOST_REQUIRE_MESSAGE(clients.back()->ch_register(false, {"sA"}) == 0, clients.back()->errorMsg());
            BOOST_REQUIRE_MESSAGE(clients.back()->sync_local() == 0, clients.back()->errorMsg());
        }
        BOOST_REQUIRE_MESSAGE(administrator->checkPtDefs() == 0, administrator->errorMsg());
        std::ifstream file(before.home() / ecf::Host{}.prefix_host_and_port(before.port(), Ecf::CHECKPT()));
        std::stringstream text;
        text << file.rdbuf();
        checkpoint = text.str();
        BOOST_REQUIRE(checkpoint.find("suite sA") != std::string::npos);
    }

    // Restarted, from the checkpoint, with a server attribute in the configuration file
    PermissionsServer after(admin,
                            {a, b},
                            R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "teamA", "permissions": "r"},)"
                            R"( {"user": "teamB", "permissions": "r"}])",
                            std::chrono::seconds(30),
                            std::nullopt,
                            std::nullopt,
                            false,
                            {},
                            [&checkpoint](const fs::path& home, const std::string& port) {
                                std::ofstream(home / ecf::Host{}.prefix_host_and_port(port, Ecf::CHECKPT()))
                                    << checkpoint;
                            });
    const auto compare = [&](const std::string& step, bool reconnect) {
        for (std::size_t i = 0; i < clients.size(); ++i) {
            auto& client     = clients[i];
            const auto& user = i < 2 ? a : b;
            client->set_host_port("localhost", after.port());
            // A handle of the former server means nothing to this one (its number may even name another handle): a
            // client reconnecting registers again, as ecFlowUI does
            if (reconnect && client->client_handle() != 0) {
                BOOST_REQUIRE_MESSAGE(client->ch_register(false, {"sA"}) == 0, client->errorMsg());
            }
            BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
            auto fresh = after.client(user);
            if (client->client_handle() != 0) {
                BOOST_REQUIRE_MESSAGE(fresh->ch_register(false, {"sA"}) == 0, fresh->errorMsg());
            }
            BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
            BOOST_CHECK_MESSAGE(full_text_of(*client->defs()) == full_text_of(*fresh->defs()),
                                step << ": the copy of " << user.user << " differs from a fresh synchronisation\n"
                                     << full_text_of(*client->defs()) << "---\n"
                                     << full_text_of(*fresh->defs()));
        }
    };
    compare("restart", true);

    // The server attribute now applies: b hides sB from a
    auto administrator = after.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->permissions("create", "/sB", "admin:rwxlop,teamB:r", false) == 0,
                          administrator->errorMsg());
    compare("after a change of permissions", false);
    BOOST_CHECK(!clients[0]->defs()->findAbsNode("/sB"));
    BOOST_CHECK(clients[2]->defs()->findAbsNode("/sB"));

    std::ifstream log(after.home() / "server.log");
    std::stringstream text;
    text << log.rdbuf();
    BOOST_CHECK_MESSAGE(text.str().find("Source = configuration file") != std::string::npos, text.str());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

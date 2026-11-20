// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/task/InitCmd.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

namespace {

///
/// Provides the log for each test case in the suite, since the server reports requests and denials to it.
///
struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_mock_server_identity.log"};
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_MockServerIdentity, LogFixture)

///
/// @brief Verifies that the mock server applies the authorisation service it is given, to the identity of each request.
///
BOOST_AUTO_TEST_CASE(test_mock_server_applies_authorisation_per_identity) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto defs = Defs::create();
    defs->add_suite("s");
    MockServer server(defs);

    WithTestFile file(AutomaticTestFile{"whitelist"}, "4.5.0\nalice\n-bob\n");
    WhiteListFile whitelist;
    std::string error;
    BOOST_REQUIRE_MESSAGE(whitelist.load(file.path().string(), error), error);
    server.set_authorisation(ecf::AuthorisationService::load_permissions_from_whitelist(whitelist).value());

    const auto bob = TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s"), "bob");
    BOOST_CHECK_MESSAGE(!bob->error().empty(), "a read-only user suspended the suite");
    BOOST_CHECK(!defs->findSuite("s")->isSuspended());

    const auto alice =
        TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s"), "alice");
    BOOST_CHECK_MESSAGE(alice->error().empty(), alice->error());
    BOOST_CHECK(defs->findSuite("s")->isSuspended());
}

///
/// @brief Verifies that the mock server records the kind of each synchronisation reply and its requester.
///
BOOST_AUTO_TEST_CASE(test_mock_server_observes_sync_replies) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s");
    MockServer server(defs);

    TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(0), "alice");
    TestHelper::invokeRequestAs(
        &server, std::make_shared<CSyncCmd>(CSyncCmd::SYNC, 0, Ecf::state_change_no(), Ecf::modify_change_no()), "bob");

    const auto& replies = server.sync_replies();
    BOOST_REQUIRE_EQUAL(replies.size(), 2U);
    BOOST_CHECK(replies[0].full);
    BOOST_CHECK_EQUAL(replies[0].user, "alice");
    BOOST_CHECK(!replies[1].full);
    BOOST_CHECK_EQUAL(replies[1].user, "bob");
}

///
/// @brief Verifies that the full synchronisation of a client handle selecting some suites is recorded as full, as the
///        client classifies it.
///
BOOST_AUTO_TEST_CASE(test_mock_server_observes_subset_handle_sync) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s1");
    defs->add_suite("s2");
    MockServer server(defs);

    auto registration = TestHelper::invokeRequestAs(
        &server, std::make_shared<ClientHandleCmd>(0, std::vector<std::string>{"s1"}, false), "alice");
    BOOST_REQUIRE_MESSAGE(registration->error().empty(), registration->error());
    const auto handle = defs->client_suite_mgr().clientSuites().front().handle();

    auto sync_cmd = std::make_shared<CSyncCmd>(handle);
    auto reply    = TestHelper::invokeRequestAs(&server, sync_cmd, "alice");
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());

    ServerReply client_side;
    client_side.set_client_defs(Defs::create());
    reply->handle_server_response(client_side, sync_cmd, false);

    BOOST_REQUIRE_EQUAL(server.sync_replies().size(), 1U);
    BOOST_CHECK(server.sync_replies()[0].full);
    BOOST_CHECK_EQUAL(server.sync_replies()[0].full, client_side.full_sync());
}

///
/// @brief Verifies that the context of a request, with its identity and server, reaches the reply builders.
///
BOOST_AUTO_TEST_CASE(test_request_context_reaches_reply_builders) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s");
    defs->server_state().set_permissions(ecf::Permissions::parse_cli("alice:rwx"));
    MockServer server(defs);

    TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(0), "alice");

    BOOST_REQUIRE_EQUAL(server.sync_replies().size(), 1U);
    const auto& reply = server.sync_replies()[0];
    BOOST_CHECK_EQUAL(reply.user, "alice");
    BOOST_CHECK(reply.server == &server);
    // The evaluator of the context computes the permissions of the requester
    BOOST_CHECK(reply.permissions_apply);
    BOOST_CHECK_EQUAL(reply.server_roles, "rwx");
}

///
/// @brief Verifies that each reply builder depending on the requester receives the context of the request.
///
BOOST_AUTO_TEST_CASE(test_request_context_reaches_every_reply_builder) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s");
    MockServer server(defs);

    const auto handle = [&] {
        TestHelper::invokeRequestAs(
            &server, std::make_shared<ClientHandleCmd>(0, std::vector<std::string>{"s"}, false), "alice");
        return static_cast<unsigned int>(defs->client_suite_mgr().clientSuites().front().handle());
    }();

    const std::vector<std::pair<std::string, Cmd_ptr>> requests{
        {"defs_cmd", std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET)},
        {"node_cmd", std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/s")},
        {"stats_cmd", std::make_shared<CtsCmd>(CtsCmd::STATS_SERVER)},
        {"suites_cmd", std::make_shared<CtsCmd>(CtsCmd::SUITES)},
        {"zombie_get_cmd", std::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES)},
        {"client_handle_suites_cmd", std::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES)},
        {"news_cmd", std::make_shared<CSyncCmd>(CSyncCmd::NEWS, handle, 0, 0)},
        {"sync_cmd", std::make_shared<CSyncCmd>(CSyncCmd::SYNC, handle, 0, 0)},
        {"sync_clock_cmd", std::make_shared<CSyncCmd>(CSyncCmd::SYNC_CLOCK, handle, 0, 0)},
        {"sync_full_cmd", std::make_shared<CSyncCmd>(handle)},
    };
    for (const auto& [builder, cmd] : requests) {
        BOOST_TEST_CONTEXT("builder " << builder) {
            const auto before = server.replies_built().size();
            const auto reply  = TestHelper::invokeRequestAs(&server, cmd, "alice");
            BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
            BOOST_REQUIRE_EQUAL(server.replies_built().size(), before + 1);
            const auto& built = server.replies_built().back();
            BOOST_CHECK_EQUAL(built.builder, builder);
            BOOST_CHECK_EQUAL(built.user, "alice");
            BOOST_CHECK(built.server == &server);
        }
    }
}

///
/// @brief Verifies that the context of a command exists only while the command is handled, whatever the outcome.
///
BOOST_AUTO_TEST_CASE(test_request_context_lifetime) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s");
    MockServer server(defs);
    const auto alice = ecf::Identity::make_user("alice", "");

    // Before and after a successful request
    auto ok = std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET);
    BOOST_CHECK_THROW((void)ok->request_context(), std::logic_error);
    {
        const ecf::RequestContext context(alice, server);
        (void)ok->handleRequest(&server, context);
    }
    BOOST_CHECK_THROW((void)ok->request_context(), std::logic_error);

    // After a request that throws
    auto failing = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/missing");
    {
        const ecf::RequestContext context(alice, server);
        BOOST_REQUIRE_THROW((void)failing->handleRequest(&server, context), std::exception);
    }
    BOOST_CHECK_THROW((void)failing->request_context(), std::logic_error);

    // After a request refused by its preconditions: task requests on a halted server
    server.set_server_state(SState::HALTED);
    auto blocked = std::make_shared<InitCmd>("/s/t", "pass", "1", 1);
    {
        const ecf::RequestContext context(alice, server);
        const auto reply = blocked->handleRequest(&server, context);
        const auto* stc  = dynamic_cast<const StcCmd*>(reply.get());
        BOOST_REQUIRE(stc != nullptr);
        BOOST_REQUIRE(stc->api() == StcCmd::BLOCK_CLIENT_SERVER_HALTED);
    }
    BOOST_CHECK_THROW((void)blocked->request_context(), std::logic_error);
    server.set_server_state(SState::RUNNING);

    // The commands of a group run in the context of the group, and keep none afterwards, even when one throws
    auto first  = std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET);
    auto second = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/missing");
    auto group  = std::make_shared<GroupCTSCmd>();
    group->addChild(first);
    group->addChild(second);
    const auto before = server.replies_built().size();
    const auto reply  = TestHelper::invokeRequestAs(&server, group, "alice");
    BOOST_REQUIRE_MESSAGE(reply->error().find("missing") != std::string::npos, reply->error());
    BOOST_REQUIRE_EQUAL(server.replies_built().size(), before + 1);
    BOOST_CHECK_EQUAL(server.replies_built().back().user, "alice");
    BOOST_CHECK_THROW((void)first->request_context(), std::logic_error);
    BOOST_CHECK_THROW((void)second->request_context(), std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

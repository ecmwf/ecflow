// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

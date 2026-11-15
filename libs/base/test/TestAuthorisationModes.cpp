// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/task/MeterCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::AuthorisationMode;
using ecf::Permissions;

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_authorisation_modes.log"};
};

///
/// @brief Returns an authorisation service applying a white list of the given content.
///
/// @param[in] content The content of the white list file
/// @param[in] grant_server_user When true, the server user is granted write access, as the server does
///
ecf::AuthorisationService whitelist(const std::string& content, bool grant_server_user = false) {
    using namespace ecf::test::scaffold;
    static WhiteListFile file; // the service keeps a reference to it
    WithTestFile source(AutomaticTestFile{"whitelist"}, content);
    std::string error;
    BOOST_REQUIRE_MESSAGE(file.load(source.path().string(), error), error);
    if (grant_server_user) {
        file.allow_write_access_for_server_user();
    }
    return ecf::AuthorisationService::load_permissions_from_whitelist(file).value();
}

///
/// @brief Builds the definitions: suite s (family f, tasks a and b, task t of family f), with the given attributes.
///
defs_ptr make_defs(const std::string& server, const std::string& on_b = {}) {
    auto defs  = Defs::create();
    auto suite = defs->add_suite("s");
    auto f     = suite->add_family("f");
    f->add_task("t")->addMeter(Meter("m", 0, 100, 100));
    suite->add_task("a");
    auto b = suite->add_task("b");
    if (!on_b.empty()) {
        b->add_permissions(Permissions::parse_cli(on_b));
    }
    if (!server.empty()) {
        defs->server_state().set_permissions(Permissions::parse_cli(server));
    }
    return defs;
}

bool allowed(const STC_Cmd_ptr& reply) {
    return dynamic_cast<const ErrorCmd*>(reply.get()) == nullptr;
}

bool denied(const STC_Cmd_ptr& reply) {
    const auto* error = dynamic_cast<const ErrorCmd*>(reply.get());
    return error && error->kind() == ErrorCmd::Kind::PermissionDenied;
}

STC_Cmd_ptr get(MockServer& server, const std::string& path, const std::string& user) {
    return TestHelper::invokeRequestAs(&server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, path), user);
}

STC_Cmd_ptr suspend(MockServer& server, const std::string& path, const std::string& user) {
    return TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, path), user);
}

} // namespace

namespace {

///
/// @brief A user command whose authorisation fails with an exception, counting how often it runs.
///
class ThrowingCmd final : public UserCmd {
public:
    void print(std::string& os) const override { os += "--throwing"; }
    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer&) const override {
        return ecf::authentication_t::success(true);
    }
    [[nodiscard]] ecf::authorisation_t authorise(const ecf::RequestContext&) const override {
        throw std::runtime_error("broken evaluation");
    }
    [[nodiscard]] const char* theArg() const override { return "throwing"; }
    void addOption(boost::program_options::options_description&) const override {}
    void create(Cmd_ptr&, boost::program_options::variables_map&, AbstractClientEnv*) const override {}

    mutable int runs{0}; ///< How many times the command ran

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override {
        ++runs;
        return PreAllocatedReply::ok_cmd();
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_AuthorisationModes, LogFixture)

///
/// @brief Verifies the selection of the mode: the server attribute, else the white list, else no restriction.
///
BOOST_AUTO_TEST_CASE(test_authorisation_mode_selection) {
    ECF_NAME_THIS_TEST();

    {
        MockServer server(make_defs(""));
        server.set_authorisation(ecf::AuthorisationService::make_unrestricted().value());
        BOOST_CHECK(ecf::authorisation_mode(server) == AuthorisationMode::Unrestricted);
    }
    {
        MockServer server(make_defs(""));
        server.set_authorisation(whitelist("4.5.0\nalice\n"));
        BOOST_CHECK(ecf::authorisation_mode(server) == AuthorisationMode::WhiteList);
    }
    {
        // D5.17: the server attribute applies, and the white list is not used
        MockServer server(make_defs("alice:r"));
        server.set_authorisation(whitelist("4.5.0\nalice\n"));
        BOOST_CHECK(ecf::authorisation_mode(server) == AuthorisationMode::Permissions);
    }
    {
        MockServer server(make_defs("alice:r"));
        server.set_authorisation(ecf::AuthorisationService::make_unrestricted().value());
        BOOST_CHECK(ecf::authorisation_mode(server) == AuthorisationMode::Permissions);
    }
}

///
/// @brief Verifies that, in Permissions mode, reads need r and changes need w, at the paths addressed.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_read_and_write) {
    ECF_NAME_THIS_TEST();

    MockServer server(make_defs("alice:r,bob:rw", "alice:r,bob:r"));

    BOOST_CHECK(allowed(get(server, "/s", "alice")));
    BOOST_CHECK(denied(suspend(server, "/s/a", "alice")));
    BOOST_CHECK(allowed(suspend(server, "/s/a", "bob")));
    BOOST_CHECK(denied(suspend(server, "/s/b", "bob"))); // b restricts bob to r
    BOOST_CHECK(denied(get(server, "/s", "carol")));     // not listed by the server

    // A path without a node is authorised at its closest existing ancestor
    BOOST_CHECK(denied(suspend(server, "/s/missing", "alice")));
    // Not a denial: the request runs, and fails on the missing node
    BOOST_CHECK_THROW((void)suspend(server, "/s/missing", "bob"), std::runtime_error);
}

///
/// @brief Verifies that, in Permissions mode, the white list grants nothing, the server user included (D5.17, D11.6).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_without_implicit_rights) {
    ECF_NAME_THIS_TEST();

    const auto server_user = ecf::get_login_name();
    MockServer server(make_defs("alice:r"));
    server.set_authorisation(whitelist("4.5.0\nalice\ncarol\n", true));

    BOOST_CHECK(denied(suspend(server, "/s/a", "alice")));     // read/write in the white list, r in the attribute
    BOOST_CHECK(denied(get(server, "/s", "carol")));           // read/write in the white list, not listed
    BOOST_CHECK(denied(suspend(server, "/s/a", server_user))); // granted by the white list, not listed

    // The same white list, without server attribute, still grants them
    MockServer unrestricted(make_defs(""));
    unrestricted.set_authorisation(whitelist("4.5.0\nalice\ncarol\n", true));
    BOOST_CHECK(allowed(suspend(unrestricted, "/s/a", "alice")));
    BOOST_CHECK(allowed(suspend(unrestricted, "/s/a", server_user)));
}

///
/// @brief Verifies that --ping is allowed to every user in Permissions mode (D10.1).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_ping) {
    ECF_NAME_THIS_TEST();

    MockServer server(make_defs("alice:r"));
    BOOST_CHECK(allowed(TestHelper::invokeRequestAs(&server, std::make_shared<CtsCmd>(CtsCmd::PING), "carol")));
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, std::make_shared<CtsCmd>(CtsCmd::STATS), "carol")));
}

///
/// @brief Verifies that task requests are not subject to Permissions (D5.1).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_task_requests) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("alice:r");
    MockServer server(defs);
    auto meter = std::make_shared<MeterCmd>(
        "/s/f/t", Submittable::DUMMY_JOBS_PASSWORD(), Submittable::DUMMY_PROCESS_OR_REMOTE_ID(), 1, "m", 42);
    BOOST_CHECK(allowed(TestHelper::invokeRequestAs(&server, meter, "carol")));
    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/f/t")->findMeter("m").value(), 42);
}

///
/// @brief Verifies that, in Permissions mode, each sub-command of a group is authorised just before it runs (D5.27).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_group) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:rw", "bob:r");
    MockServer server(defs);
    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/f"));

    const auto reply = TestHelper::invokeRequestAs(&server, group, "bob");

    // The earlier and later sub-commands ran; the denied one did not
    BOOST_CHECK(defs->findAbsNode("/s/a")->isSuspended());
    BOOST_CHECK(!defs->findAbsNode("/s/b")->isSuspended());
    BOOST_CHECK(defs->findAbsNode("/s/f")->isSuspended());

    // The reply holds the denial of the sub-command, which the client reports as a denial
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK_EQUAL(replies->cmdVec()[0]->error(), "Permission denied: --suspend");
    ServerReply client_reply;
    BOOST_CHECK(!reply->handle_server_response(client_reply, group, false));
    BOOST_CHECK(client_reply.permission_denied());
}

///
/// @brief Verifies that, in white-list mode, a group with a denied sub-command is denied as a whole, as before.
///
BOOST_AUTO_TEST_CASE(test_whitelist_mode_group_unchanged) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("");
    MockServer server(defs);
    server.set_authorisation(whitelist("4.5.0\nbob /s/a\n"));
    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));

    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, group, "bob")));
    BOOST_CHECK(!defs->findAbsNode("/s/a")->isSuspended());
    BOOST_CHECK(!defs->findAbsNode("/s/b")->isSuspended());
}

///
/// @brief Verifies that an error while evaluating permissions denies the request (R11.7).
///
BOOST_AUTO_TEST_CASE(test_fail_closed) {
    ECF_NAME_THIS_TEST();

    const auto thrown = ecf::fail_closed([]() -> ecf::authorisation_t { throw std::runtime_error("broken"); });
    BOOST_CHECK(!thrown.ok());
    BOOST_CHECK(thrown.reason().find("evaluating permissions") != std::string::npos);

    const auto unknown = ecf::fail_closed([]() -> ecf::authorisation_t { throw 42; });
    BOOST_CHECK(!unknown.ok());

    BOOST_CHECK(ecf::fail_closed([] { return ecf::authorisation_t::success(true); }).ok());
    BOOST_CHECK(!ecf::fail_closed([] { return ecf::authorisation_t::failure("no"); }).ok());
}

///
/// @brief Verifies that an attribute path (path:name) is authorised at its node, with or without a leading slash.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_attribute_paths) {
    ECF_NAME_THIS_TEST();

    auto defs  = Defs::create();
    auto suite = defs->add_suite("s");
    suite->addEvent(Event("e"));
    suite->add_permissions(Permissions::parse_cli("bob:r"));
    defs->server_state().set_permissions(Permissions::parse_cli("bob:rw"));
    MockServer server(defs);

    const auto bob = ecf::Identity::make_user("bob", "");
    const ecf::RequestContext context(bob, server);
    for (const auto* path : {"s:e", "/s:e", "s", "/s", "/s/missing:e"}) {
        BOOST_TEST_CONTEXT("path " << path) {
            BOOST_CHECK_EQUAL(context.roles_at(path).to_string(), "r");
        }
    }
    BOOST_CHECK_EQUAL(context.roles_at("/").to_string(), "rw");
    BOOST_CHECK_EQUAL(context.roles_at("").to_string(), "rw");

    for (const auto* path : {"s:e", "/s:e"}) {
        BOOST_TEST_CONTEXT("path " << path) {
            auto force = std::make_shared<ForceCmd>(path, "set", false, false);
            BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, force, "bob")));
            BOOST_CHECK(!suite->findEvent(Event("e")).value());
        }
    }
}

///
/// @brief Verifies that an error while evaluating permissions denies a request, alone or in a group, which then goes on
///        (R11.7).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_fail_closed_requests) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:rw");
    MockServer server(defs);

    auto throwing = std::make_shared<ThrowingCmd>();
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, throwing, "bob")));
    BOOST_CHECK_EQUAL(throwing->runs, 0);

    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(throwing);
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
    const auto reply    = TestHelper::invokeRequestAs(&server, group, "bob");
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK_EQUAL(throwing->runs, 0);
    BOOST_CHECK(defs->findAbsNode("/s/a")->isSuspended());
}

///
/// @brief Verifies that each sub-command of a group is authorised on the definitions as left by the earlier ones.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_group_fresh_evaluation) {
    ECF_NAME_THIS_TEST();

    // b grants bob rw; the replacement of b grants bob r only
    auto defs = make_defs("bob:rw", "bob:rw");
    MockServer server(defs);
    auto replacement = Defs::create();
    replacement->add_suite("s")->add_task("b")->add_permissions(Permissions::parse_cli("bob:r"));

    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<ReplaceNodeCmd>("/s/b", false, replacement, false));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
    const auto reply = TestHelper::invokeRequestAs(&server, group, "bob");

    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/b")->permissions()->to_cli(), "bob:r");
    BOOST_CHECK(!defs->findAbsNode("/s/b")->isSuspended());
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
}

///
/// @brief Verifies that the changes of a group are notified for checkpointing, once, even when a sub-command is denied.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_group_checkpoint) {
    ECF_NAME_THIS_TEST();

    {
        MockServer server(make_defs("bob:rw", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        MockServer server(make_defs("bob:rw"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // A command classified as a write by its arguments, which its clean-up empties
        MockServer server(make_defs("bob:rw", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY, "clear"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // The same command in a group that succeeds, with nothing to return
        MockServer server(make_defs("bob:rw"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY, "clear"));
        group->addChild(std::make_shared<CtsCmd>(CtsCmd::PING));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // Nothing changed: nothing notified
        MockServer server(make_defs("bob:rw", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <csignal>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>
#include <sys/resource.h>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/cts/task/MeterCmd.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/PermissionsCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/cts/user/ShowCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
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

///
/// @brief A command denied by its authorisation, whose first printing throws, as a failure of the recording would.
///
class DeniedOncePrintThrowingCmd final : public UserCmd {
public:
    void print(std::string& os) const override { os += "--denied"; }
    [[nodiscard]] std::string print_short() const override {
        if (!printed) {
            printed = true;
            throw std::runtime_error("broken printing");
        }
        return "--denied";
    }
    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer&) const override {
        return ecf::authentication_t::success(true);
    }
    [[nodiscard]] ecf::authorisation_t authorise(const ecf::RequestContext&) const override {
        return ecf::authorisation_t::failure("Authorisation (user) failed, due to: denied for the test");
    }
    [[nodiscard]] const char* theArg() const override { return "denied"; }
    void addOption(boost::program_options::options_description&) const override {}
    void create(Cmd_ptr&, boost::program_options::variables_map&, AbstractClientEnv*) const override {}

    mutable bool printed{false}; ///< Whether the command was printed once
    mutable int runs{0};         ///< How many times the command ran

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override {
        ++runs;
        return PreAllocatedReply::ok_cmd();
    }
};

///
/// @brief Directs the log to a fresh file that cannot grow beyond 1 byte, for the lifetime of the object.
///
/// The soft limit RLIMIT_FSIZE is lowered and SIGXFSZ ignored, so that writing past the limit fails with EFBIG
/// instead of stopping the process. The destructor restores the limit, the signal disposition and the log, and removes
/// the file, whatever happened meanwhile.
///
/// @throws std::runtime_error when the limit or the signal disposition cannot be changed; nothing is then left changed.
///
class FileSizeLimitedLog {
public:
    FileSizeLimitedLog()
        : log_path_{ecf::Log::instance()->path()},
          file_{(fs::path(log_path_).parent_path() / "test_audit_only_flush_fails.log").string()} {
        std::error_code ignored;
        fs::remove(file_, ignored);
        if (getrlimit(RLIMIT_FSIZE, &saved_) != 0) {
            throw std::runtime_error("getrlimit(RLIMIT_FSIZE) failed");
        }
        // From here on, every change is undone on failure
        previous_ = std::signal(SIGXFSZ, SIG_IGN);
        if (previous_ == SIG_ERR) {
            throw std::runtime_error("signal(SIGXFSZ) failed");
        }
        try {
            ecf::Log::destroy();
            ecf::Log::create(file_);
            rlimit limited   = saved_;
            limited.rlim_cur = 1;
            if (setrlimit(RLIMIT_FSIZE, &limited) != 0) {
                throw std::runtime_error("setrlimit(RLIMIT_FSIZE) failed");
            }
        }
        catch (...) {
            restore();
            throw;
        }
    }

    FileSizeLimitedLog(const FileSizeLimitedLog&)            = delete;
    FileSizeLimitedLog& operator=(const FileSizeLimitedLog&) = delete;

    ~FileSizeLimitedLog() { restore(); }

private:
    ///
    /// @brief Undoes every change, each step independently; never throws, and reports a step that fails.
    ///
    void restore() noexcept {
        if (setrlimit(RLIMIT_FSIZE, &saved_) != 0) {
            BOOST_TEST_MESSAGE("FileSizeLimitedLog: setrlimit(RLIMIT_FSIZE) could not be restored");
        }
        if (std::signal(SIGXFSZ, previous_) == SIG_ERR) {
            BOOST_TEST_MESSAGE("FileSizeLimitedLog: signal(SIGXFSZ) could not be restored");
        }
        try {
            ecf::Log::destroy();
            ecf::Log::create(log_path_);
        }
        catch (...) {
            BOOST_TEST_MESSAGE("FileSizeLimitedLog: the log could not be restored");
        }
        std::error_code ignored;
        fs::remove(file_, ignored);
    }

    std::string log_path_;
    std::string file_;
    rlimit saved_{};
    void (*previous_)(int){SIG_DFL};
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
/// @brief Verifies that, in Permissions mode, a read needs r and an operation (--suspend) x, at the paths addressed.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_read_and_write) {
    ECF_NAME_THIS_TEST();

    MockServer server(make_defs("alice:r,bob:rwx", "alice:r,bob:r"));

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
/// @brief Verifies that the reason of a denial, in the reply and the server log, names the target denied (D5.29).
///
BOOST_AUTO_TEST_CASE(test_denial_reason_names_the_target) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r", "admin:r");
    MockServer server(defs);
    const auto bob = ecf::Identity::make_user("bob", "");
    const ecf::RequestContext context(bob, server);
    CtsNodeCmd get(CtsNodeCmd::GET, "/s/b");
    get.set_identity(bob);
    const auto result = ecf::is_authorised(get, context);
    BOOST_REQUIRE(!result.ok());
    BOOST_CHECK_MESSAGE(result.reason().find("/s/b") != std::string::npos, result.reason());
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

    // --show requires r at the server level
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, std::make_shared<ShowCmd>(), "carol")));
    BOOST_CHECK(allowed(TestHelper::invokeRequestAs(&server, std::make_shared<ShowCmd>(), "alice")));
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

    auto defs = make_defs("bob:rwx", "bob:r");
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
    BOOST_CHECK_MESSAGE(
        replies->cmdVec()[0]->error().rfind("Command not accepted, due to: Authorisation (user) failed, due "
                                            "to: Insufficient permissions (role x required at /s/b) "
                                            "[--suspend /s/b :bob@",
                                            0) == 0,
        replies->cmdVec()[0]->error());
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
    defs->server_state().set_permissions(Permissions::parse_cli("bob:rwx"));
    MockServer server(defs);

    const auto bob = ecf::Identity::make_user("bob", "");
    const ecf::RequestContext context(bob, server);
    for (const auto* path : {"s:e", "/s:e", "s", "/s", "/s/missing:e"}) {
        BOOST_TEST_CONTEXT("path " << path) {
            BOOST_CHECK_EQUAL(context.roles_at(path).to_string(), "r");
        }
    }
    BOOST_CHECK_EQUAL(context.roles_at("/").to_string(), "rwx");
    BOOST_CHECK_EQUAL(context.roles_at("").to_string(), "rwx");

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

    auto defs = make_defs("bob:rwx");
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
/// @brief Verifies that, in audit-only mode, a denial is executed, and an error while evaluating still
///        refuses the request, alone or within a group (R12.3, R11.7).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_audit_only) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r");
    MockServer server(defs);
    server.set_audit_only(true);

    // Denied by the attributes (bob lacks x): executed, and counted
    BOOST_CHECK(
        !denied(TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"), "bob")));
    BOOST_CHECK(defs->findAbsNode("/s/a")->isSuspended());

    // An error while evaluating: refused, never executed
    auto throwing = std::make_shared<ThrowingCmd>();
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, throwing, "bob")));
    BOOST_CHECK_EQUAL(throwing->runs, 0);

    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(throwing);
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
    const auto reply    = TestHelper::invokeRequestAs(&server, group, "bob");
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK_EQUAL(throwing->runs, 0);
    BOOST_CHECK(defs->findAbsNode("/s/b")->isSuspended()); // audited
}

///
/// @brief Verifies that, in audit-only mode, a request denied for want of the role r is refused, alone or within a
///        group, while a request lacking another role is executed, also at a node the requester cannot read.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_audit_only_reads_enforced) {
    ECF_NAME_THIS_TEST();

    // bob holds r at server level and nothing at /s/b; carol holds x at server level, without r
    auto defs = make_defs("bob:r,carol:x", "admin:r");
    MockServer server(defs);
    server.set_audit_only(true);

    // A read lacking r: refused, naming the role
    const auto bob = ecf::Identity::make_user("bob", "");
    const ecf::RequestContext context(bob, server);
    CtsNodeCmd read(CtsNodeCmd::GET, "/s/b");
    read.set_identity(bob);
    const auto result = ecf::is_authorised_or_audited(read, context);
    BOOST_REQUIRE(!result.ok());
    BOOST_CHECK_MESSAGE(result.reason().find("(role r required at /s/b)") != std::string::npos, result.reason());
    BOOST_CHECK(denied(get(server, "/s/b", "bob")));
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, std::make_shared<CtsCmd>(CtsCmd::STATS), "carol")));

    // Lacking x only, at a node bob cannot read: executed
    BOOST_CHECK(allowed(suspend(server, "/s/b", "bob")));
    BOOST_CHECK(defs->findAbsNode("/s/b")->isSuspended());

    // Within a group: the read is refused, the other command executed
    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<CtsNodeCmd>(CtsNodeCmd::WHY, "/s/b"));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
    const auto reply    = TestHelper::invokeRequestAs(&server, group, "bob");
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK(defs->findAbsNode("/s/a")->isSuspended());
}

///
/// @brief Verifies that registering or adding an unreadable suite on the handle of another user is denied naming both
///        the owner of the handle and the role r, and that, in audit-only mode, such a request is refused and counted
///        as denied, alone or within a group (D10.7, R5.2).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_handle_of_another_user_unreadable_suite) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r,victim:r");
    defs->add_suite("u")->add_permissions(Permissions::parse_cli("admin:r"));
    MockServer server(defs);
    auto& stats       = server.stats();
    const auto handle = defs->client_suite_mgr().create_client_suite(false, {"s"}, "victim");
    const auto bob    = ecf::Identity::make_user("bob", "");
    const auto suites = [&]() {
        std::vector<std::string> names;
        defs->client_suite_mgr().suites(handle, names);
        return names;
    };
    const auto both = [](const std::string& reason) {
        return reason.find("the handle belongs to another user") != std::string::npos &&
               reason.find("(role r required at /u)") != std::string::npos;
    };

    for (const bool audit : {false, true}) {
        server.set_audit_only(audit);
        for (const auto api : {ClientHandleCmd::ADD, ClientHandleCmd::REGISTER}) {
            BOOST_TEST_CONTEXT("audit only: " << audit << ", api: " << api) {
                const auto command =
                    api == ClientHandleCmd::ADD
                        ? std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"u"}, ClientHandleCmd::ADD)
                        : std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"u"}, false);
                const auto name = ecf::command_name(*command);

                // One reason names both the owner of the handle and the role r missing
                command->set_identity(bob);
                const ecf::RequestContext context(bob, server);
                const auto result = ecf::is_authorised_or_audited(*command, context);
                BOOST_REQUIRE(!result.ok());
                BOOST_CHECK_MESSAGE(both(result.reason()), result.reason());
                BOOST_CHECK(ecf::is_role_denial(result.reason(), ecf::Role::Read));

                // Refused in both modes, never executed, and counted as denied
                const auto denials = stats.denials_[name];
                const auto reply   = TestHelper::invokeRequestAs(&server, command, "bob");
                BOOST_CHECK(denied(reply));
                BOOST_CHECK_MESSAGE(both(reply->error()), reply->error());
                BOOST_CHECK_EQUAL(stats.denials_[name], denials + 1);
                BOOST_CHECK_EQUAL(stats.audited_.count(name), 0U);
                BOOST_CHECK(suites() == std::vector<std::string>{"s"});
            }
        }
    }

    // Within a group, in audit-only mode: both handle commands refused, the other command audited and executed
    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"u"}, ClientHandleCmd::ADD));
    group->addChild(std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"u"}, false));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
    const auto denials  = stats.denials_.size();
    const auto reply    = TestHelper::invokeRequestAs(&server, group, "bob");
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 2U);
    for (const auto& refused : replies->cmdVec()) {
        BOOST_CHECK(denied(refused));
        BOOST_CHECK_MESSAGE(both(refused->error()), refused->error());
    }
    BOOST_CHECK(suites() == std::vector<std::string>{"s"});
    BOOST_CHECK(defs->findAbsNode("/s/a")->isSuspended());
    BOOST_CHECK_EQUAL(stats.denials_.size(), denials);
    BOOST_CHECK_EQUAL(stats.denials_["--ch_add"], 3U);
    BOOST_CHECK_EQUAL(stats.denials_["--ch_register"], 3U);
    BOOST_CHECK_EQUAL(stats.audited_["--suspend"], 1U);
}

///
/// @brief Verifies that registering or adding a readable suite on the handle of another user is denied naming the
///        owner of the handle only, and that, in audit-only mode, such a request is audited and executed (D10.7).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_handle_of_another_user) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r,victim:r");
    defs->add_suite("v");
    MockServer server(defs);
    auto& stats       = server.stats();
    const auto handle = defs->client_suite_mgr().create_client_suite(false, {"s"}, "victim");
    const auto bob    = ecf::Identity::make_user("bob", "");
    const ecf::RequestContext context(bob, server);
    const auto add     = std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"v"}, ClientHandleCmd::ADD);
    const auto replace = std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"v"}, false);
    add->set_identity(bob);
    replace->set_identity(bob);

    // Permissions mode: refused, naming the owner of the handle only
    for (const auto& command : {add, replace}) {
        const auto result = ecf::is_authorised(*command, context);
        BOOST_REQUIRE(!result.ok());
        BOOST_CHECK_EQUAL(result.reason(), "Authorisation (user) failed, due to: the handle belongs to another user");
        BOOST_CHECK(!ecf::is_role_denial(result.reason(), ecf::Role::Read));
    }

    // Audit-only mode: audited and executed
    server.set_audit_only(true);
    BOOST_CHECK(allowed(TestHelper::invokeRequestAs(&server, add, "bob")));
    BOOST_CHECK_EQUAL(stats.audited_[ecf::command_name(*add)], 1U);
    std::vector<std::string> names;
    defs->client_suite_mgr().suites(handle, names);
    BOOST_CHECK(names == (std::vector<std::string>{"s", "v"}));

    BOOST_CHECK(allowed(TestHelper::invokeRequestAs(&server, replace, "bob")));
    BOOST_CHECK_EQUAL(stats.audited_[ecf::command_name(*replace)], 1U);
    BOOST_CHECK_EQUAL(stats.denials_.size(), 0U);
    BOOST_CHECK(defs->client_suite_mgr().clientSuites().front().user() == "bob");
}

///
/// @brief Verifies that, in audit-only mode, a would-be denial that cannot be written to the log is enforced, alone or
///        within a group.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_audit_only_unrecorded) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r");
    MockServer server(defs);
    server.set_audit_only(true);

    // A log that cannot be written: its directory does not exist
    const auto log_path = ecf::Log::instance()->path();
    ecf::Log::destroy();
    ecf::Log::create((fs::path(log_path).parent_path() / "no_such_directory" / "audit.log").string());

    const bool alone =
        denied(TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"), "bob"));
    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
    const auto reply = TestHelper::invokeRequestAs(&server, group, "bob");

    ecf::Log::destroy();
    ecf::Log::create(log_path);

    BOOST_CHECK(alone);
    BOOST_CHECK(!defs->findAbsNode("/s/a")->isSuspended());
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK(!defs->findAbsNode("/s/b")->isSuspended());
}

///
/// @brief Verifies the statistics of the replies filtered per requester: counted for each kind of filtered reply, with
///        their cost, and never without a server attribute (R12.5).
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_filtered_reply_statistics) {
    ECF_NAME_THIS_TEST();

    // A large definition, so that building a filtered reply takes measurable time; bob cannot read /s/b
    auto defs = make_defs("admin:rwxp,bob:r", "admin:rwxp");
    auto big  = defs->add_suite("big");
    for (int i = 0; i < 2000; ++i) {
        big->add_task("t" + std::to_string(i));
    }
    MockServer server(defs);
    auto& stats = server.stats();

    (void)TestHelper::invokeRequestAs(&server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET), "bob");
    BOOST_CHECK_EQUAL(stats.filtered_replies_, 1U);
    BOOST_CHECK_GT(stats.filtering_time_us_, 0U);
    (void)TestHelper::invokeRequestAs(&server, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET, "/s"), "bob");
    BOOST_CHECK_EQUAL(stats.filtered_replies_, 2U);
    (void)TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(CSyncCmd::SYNC, 0, 0, 0), "bob");
    BOOST_CHECK_EQUAL(stats.filtered_replies_, 3U);
    (void)TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(CSyncCmd::SYNC_CLOCK, 0, 0, 0), "bob");
    BOOST_CHECK_EQUAL(stats.filtered_replies_, 4U);
    (void)TestHelper::invokeRequestAs(&server, std::make_shared<CSyncCmd>(0), "bob"); // full synchronisation
    BOOST_CHECK_EQUAL(stats.filtered_replies_, 5U);

    // Without a server attribute, nothing is filtered, hence nothing counted
    {
        MockServer unrestricted(make_defs(""));
        (void)TestHelper::invokeRequestAs(&unrestricted, std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET), "bob");
        (void)TestHelper::invokeRequestAs(&unrestricted, std::make_shared<CSyncCmd>(0), "bob");
        BOOST_CHECK_EQUAL(unrestricted.stats().filtered_replies_, 0U);
        BOOST_CHECK_EQUAL(unrestricted.stats().filtering_time_us_, 0U);
    }

    // The mode and the readable sets, as --stats reports them
    ecf::update_authorisation_statistics(server);
    BOOST_CHECK_EQUAL(stats.authorisation_mode_, "permissions");
    BOOST_CHECK_GE(stats.readable_sets_, 1U);
    server.set_audit_only(true);
    ecf::update_authorisation_statistics(server);
    BOOST_CHECK_EQUAL(stats.authorisation_mode_, "permissions (audit only)");
}

///
/// @brief Verifies that, in audit-only mode, a would-be denial whose record is buffered but cannot be flushed (e.g. on
/// a
///        full file system) is enforced, alone or within a group.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_audit_only_flush_fails) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs("bob:r");
    MockServer server(defs);
    server.set_audit_only(true);

    // A log whose file cannot grow: the record fits the buffer of the stream, the flush fails (EFBIG); every
    // process-wide change is undone by the guard, whatever happens in the test
    bool alone = false;
    STC_Cmd_ptr reply;
    {
        FileSizeLimitedLog limited;
        alone =
            denied(TestHelper::invokeRequestAs(&server, std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"), "bob"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        reply = TestHelper::invokeRequestAs(&server, group, "bob");
    }

    BOOST_CHECK(alone);
    BOOST_CHECK(!defs->findAbsNode("/s/a")->isSuspended());
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK(!defs->findAbsNode("/s/b")->isSuspended());
}

///
/// @brief Verifies that, in audit-only mode, a would-be denial whose recording throws is enforced, alone or within a
///        group.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_audit_only_recording_throws) {
    ECF_NAME_THIS_TEST();

    MockServer server(make_defs("bob:r"));
    server.set_audit_only(true);

    auto alone = std::make_shared<DeniedOncePrintThrowingCmd>();
    BOOST_CHECK(denied(TestHelper::invokeRequestAs(&server, alone, "bob")));
    BOOST_CHECK(alone->printed);
    BOOST_CHECK_EQUAL(alone->runs, 0);

    auto within = std::make_shared<DeniedOncePrintThrowingCmd>();
    auto group  = std::make_shared<GroupCTSCmd>();
    group->addChild(within);
    const auto reply    = TestHelper::invokeRequestAs(&server, group, "bob");
    const auto* replies = dynamic_cast<const GroupSTCCmd*>(reply.get());
    BOOST_REQUIRE(replies != nullptr);
    BOOST_REQUIRE_EQUAL(replies->cmdVec().size(), 1U);
    BOOST_CHECK(denied(replies->cmdVec()[0]));
    BOOST_CHECK(within->printed);
    BOOST_CHECK_EQUAL(within->runs, 0);
}

///
/// @brief Verifies that the server checks the user diagnosed by --permissions effective, for a request built directly,
///        which no client has checked (R2.8).
///
BOOST_AUTO_TEST_CASE(test_permissions_effective_user_checked_by_the_server) {
    ECF_NAME_THIS_TEST();

    MockServer server(make_defs("bob:r"));
    // The server refuses the request while handling it (the server turns the exception into an error reply)
    for (const std::string name : {"a:b", "*", "al ice"}) {
        BOOST_TEST_CONTEXT("user '" << name << "'") {
            BOOST_CHECK_EXCEPTION((void)TestHelper::invokeRequestAs(
                                      &server, std::make_shared<PermissionsCmd>("effective", "/s", name), "bob"),
                                  std::runtime_error,
                                  [](const std::runtime_error& e) {
                                      return std::string(e.what()).find("invalid user name") != std::string::npos;
                                  });
        }
    }
    const auto reply =
        TestHelper::invokeRequestAs(&server, std::make_shared<PermissionsCmd>("effective", "/s", "bob"), "bob");
    BOOST_CHECK(dynamic_cast<const ErrorCmd*>(reply.get()) == nullptr);
}

///
/// @brief Verifies that each sub-command of a group is authorised on the definitions as left by the earlier ones.
///
BOOST_AUTO_TEST_CASE(test_permissions_mode_group_fresh_evaluation) {
    ECF_NAME_THIS_TEST();

    // b grants bob rwxp (p to replace a node carrying an attribute); the replacement of b grants bob r only, and keeps
    // p for admin, so that the validation of changes accepts it (D4.6)
    auto defs = make_defs("admin:rwxp,bob:rwxp", "admin:rwxp,bob:rwxp");
    MockServer server(defs);
    auto replacement = Defs::create();
    replacement->add_suite("s")->add_task("b")->add_permissions(Permissions::parse_cli("admin:rwxp,bob:r"));

    auto group = std::make_shared<GroupCTSCmd>();
    group->addChild(std::make_shared<ReplaceNodeCmd>("/s/b", false, replacement, false));
    group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
    const auto reply = TestHelper::invokeRequestAs(&server, group, "bob");

    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/b")->permissions()->to_cli(), "admin:rwxp,bob:r");
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
        MockServer server(make_defs("bob:rwx", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        MockServer server(make_defs("bob:rwx"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/a"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // A command classified as a write by its arguments, which its clean-up empties
        MockServer server(make_defs("bob:rwx", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY, "clear"));
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // The same command in a group that succeeds, with nothing to return
        MockServer server(make_defs("bob:rwx"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY, "clear"));
        group->addChild(std::make_shared<CtsCmd>(CtsCmd::PING));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 1);
    }
    {
        // Nothing changed: nothing notified
        MockServer server(make_defs("bob:rwx", "bob:r"));
        auto group = std::make_shared<GroupCTSCmd>();
        group->addChild(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s/b"));
        (void)TestHelper::invokeRequestAs(&server, group, "bob");
        BOOST_CHECK_EQUAL(server.node_tree_state_changes(), 0);
    }
}

///
/// @brief Verifies that handles are created for, and dropped by, the authenticated requester, whatever user the request
///        names (D10.7).
///
BOOST_AUTO_TEST_CASE(test_handles_follow_the_authenticated_requester) {
    ECF_NAME_THIS_TEST();

    for (const auto* server_attribute : {"bob:r,victim:r", ""}) {
        BOOST_TEST_CONTEXT("server attribute: " << server_attribute) {
            auto defs = make_defs(server_attribute);
            MockServer server(defs);
            (void)defs->client_suite_mgr().create_client_suite(false, {"s"}, "victim");

            // In a group, the sub-commands take the identity of the group, but keep the user they name: the
            // credentials of the group are set before the sub-commands naming victim are added
            const auto send = [&](const Cmd_ptr& child) {
                auto group = std::make_shared<GroupCTSCmd>();
                static_cast<ClientToServerCmd&>(*group).setup_user_authentification("bob", "");
                child->setup_user_authentification("victim", "");
                group->addChild(child);
                // As the server does on receiving the request: the identity of the group, given to its sub-commands
                group->set_identity(ecf::Identity::make_user("bob", ""));
                BOOST_REQUIRE_EQUAL(child->identity().username().value(), "bob");
                BOOST_REQUIRE_EQUAL(dynamic_cast<const UserCmd&>(*child).user(), "victim");
                ClientToServerRequest request;
                request.set_cmd(group);
                const auto reply = request.handleRequest(&server);
                BOOST_REQUIRE_EQUAL(dynamic_cast<const UserCmd&>(*child).user(), "victim"); // the mismatch held
                return reply;
            };

            // A handle registered by bob belongs to bob, whatever user the sub-command names
            const auto registered = send(std::make_shared<ClientHandleCmd>(0, std::vector<std::string>{"s"}, false));
            BOOST_CHECK(registered->ok());
            const auto& handles = defs->client_suite_mgr().clientSuites();
            BOOST_REQUIRE_EQUAL(handles.size(), 2U);
            BOOST_CHECK_EQUAL(handles.back().user(), "bob");

            // An implicit --ch_drop_user drops bob's handles, not victim's
            const auto dropped = send(std::make_shared<ClientHandleCmd>(std::string{}));
            BOOST_CHECK(dropped->ok());
            BOOST_REQUIRE_EQUAL(handles.size(), 1U);
            BOOST_CHECK_EQUAL(handles.front().user(), "victim");
        }
    }
}

///
/// @brief Verifies that a command forwarded on behalf of a requester carries the requester's credentials and kind
///        (R11.9), through serialisation too.
///
BOOST_AUTO_TEST_CASE(test_forwarded_identity) {
    ECF_NAME_THIS_TEST();

    for (const bool custom : {true, false}) {
        BOOST_TEST_CONTEXT("custom: " << custom) {
            const auto identity =
                custom ? ecf::Identity::make_custom_user("bob", "pw") : ecf::Identity::make_user("bob", "pw");
            MoveCmd move;
            move.setup_user_authentification(identity);
            BOOST_CHECK_EQUAL(move.user(), "bob");
            BOOST_CHECK_EQUAL(move.passwd(), "pw");
            BOOST_CHECK_EQUAL(move.is_custom_user(), custom);
            BOOST_CHECK_EQUAL(move.identity().is_custom(), custom);

            std::string data;
            ecf::save_as_string(data, move);
            MoveCmd received;
            ecf::restore_from_string(data, received);
            BOOST_CHECK_EQUAL(received.user(), "bob");
            BOOST_CHECK_EQUAL(received.is_custom_user(), custom);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

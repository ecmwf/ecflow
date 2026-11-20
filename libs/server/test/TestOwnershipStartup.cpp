// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/server/OwnershipStartup.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/FakePrivileges.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::FakeAccountDatabase;
using ecf::test::scaffold::FakePrivileges;

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_OwnershipStartup)

namespace {

constexpr uid_t server_uid = 500;

FakeAccountDatabase make_accounts() {
    FakeAccountDatabase accounts;
    accounts.add_group("root", 0).add_group("ecflow", 500).add_group("users", 100).add_group("ops", 3000);
    accounts.add_user("root", 0, 0).add_user("ecflow", server_uid, 500);
    accounts.add_user("ecflow-default", 1000, 100, {3000});
    return accounts;
}

OwnershipSettings configured(const std::string& owner) {
    OwnershipSettings settings;
    settings.owner = owner;
    return settings;
}

/// A server running under the server account, with the capabilities to change identity
FakePrivileges capable() {
    FakePrivileges privileges;
    privileges.uid              = server_uid;
    privileges.has_capabilities = true;
    return privileges;
}

void check_refused(const OwnershipSettings& settings,
                   FakePrivileges& privileges,
                   const std::string& expected_in_reason,
                   bool available_in_build = true) {
    auto accounts = make_accounts();
    BOOST_CHECK_EXCEPTION(check_ownership_at_startup(settings, accounts, privileges, available_in_build),
                          std::runtime_error,
                          [&](const std::runtime_error& e) {
                              bool found = std::string(e.what()).find(expected_in_reason) != std::string::npos;
                              BOOST_CHECK_MESSAGE(found, "Unexpected reason: " << e.what());
                              return found;
                          });
}

} // namespace

BOOST_AUTO_TEST_CASE(test_disabled_without_server_owner) {
    ECF_NAME_THIS_TEST();

    // Nothing is checked, nor needed, whatever the build and the privileges
    auto accounts = make_accounts();
    FakePrivileges privileges;
    privileges.supports_capabilities = false;
    privileges.executable_problem    = "never checked";
    for (bool available : {false, true}) {
        auto status = check_ownership_at_startup(OwnershipSettings{}, accounts, privileges, available);
        BOOST_CHECK(!status.enabled);
        BOOST_CHECK(!status.server_owner);
        BOOST_CHECK(status.warnings.empty());
    }
    BOOST_CHECK(privileges.trials.empty());
    BOOST_CHECK(!privileges.raised);
}

BOOST_AUTO_TEST_CASE(test_refused_in_a_build_without_ownership) {
    ECF_NAME_THIS_TEST();

    auto privileges = capable();
    check_refused(configured("ecflow-default"), privileges, "cannot be enabled in this build", false);
    BOOST_CHECK(privileges.trials.empty());
}

BOOST_AUTO_TEST_CASE(test_refused_server_owners) {
    ECF_NAME_THIS_TEST();

    auto privileges = capable();
    check_refused(configured("1000"), privileges, "Invalid server Owner '1000'");
    check_refused(configured("ecflow-default:"), privileges, "Invalid server Owner");
    check_refused(configured("nobody-here"), privileges, "unknown user");
    check_refused(configured("root"), privileges, "superuser");
    check_refused(configured("ecflow"), privileges, "server account");
    check_refused(configured("ecflow-default:ecflow"), privileges, "not a member");
    check_refused(configured("ecflow-default:nogroup"), privileges, "unknown group");
    BOOST_CHECK(privileges.trials.empty());
}

BOOST_AUTO_TEST_CASE(test_refused_without_the_privilege_to_change_identity) {
    ECF_NAME_THIS_TEST();

    {
        // Linux, without the capabilities
        FakePrivileges privileges;
        privileges.uid = server_uid;
        check_refused(configured("ecflow-default"), privileges, "CAP_SETUID and CAP_SETGID");
    }
    {
        // Elsewhere, not root
        FakePrivileges privileges;
        privileges.uid                   = server_uid;
        privileges.supports_capabilities = false;
        check_refused(configured("ecflow-default"), privileges, "running as root");
    }
    {
        // The capabilities cannot be raised
        auto privileges       = capable();
        privileges.fail_raise = true;
        check_refused(configured("ecflow-default"), privileges, "could not raise the capabilities");
    }
    {
        // The trial switch fails
        auto privileges          = capable();
        privileges.trial_failure = "setuid: Operation not permitted";
        check_refused(configured("ecflow-default"), privileges, "setuid: Operation not permitted");
    }
}

BOOST_AUTO_TEST_CASE(test_refused_with_an_unsafe_executable) {
    ECF_NAME_THIS_TEST();

    auto privileges               = capable();
    privileges.executable_problem = "it carries file capabilities and is executable by other accounts";
    check_refused(configured("ecflow-default"), privileges, "executable by other accounts");
}

BOOST_AUTO_TEST_CASE(test_enabled_with_capabilities) {
    ECF_NAME_THIS_TEST();

    auto accounts   = make_accounts();
    auto privileges = capable();
    auto status     = check_ownership_at_startup(configured("ecflow-default:ops"), accounts, privileges, true);
    BOOST_CHECK(status.enabled);
    BOOST_REQUIRE(status.server_owner);
    BOOST_CHECK_EQUAL(status.server_owner->value(), "ecflow-default:ops");
    BOOST_CHECK(privileges.raised);
    BOOST_REQUIRE_EQUAL(privileges.trials.size(), 1u);
    BOOST_CHECK_EQUAL(privileges.trials.front().uid, 1000u);
    BOOST_CHECK_EQUAL(privileges.trials.front().gid, 3000u);
    BOOST_CHECK(status.warnings.empty());
    BOOST_CHECK_MESSAGE(status.summary().find("server Owner ecflow-default:ops") != std::string::npos,
                        status.summary());
    BOOST_CHECK_MESSAGE(status.summary().find("CAP_SETUID") != std::string::npos, status.summary());
}

BOOST_AUTO_TEST_CASE(test_enabled_as_root_with_a_warning) {
    ECF_NAME_THIS_TEST();

    for (bool supports_capabilities : {true, false}) {
        auto accounts = make_accounts();
        FakePrivileges privileges;
        privileges.uid                   = 0;
        privileges.supports_capabilities = supports_capabilities;
        auto status = check_ownership_at_startup(configured("ecflow-default"), accounts, privileges, true);
        BOOST_CHECK(status.enabled);
        BOOST_CHECK(!privileges.raised);
        BOOST_CHECK_EQUAL(privileges.trials.size(), 1u);
        BOOST_REQUIRE(!status.warnings.empty());
        BOOST_CHECK_MESSAGE(status.warnings.front().find("root") != std::string::npos, status.warnings.front());
        BOOST_CHECK_MESSAGE(status.summary().find("root") != std::string::npos, status.summary());
        if (!supports_capabilities) {
            // macOS: only the spawning of commands as the owner is supported
            BOOST_CHECK_EQUAL(status.warnings.size(), 2u);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_warning_for_owners_when_disabled) {
    ECF_NAME_THIS_TEST();

    defs_ptr defs = Defs::create();
    defs->add_suite("s")->add_family("f");
    OwnershipStatus disabled;
    BOOST_CHECK(!owners_without_ownership_warning(*defs, disabled));

    defs->findAbsNode("/s/f")->addOwner(OwnerAttr::create("alice"));
    auto warning = owners_without_ownership_warning(*defs, disabled);
    BOOST_REQUIRE(warning);
    BOOST_CHECK_MESSAGE(warning->find("server account") != std::string::npos, *warning);

    OwnershipStatus enabled;
    enabled.enabled = true;
    BOOST_CHECK(!owners_without_ownership_warning(*defs, enabled));
}

BOOST_AUTO_TEST_CASE(test_warning_for_owners_in_archives_when_disabled) {
    ECF_NAME_THIS_TEST();

    // The only Owner is held by a task of an archived family, absent from memory
    ecf::test::scaffold::TestLog test_log{"test_warning_for_owners_in_archives_when_disabled.log"};
    auto ecf_home = ecf::test::scaffold::MakeDirectory{}.create();
    defs_ptr defs = Defs::create();
    suite_ptr s   = defs->add_suite("s");
    s->add_variable("ECF_HOME", fs::absolute(ecf_home.path()).string());
    family_ptr f = s->add_family("f");
    f->add_task("t")->addOwner(OwnerAttr::create("alice"));
    f->archive();
    BOOST_REQUIRE(f->get_flag().is_set(ecf::Flag::ARCHIVED));
    BOOST_REQUIRE(f->children().empty());

    OwnershipStatus disabled;
    auto warning = owners_without_ownership_warning(*defs, disabled);
    BOOST_REQUIRE_MESSAGE(warning, "No warning for the Owner of an archived node");
    BOOST_CHECK_MESSAGE(warning->find("server account") != std::string::npos, *warning);

    // An unreadable archive does not prevent the start-up: it is skipped, with a warning in the log
    {
        std::ofstream corrupt(f->archive_path(), std::ios::trunc);
        corrupt << "not an archive";
    }
    std::optional<std::string> unreadable;
    BOOST_REQUIRE_NO_THROW(unreadable = owners_without_ownership_warning(*defs, disabled));
    BOOST_CHECK(!unreadable);
    Log::instance()->flush();
    std::ifstream log("test_warning_for_owners_in_archives_when_disabled.log");
    const std::string content{std::istreambuf_iterator<char>(log), std::istreambuf_iterator<char>()};
    BOOST_CHECK_MESSAGE(content.find("Could not read the archive of /s/f") != std::string::npos, content);

    // Nor does an archive that cannot be reached (ECF_HOME not traversable)
    if (::geteuid() != 0) {
        BOOST_REQUIRE(::chmod(ecf_home.path().c_str(), 0) == 0);
        std::optional<std::string> unreachable;
        BOOST_CHECK_NO_THROW(unreachable = owners_without_ownership_warning(*defs, disabled));
        BOOST_CHECK(!unreachable);
        BOOST_REQUIRE(::chmod(ecf_home.path().c_str(), 0700) == 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

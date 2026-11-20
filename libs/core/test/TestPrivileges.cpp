// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <optional>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/os/AccountDatabase.hpp"
#include "ecflow/core/os/Privileges.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf::os;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Privileges)

BOOST_AUTO_TEST_CASE(test_system_privileges_describe_the_process) {
    ECF_NAME_THIS_TEST();

    SystemPrivileges privileges;
    BOOST_CHECK_EQUAL(privileges.effective_uid(), ::geteuid());
#if defined(__linux__)
    BOOST_CHECK(privileges.capabilities_supported());
#else
    BOOST_CHECK(!privileges.capabilities_supported());
    BOOST_CHECK(!privileges.has_identity_capabilities());
    BOOST_CHECK_THROW(privileges.raise_identity_capabilities(), std::runtime_error);
#endif
    // The test executable carries no file capabilities
    auto problem = privileges.check_executable();
    BOOST_CHECK_MESSAGE(!problem, problem.value_or(""));
}

BOOST_AUTO_TEST_CASE(test_trial_switch_without_privileges_fails) {
    ECF_NAME_THIS_TEST();

    SystemPrivileges privileges;
    if (::geteuid() == 0 || privileges.has_identity_capabilities()) {
        BOOST_TEST_MESSAGE("Skipped: the process can change identity");
        return;
    }
    // Another identity than that of the process cannot be taken without privileges
    const passwd* root = ::getpwnam("root");
    BOOST_REQUIRE(root != nullptr);
    OsIdentity identity{"root", root->pw_uid, root->pw_gid, {root->pw_gid}, root->pw_dir, root->pw_shell};
    BOOST_CHECK_THROW(privileges.trial_switch(identity), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_inspect_executable_permissions) {
    ECF_NAME_THIS_TEST();

    // Set-user-ID and set-group-ID executables (file capabilities need privileges, and are tested in the
    // privileged environment)
    auto directory  = ecf::test::scaffold::MakeDirectory{}.create();
    const auto path = (fs::absolute(directory.path()) / "server").string();
    std::ofstream(path) << "#!/bin/sh\n";
    const uid_t self = ::geteuid();
    auto inspect     = [&](mode_t mode, uid_t server_uid) {
        BOOST_REQUIRE(::chmod(path.c_str(), mode) == 0);
        return inspect_executable(path, server_uid);
    };

    BOOST_CHECK(!inspect(0755, self));  // grants nothing
    BOOST_CHECK(!inspect(04700, self)); // only the server account can run it
    BOOST_CHECK(!inspect(02700, self));

    auto refused = [](const std::optional<std::string>& problem, const std::string& expected) {
        BOOST_CHECK_MESSAGE(problem && problem->find(expected) != std::string::npos, problem.value_or("accepted"));
    };
    refused(inspect(04755, self), "set-user-ID or set-group-ID and is executable by other accounts");
    refused(inspect(04710, self), "executable by other accounts");
    refused(inspect(02701, self), "executable by other accounts");
    if (self != 0) {
        refused(inspect(04700, self + 1), "owned by another account than root or the server account");
    }

    BOOST_REQUIRE(::chmod(path.c_str(), 0600) == 0);
    refused(inspect_executable((fs::absolute(directory.path()) / "missing").string(), self), "cannot be inspected");
}

#if defined(__APPLE__)
BOOST_AUTO_TEST_CASE(test_inspect_executable_with_an_access_control_list) {
    ECF_NAME_THIS_TEST();

    // An ACL can grant execution to other accounts without any group or other execute bit (Linux ACLs are tested in
    // the privileged environment)
    auto directory  = ecf::test::scaffold::MakeDirectory{}.create();
    const auto path = (fs::absolute(directory.path()) / "server").string();
    std::ofstream(path) << "#!/bin/sh\n";
    BOOST_REQUIRE(::chmod(path.c_str(), 04700) == 0);
    BOOST_REQUIRE(!inspect_executable(path, ::geteuid()));

    ecf::test::scaffold::Process chmod("/bin/chmod", {"+a", "everyone allow execute", path});
    BOOST_REQUIRE_EQUAL(chmod.wait(), 0);
    const auto problem = inspect_executable(path, ::geteuid());
    BOOST_CHECK_MESSAGE(problem && problem->find("set-user-ID or set-group-ID and has an access control list") !=
                                       std::string::npos,
                        problem.value_or("accepted"));
}
#endif

BOOST_AUTO_TEST_CASE(test_groups_of_an_identity) {
    ECF_NAME_THIS_TEST();

    OsIdentity identity{"alice", 1001, 100, {}, "/home/alice", "/bin/sh"};
    for (gid_t gid = 200; gid < 220; ++gid) {
        identity.groups.push_back(gid);
    }
    identity.groups.push_back(100); // the primary group, also listed as supplementary

    const auto groups = groups_of(identity);
    BOOST_REQUIRE(!groups.empty());
    BOOST_CHECK_EQUAL(groups.front(), 100U);
    BOOST_CHECK_EQUAL(std::count(groups.begin(), groups.end(), 100U), 1);
#if defined(__APPLE__)
    // macOS takes at most NGROUPS groups: the primary group and the first supplementary ones
    BOOST_REQUIRE_EQUAL(groups.size(), 16U);
    BOOST_CHECK_EQUAL(groups.back(), 214U);
#else
    BOOST_REQUIRE_EQUAL(groups.size(), 21U);
    BOOST_CHECK_EQUAL(groups.back(), 219U);
#endif
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

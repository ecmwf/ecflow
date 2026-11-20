// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <grp.h>
#include <optional>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/os/AccountDatabase.hpp"
#include "ecflow/core/os/AccountLookup.hpp"
#include "ecflow/core/os/OwnerEligibility.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf::os;
using ecf::test::scaffold::FakeAccountDatabase;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_AccountDatabase)

namespace {

/// The account running the tests, when it has an entry in the account database (containers may run under a numeric
/// user without one)
std::optional<Account> current_account() {
    const passwd* pw = getpwuid(getuid());
    if (pw == nullptr) {
        return std::nullopt;
    }
    return Account{pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell};
}

constexpr uid_t some_other_server_uid = 4000000001; // not an account on the test hosts

void check_refused(const OwnerEligibility& eligibility,
                   const std::string& user,
                   const std::optional<std::string>& group,
                   const std::string& expected_in_reason) {
    const auto result = eligibility.check(user, group);
    BOOST_CHECK_MESSAGE(!result.ok(), "Expected '" << user << "' to be refused");
    if (!result.ok()) {
        BOOST_CHECK_MESSAGE(result.reason().find(expected_in_reason) != std::string::npos,
                            "Unexpected reason: " << result.reason());
        BOOST_CHECK_MESSAGE(result.reason().find(user) != std::string::npos,
                            "The reason should name the user: " << result.reason());
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(test_system_lookup_of_current_login) {
    ECF_NAME_THIS_TEST();

    auto expected = current_account();
    if (!expected) {
        BOOST_TEST_MESSAGE("Skipped: the current user has no entry in the account database");
        return;
    }

    SystemAccountDatabase db;
    auto account = db.find_user(expected->name);
    BOOST_REQUIRE(account.has_value());
    BOOST_CHECK_EQUAL(account->name, expected->name);
    BOOST_CHECK_EQUAL(account->uid, expected->uid);
    BOOST_CHECK_EQUAL(account->gid, expected->gid); // the recorded primary group, not the process group
    BOOST_CHECK_EQUAL(account->home, expected->home);

    auto groups = db.groups_of(*account);
    BOOST_REQUIRE(!groups.empty());
    BOOST_CHECK_EQUAL(groups.front(), account->gid);

    // The primary group, looked up by name
    if (const group* gr = getgrgid(account->gid); gr != nullptr) {
        auto found = db.find_group(gr->gr_name);
        BOOST_REQUIRE(found.has_value());
        BOOST_CHECK_EQUAL(found->gid, account->gid);
    }
}

BOOST_AUTO_TEST_CASE(test_system_lookup_of_root_and_unknown) {
    ECF_NAME_THIS_TEST();

    SystemAccountDatabase db;
    auto root = db.find_user("root");
    BOOST_REQUIRE(root.has_value());
    BOOST_CHECK_EQUAL(root->uid, 0u);

    BOOST_CHECK(!db.find_user("ecflow-no-such-user-0815").has_value());
    BOOST_CHECK(!db.find_group("ecflow-no-such-group-0815").has_value());
}

BOOST_AUTO_TEST_CASE(test_eligibility_with_system_accounts) {
    ECF_NAME_THIS_TEST();

    SystemAccountDatabase db;
    check_refused(OwnerEligibility(db, some_other_server_uid), "root", std::nullopt, "superuser");
    check_refused(OwnerEligibility(db, some_other_server_uid), "ecflow-no-such-user-0815", std::nullopt, "unknown");

    // The checks below use the current account, which must be an ordinary named account (not so in some containers)
    auto account = current_account();
    if (!account || account->uid == 0) {
        BOOST_TEST_MESSAGE("Skipped: the current user is the superuser, or has no entry in the account database");
        return;
    }
    {
        // The current account runs the server: it cannot be an owner
        OwnerEligibility eligibility(db, account->uid);
        check_refused(eligibility, account->name, std::nullopt, "server account");
    }
    {
        // With another server account, the current account is an eligible owner
        OwnerEligibility eligibility(db, some_other_server_uid);
        const auto result = eligibility.check(account->name, std::nullopt);
        BOOST_REQUIRE_MESSAGE(result.ok(), (result.ok() ? std::string() : result.reason()));
        BOOST_CHECK_EQUAL(result.value().user, account->name);
        BOOST_CHECK_EQUAL(result.value().uid, account->uid);
        BOOST_CHECK_EQUAL(result.value().gid, account->gid);
    }
}

BOOST_AUTO_TEST_CASE(test_eligibility_groups) {
    ECF_NAME_THIS_TEST();

    FakeAccountDatabase db;
    db.add_group("alice", 2001).add_group("ops", 3000).add_group("dev", 3001);
    db.add_user("alice", 1001, 2001, {3000});
    OwnerEligibility eligibility(db, 999);

    // By default, the primary group
    const auto primary = eligibility.check("alice", std::nullopt);
    BOOST_REQUIRE(primary.ok());
    BOOST_CHECK_EQUAL(primary.value().uid, 1001u);
    BOOST_CHECK_EQUAL(primary.value().gid, 2001u);
    BOOST_CHECK(primary.value().groups == (std::vector<gid_t>{2001, 3000}));

    // A supplementary group the user belongs to
    const auto supplementary = eligibility.check("alice", std::string("ops"));
    BOOST_REQUIRE(supplementary.ok());
    BOOST_CHECK_EQUAL(supplementary.value().gid, 3000u);

    // Refusals
    check_refused(eligibility, "alice", std::string("dev"), "not a member");
    check_refused(eligibility, "alice", std::string("nogroup"), "unknown group");
    check_refused(eligibility, "bob", std::nullopt, "unknown");
}

BOOST_AUTO_TEST_CASE(test_eligibility_refuses_superuser_and_server_account) {
    ECF_NAME_THIS_TEST();

    FakeAccountDatabase db;
    db.add_group("root", 0).add_group("ecflow", 500);
    db.add_user("root", 0, 0).add_user("ecflow", 500, 500).add_user("toor", 0, 500);
    OwnerEligibility eligibility(db, 500);

    check_refused(eligibility, "root", std::nullopt, "superuser");
    check_refused(eligibility, "toor", std::nullopt, "superuser"); // any name with uid 0
    check_refused(eligibility, "ecflow", std::nullopt, "server account");
}

BOOST_AUTO_TEST_CASE(test_lookup_grows_the_buffer) {
    ECF_NAME_THIS_TEST();

    // A lookup reporting ERANGE until the buffer holds 4096 bytes
    std::vector<std::size_t> sizes;
    auto function = [&sizes](const char*, int* entry, char*, std::size_t size, int** result) {
        sizes.push_back(size);
        if (size < 4096) {
            return ERANGE;
        }
        *entry  = 42;
        *result = entry;
        return 0;
    };
    int entry   = 0;
    int* result = nullptr;
    std::vector<char> buffer(1024);
    BOOST_CHECK_EQUAL(detail::lookup_with_growing_buffer(function, "alice", entry, result, buffer), 0);
    BOOST_CHECK(result == &entry);
    BOOST_CHECK(sizes == (std::vector<std::size_t>{1024, 2048, 4096}));
}

BOOST_AUTO_TEST_CASE(test_lookup_buffer_is_bounded) {
    ECF_NAME_THIS_TEST();

    // A lookup always reporting ERANGE fails once the largest buffer was tried
    std::size_t largest = 0;
    auto function       = [&largest](const char*, int*, char*, std::size_t size, int**) {
        largest = std::max(largest, size);
        return ERANGE;
    };
    int entry   = 0;
    int* result = nullptr;
    std::vector<char> buffer(1024);
    BOOST_CHECK_THROW(detail::lookup_with_growing_buffer(function, "alice", entry, result, buffer), std::runtime_error);
    BOOST_CHECK_EQUAL(largest, detail::max_lookup_buffer_size);
}

BOOST_AUTO_TEST_CASE(test_lookup_errors_are_classified) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(detail::is_not_found(0));
    BOOST_CHECK(detail::is_not_found(ENOENT));
    BOOST_CHECK(!detail::is_not_found(EIO));
    BOOST_CHECK(!detail::is_not_found(EMFILE));
    BOOST_CHECK(!detail::is_not_found(ENOMEM));
}

BOOST_AUTO_TEST_CASE(test_group_list_grows) {
    ECF_NAME_THIS_TEST();

    // A listing that fails until it can hold 100 groups, without reporting the size needed
    std::vector<unsigned> all(100);
    for (unsigned i = 0; i < all.size(); ++i) {
        all[i] = 1000 + i;
    }
    all[50]       = 7; // the primary group, also listed among the others
    int calls     = 0;
    auto function = [&](const char*, unsigned, unsigned* ids, int* count) {
        ++calls;
        if (*count < static_cast<int>(all.size())) {
            return -1;
        }
        std::copy(all.begin(), all.end(), ids);
        *count = static_cast<int>(all.size());
        return static_cast<int>(all.size());
    };
    auto groups = detail::list_groups<unsigned>(function, "alice", 7u);
    BOOST_CHECK_EQUAL(calls, 2); // 64, then 128
    BOOST_REQUIRE_EQUAL(groups.size(), all.size());
    BOOST_CHECK_EQUAL(groups.front(), 7u);
}

BOOST_AUTO_TEST_CASE(test_group_list_is_bounded) {
    ECF_NAME_THIS_TEST();

    auto function = [](const char*, unsigned, unsigned*, int*) { return -1; };
    BOOST_CHECK_THROW(detail::list_groups<unsigned>(function, "alice", 7u), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

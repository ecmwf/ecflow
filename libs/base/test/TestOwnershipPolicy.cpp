// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/server/OwnershipPolicy.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::FakeAccountDatabase;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_OwnershipPolicy)

namespace {

constexpr uid_t server_uid = 500;

std::shared_ptr<const os::AccountDatabase> make_accounts() {
    auto db = std::make_shared<FakeAccountDatabase>();
    db->add_group("root", 0).add_group("ecflow", 500).add_group("users", 100).add_group("ops", 3000);
    db->add_user("root", 0, 0);
    db->add_user("ecflow", server_uid, 500);
    db->add_user("alice", 1001, 100, {3000});
    db->add_user("bob", 1002, 100);
    db->add_user("charlie", 1003, 100);
    return db;
}

struct Fixture
{
    defs_ptr defs = Defs::create();
    suite_ptr s;
    family_ptr f;
    task_ptr t;
    alias_ptr a;
    MockServer server{defs};

    explicit Fixture(bool with_server_owner = false) {
        s = defs->add_suite("s");
        f = s->add_family("f");
        t = f->add_task("t");
        a = t->add_alias_only();
        server.set_account_database(make_accounts(), server_uid);
        if (with_server_owner) {
            defs->server_state().set_server_owner(OwnerAttr::create("charlie"));
        }
    }

    const OwnershipPolicy& policy() const { return server.ownership(); }
};

Identity user(const std::string& name) {
    return Identity::make_user(name, "");
}

void check_allowed(const std::function<void()>& check) {
    BOOST_CHECK_NO_THROW(check());
}

/// Checks that the operation is refused by the policy, on the given node, for the expected reason
void check_refused(const std::function<void()>& check,
                   const std::string& operation,
                   const Node& node,
                   const std::string& expected_in_reason) {
    try {
        check();
        BOOST_ERROR("Expected the operation to be refused (" << expected_in_reason << ")");
    }
    catch (const std::runtime_error& e) {
        const std::string what   = e.what();
        const std::string prefix = "Ownership " + operation + " refused on " + node.absNodePath() + ": ";
        BOOST_CHECK_MESSAGE(what.rfind(prefix, 0) == 0, "Expected the prefix '" << prefix << "' in: " << what);
        BOOST_CHECK_MESSAGE(what.find(expected_in_reason, prefix.size()) != std::string::npos,
                            "Unexpected reason: " << what);
    }
}

/// The outcomes are identical whether ownership is enabled (server Owner) or not (D34)
void for_both_modes(const std::function<void(Fixture&)>& test) {
    for (bool enabled : {false, true}) {
        BOOST_TEST_CONTEXT("ownership " << (enabled ? "enabled" : "disabled")) {
            Fixture fixture(enabled);
            test(fixture);
        }
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(test_create) {
    ECF_NAME_THIS_TEST();

    for_both_modes([](Fixture& x) {
        const auto& p = x.policy();
        check_allowed([&] { p.check_create(user("alice"), *x.f, OwnerAttr::create("alice")); });
        check_allowed([&] { p.check_create(user("alice"), *x.f, OwnerAttr::create("alice:ops")); });

        check_refused([&] { p.check_create(user("bob"), *x.f, OwnerAttr::create("alice")); },
                      "create",
                      *x.f,
                      "naming themselves");
        check_refused(
            [&] { p.check_create(user("alice"), *x.a, OwnerAttr::create("alice")); }, "create", *x.a, "alias");
        check_refused(
            [&] { p.check_create(user("root"), *x.f, OwnerAttr::create("root")); }, "create", *x.f, "superuser");
        check_refused([&] { p.check_create(user("ecflow"), *x.f, OwnerAttr::create("ecflow")); },
                      "create",
                      *x.f,
                      "server account");
        check_refused(
            [&] { p.check_create(user("bob"), *x.f, OwnerAttr::create("bob:ops")); }, "create", *x.f, "not a member");
        check_refused(
            [&] { p.check_create(user("dave"), *x.f, OwnerAttr::create("dave")); }, "create", *x.f, "unknown user");
        check_refused([&] { p.check_create(Identity::make_task("1", "pass", "1"), *x.f, OwnerAttr::create("alice")); },
                      "create",
                      *x.f,
                      "only users can manage Owners");

        x.f->addOwner(OwnerAttr::create("bob"));
        check_refused([&] { p.check_create(user("alice"), *x.f, OwnerAttr::create("alice")); },
                      "create",
                      *x.f,
                      "already has an Owner");
        // An Owner above does not prevent another one further down
        check_allowed([&] { p.check_create(user("alice"), *x.t, OwnerAttr::create("alice")); });
    });
}

BOOST_AUTO_TEST_CASE(test_owners_introduced_while_disabled_are_reported) {
    ECF_NAME_THIS_TEST();

    const std::string log_path = "test_owners_introduced_while_disabled_are_reported.log";
    auto log_content           = [&log_path] {
        Log::instance()->flush();
        std::ifstream in(log_path);
        return std::string{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    };

    for (bool enabled : {false, true}) {
        BOOST_TEST_CONTEXT("ownership " << (enabled ? "enabled" : "disabled")) {
            ecf::test::scaffold::TestLog test_log{log_path};
            Fixture x;
            x.server.set_ownership_enabled(enabled);
            const auto& p = x.policy();

            // The checks report nothing: the commands report what they brought in, once their change is made
            p.check_create(user("alice"), *x.f, OwnerAttr::create("alice"));
            x.t->addOwner(OwnerAttr::create("alice"));
            p.check_update(user("alice"), *x.t, OwnerAttr::create("bob"));
            BOOST_CHECK_MESSAGE(log_content().find("Ownership is disabled") == std::string::npos, log_content());

            p.report_addition(OwnerAttr::create("alice"), "/s/f", "create");
            ownership::ChangeSet changes;
            changes.added.push_back({"/s/f/t", OwnerAttr::create("bob")});
            p.report_additions(changes, "load");

            const auto log = log_content();
            const std::string create =
                "Ownership is disabled, but create brings in the Owner 'alice' of /s/f: its commands run as the server "
                "account";
            const std::string load = "Ownership is disabled, but load brings in the Owner 'bob' of /s/f/t";
            BOOST_CHECK_MESSAGE((log.find(create) != std::string::npos) == !enabled, log);
            BOOST_CHECK_MESSAGE((log.find(load) != std::string::npos) == !enabled, log);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_update) {
    ECF_NAME_THIS_TEST();

    for_both_modes([](Fixture& x) {
        const auto& p = x.policy();
        check_refused(
            [&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("bob")); }, "update", *x.f, "has no Owner");

        x.f->addOwner(OwnerAttr::create("alice"));
        // The current owner may hand the node over to any eligible user (D3)
        check_allowed([&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("bob")); });
        check_allowed([&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("alice:ops")); });

        check_refused(
            [&] { p.check_update(user("bob"), *x.f, OwnerAttr::create("bob")); }, "update", *x.f, "current owner");
        check_refused(
            [&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("root")); }, "update", *x.f, "superuser");
        check_refused([&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("ecflow")); },
                      "update",
                      *x.f,
                      "server account");
        check_refused(
            [&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("bob:ops")); }, "update", *x.f, "not a member");
        check_refused(
            [&] { p.check_update(user("alice"), *x.f, OwnerAttr::create("dave")); }, "update", *x.f, "unknown user");

        // Inheriting an Owner is not having one
        check_refused(
            [&] { p.check_update(user("alice"), *x.t, OwnerAttr::create("bob")); }, "update", *x.t, "has no Owner");
    });
}

BOOST_AUTO_TEST_CASE(test_update_with_a_group_of_the_current_owner) {
    ECF_NAME_THIS_TEST();

    // The group of the current Owner does not matter: only its user does
    Fixture x;
    x.f->addOwner(OwnerAttr::create("alice:ops"));
    check_allowed([&] { x.policy().check_update(user("alice"), *x.f, OwnerAttr::create("alice")); });
}

BOOST_AUTO_TEST_CASE(test_delete) {
    ECF_NAME_THIS_TEST();

    for_both_modes([](Fixture& x) {
        const auto& p = x.policy();
        check_refused([&] { p.check_delete(user("alice"), *x.f); }, "delete", *x.f, "has no Owner");

        x.f->addOwner(OwnerAttr::create("alice"));
        check_allowed([&] { p.check_delete(user("alice"), *x.f); });
        check_refused([&] { p.check_delete(user("bob"), *x.f); }, "delete", *x.f, "current owner");
        check_refused([&] { p.check_delete(user("alice"), *x.t); }, "delete", *x.t, "has no Owner");
    });
}

BOOST_AUTO_TEST_CASE(test_delete_of_an_owner_no_longer_eligible) {
    ECF_NAME_THIS_TEST();

    // The account checks only apply to Owners being introduced: an Owner that is no longer eligible can be removed
    Fixture x;
    x.f->addOwner(OwnerAttr::create("dave"));
    check_allowed([&] { x.policy().check_delete(user("dave"), *x.f); });
}

BOOST_AUTO_TEST_CASE(test_unknown_group) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    check_refused([&] { x.policy().check_create(user("alice"), *x.f, OwnerAttr::create("alice:nogroup")); },
                  "create",
                  *x.f,
                  "unknown group");
    x.f->addOwner(OwnerAttr::create("alice"));
    check_refused([&] { x.policy().check_update(user("alice"), *x.f, OwnerAttr::create("alice:nogroup")); },
                  "update",
                  *x.f,
                  "unknown group");
}

BOOST_AUTO_TEST_CASE(test_account_database_failure_refuses) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    auto accounts = std::make_shared<FakeAccountDatabase>();
    accounts->add_group("users", 100).add_user("alice", 1001, 100).fail_lookups();
    x.server.set_account_database(accounts, server_uid);

    // Fail closed, with the context of the operation
    check_refused([&] { x.policy().check_create(user("alice"), *x.f, OwnerAttr::create("alice")); },
                  "create",
                  *x.f,
                  "could not check the Owner 'alice': Could not look up 'alice'");
    x.f->addOwner(OwnerAttr::create("alice"));
    check_refused([&] { x.policy().check_update(user("alice"), *x.f, OwnerAttr::create("alice")); },
                  "update",
                  *x.f,
                  "could not check the Owner 'alice'");
    // Removing an Owner needs no lookup
    check_allowed([&] { x.policy().check_delete(user("alice"), *x.f); });
}

BOOST_AUTO_TEST_CASE(test_account_database_is_required) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_THROW(OwnershipPolicy(nullptr, server_uid), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

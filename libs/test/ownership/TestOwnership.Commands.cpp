// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PrivilegedEnvironment.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf::test::ownership;

BOOST_AUTO_TEST_SUITE(S_Ownership)

BOOST_AUTO_TEST_SUITE(T_Commands)

namespace {

///
/// Definitions with suite s (family f, task t), with the given Owners on the suite and the family, if any
///
defs_ptr make_defs(const std::string& suite, const std::string& suite_owner = "", const std::string& f_owner = "") {
    defs_ptr defs = Defs::create();
    suite_ptr s   = defs->add_suite(suite);
    family_ptr f  = s->add_family("f");
    f->add_task("t");
    if (!suite_owner.empty()) {
        s->addOwner(ecf::OwnerAttr::create(suite_owner));
    }
    if (!f_owner.empty()) {
        f->addOwner(ecf::OwnerAttr::create(f_owner));
    }
    return defs;
}

/// The reply of a read of the ownership of a node
std::string read_reply(const std::string& owner, const std::string& effective, const std::string& origin) {
    return "owner=" + owner + "\neffective=" + effective + "\norigin=" + origin + "\n";
}

///
/// A server with ownership enabled, and a client per user
///
struct Fixture
{
    Fixture() { server.require_ready(); }

    /// The client of the given user
    ClientInvoker& as(const std::string& user) {
        server.configure(client, user);
        return client;
    }

    /// The definitions of the server, as text
    std::string defs() {
        BOOST_REQUIRE_MESSAGE(as(alice).getDefs() == 0, client.errorMsg());
        return ecf::as_string(*client.defs(), PrintStyle::DEFS);
    }

    /// Checks the ownership of a node, as read by the given user
    void check_read(const std::string& user, const std::string& path, const std::string& expected) {
        BOOST_REQUIRE_MESSAGE(as(user).ownership("read", path) == 0, client.errorMsg());
        BOOST_CHECK_EQUAL(client.get_string(), expected);
    }

    /// Runs a command as the given user, which succeeds
    void check_accepted(const std::string& user, const std::function<int(ClientInvoker&)>& command) {
        BOOST_CHECK_MESSAGE(command(as(user)) == 0, user << ": " << client.errorMsg());
    }

    /// Runs a command as the given user, which is refused for the expected reason without changing the definitions
    void check_refused(const std::string& user,
                       const std::function<int(ClientInvoker&)>& command,
                       const std::string& expected_reason) {
        const auto before = defs();
        BOOST_CHECK_MESSAGE(command(as(user)) == 1, "Expected " << user << " to be refused");
        BOOST_CHECK_MESSAGE(client.errorMsg().find(expected_reason) != std::string::npos,
                            "Expected '" << expected_reason << "'; error: " << client.errorMsg());
        BOOST_CHECK_EQUAL(defs(), before);
    }

    LaunchedServer server{Launch{}};
    ClientInvoker client;
};

auto ownership(const std::string& op, const std::string& path, const std::string& value = "") {
    return [=](ClientInvoker& c) { return c.ownership(op, path, value); };
}

} // namespace

BOOST_FIXTURE_TEST_CASE(test_ownership_operations_by_several_accounts, Fixture) {
    ECF_NAME_THIS_TEST();

    check_accepted(alice, [](ClientInvoker& c) { return c.load(make_defs("a")); });
    check_accepted(alice, [](ClientInvoker& c) { return c.load(make_defs("b")); });

    // create: users claim ownership for themselves only
    check_accepted(alice, ownership("create", "/a", alice));
    check_read(bob, "/a/f/t", read_reply("none", alice, "/a"));
    check_refused(bob, ownership("create", "/b", alice), "users can only create an Owner naming themselves");
    check_refused(alice, ownership("create", "/a", alice), "the node already has an Owner");

    // nested Owners: the closest one applies
    check_accepted(bob, ownership("create", "/a/f", bob));
    check_read(charlie, "/a/f/t", read_reply("none", bob, "/a/f"));

    // groups: the user must be a member of the group
    check_accepted(charlie, ownership("create", "/b", charlie + ":" + owners_group));
    check_read(alice, "/b", read_reply(charlie + ":" + owners_group, charlie + ":" + owners_group, "/b"));
    check_refused(alice, ownership("create", "/b/f", alice + ":" + bob), "is not a member of the group 'bob'");

    // ineligible accounts
    check_refused(server_account,
                  ownership("create", "/b/f", server_account),
                  "The owner '" + server_account + "' is the server account");

    // update and delete: only by the current owner, who may hand over the Owner to anyone (D3)
    check_refused(bob, ownership("update", "/a", bob), "only the current owner 'alice' can change it, not 'bob'");
    check_accepted(alice, ownership("update", "/a", bob));
    check_read(alice, "/a", read_reply(bob, bob, "/a"));
    check_refused(alice, ownership("delete", "/a"), "only the current owner 'bob' can change it, not 'alice'");
    check_accepted(bob, ownership("delete", "/a"));
    check_read(alice, "/a", read_reply("none", server_owner, "server"));
    check_refused(bob, ownership("delete", "/a"), "the node has no Owner");
}

BOOST_FIXTURE_TEST_CASE(test_load_and_replace_by_several_accounts, Fixture) {
    ECF_NAME_THIS_TEST();

    // load: own Owners only
    check_accepted(alice, [](ClientInvoker& c) { return c.load(make_defs("s", alice)); });
    check_read(bob, "/s/f/t", read_reply("none", alice, "/s"));
    check_refused(
        bob,
        [](ClientInvoker& c) { return c.load(make_defs("u", alice)); },
        "users can only bring in Owners naming "
        "themselves, but 'bob' brings in 'alice'");

    // load --force: replacing the suite of another owner, even with an identical Owner, is refused (D27)
    check_refused(
        bob,
        [](ClientInvoker& c) { return c.load(make_defs("s", alice), true); },
        "only the current owner 'alice' can remove or replace its Owner, not 'bob'");
    check_refused(
        bob,
        [](ClientInvoker& c) { return c.load(make_defs("s"), true); },
        "only the current owner 'alice' can remove or replace its Owner, not 'bob'");
    check_accepted(alice,
                   [](ClientInvoker& c) { return c.load(make_defs("s", alice, alice + ":" + owners_group), true); });
    check_read(bob, "/s/f/t", read_reply("none", alice + ":" + owners_group, "/s/f"));

    // replace: the replaced Owners must be the requester's, and so must the incoming ones
    check_refused(
        bob,
        [](ClientInvoker& c) { return c.replace_1("/s/f", make_defs("s", "", bob)); },
        "only the current owner 'alice' can remove or replace its Owner, not 'bob'");
    check_refused(
        alice,
        [](ClientInvoker& c) { return c.replace_1("/s/f", make_defs("s", "", bob)); },
        "users can only bring in Owners naming themselves, but 'alice' brings in 'bob'");
    check_accepted(alice, [](ClientInvoker& c) { return c.replace_1("/s/f", make_defs("s")); });
    check_read(bob, "/s/f/t", read_reply("none", alice, "/s"));

    // a node without Owner may be replaced by another user, who brings in their own Owner (nested Owners)
    check_accepted(bob, [](ClientInvoker& c) { return c.replace_1("/s/f", make_defs("s", "", bob)); });
    check_read(alice, "/s/f/t", read_reply("none", bob, "/s/f"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

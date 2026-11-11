// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/cts/user/OwnershipCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::FakeAccountDatabase;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_OwnershipCmd)

namespace {

constexpr uid_t server_uid = 500;

struct Fixture
{
    ecf::test::scaffold::TestLog test_log{"test_ownership_cmd.log"};
    defs_ptr defs = Defs::create();
    MockServer server{defs};

    Fixture() {
        auto s = defs->add_suite("s");
        auto f = s->add_family("f");
        f->add_task("t");
        auto accounts = std::make_shared<FakeAccountDatabase>();
        accounts->add_group("ecflow", 500).add_group("users", 100);
        accounts->add_user("ecflow", server_uid, 500).add_user("alice", 1001, 100).add_user("bob", 1002, 100);
        server.set_account_database(accounts, server_uid);
    }

    /// Handles the command as the given user, returning the reply text; refusals are reported as exceptions
    std::string
    handle(const std::string& user, OwnershipCmd::Op op, const std::string& path, const std::string& value = "") {
        OwnershipCmd cmd(op, path, value);
        cmd.set_identity(Identity::make_user(user, ""));
        cmd.setup_user_authentification(user, "");
        STC_Cmd_ptr reply = cmd.handleRequest(&server);
        BOOST_REQUIRE(reply);
        BOOST_REQUIRE_MESSAGE(reply->ok(), reply->error());
        return reply->get_string();
    }

    /// Checks that the command is refused for the expected reason, leaving the definitions unchanged
    void check_refused(const std::string& user,
                       OwnershipCmd::Op op,
                       const std::string& path,
                       const std::string& value,
                       const std::string& expected_in_reason) {
        const std::string before = ecf::as_string(*defs, PrintStyle::DEFS);
        BOOST_CHECK_EXCEPTION(handle(user, op, path, value), std::runtime_error, [&](const std::runtime_error& e) {
            bool found = std::string(e.what()).find(expected_in_reason) != std::string::npos;
            BOOST_CHECK_MESSAGE(found, "Unexpected reason: " << e.what());
            return found;
        });
        BOOST_CHECK_EQUAL(ecf::as_string(*defs, PrintStyle::DEFS), before);
    }

    const OwnerAttr* owner_of(const std::string& path) const { return defs->findAbsNode(path)->get_owner(); }
};

} // namespace

BOOST_AUTO_TEST_CASE(test_construction_validates_arguments) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_NO_THROW(OwnershipCmd(OwnershipCmd::Op::CREATE, "/s", "alice"));
    BOOST_CHECK_NO_THROW(OwnershipCmd(OwnershipCmd::Op::UPDATE, "/s", "alice:users"));
    BOOST_CHECK_NO_THROW(OwnershipCmd(OwnershipCmd::Op::READ, "/s"));
    BOOST_CHECK_NO_THROW(OwnershipCmd(OwnershipCmd::Op::READ, "/"));
    BOOST_CHECK_NO_THROW(OwnershipCmd(OwnershipCmd::Op::DELETE, "/s"));

    // a value is required for create and update, and rejected for read and delete
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::CREATE, "/s"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::UPDATE, "/s"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::READ, "/s", "alice"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::DELETE, "/s", "alice"), std::runtime_error);
    // the value must be a valid Owner, and the path absolute
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::CREATE, "/s", "1000"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::CREATE, "/s", "alice:"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::READ, "s"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::READ, ""), std::runtime_error);
    // the server Owner is configuration: only read
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::CREATE, "/", "alice"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::UPDATE, "/", "alice"), std::runtime_error);
    BOOST_CHECK_THROW(OwnershipCmd(OwnershipCmd::Op::DELETE, "/"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_operation_names) {
    ECF_NAME_THIS_TEST();

    for (auto op :
         {OwnershipCmd::Op::CREATE, OwnershipCmd::Op::READ, OwnershipCmd::Op::UPDATE, OwnershipCmd::Op::DELETE}) {
        auto parsed = OwnershipCmd::to_op(OwnershipCmd::to_string(op));
        BOOST_REQUIRE(parsed.has_value());
        BOOST_CHECK(*parsed == op);
    }
    BOOST_CHECK(!OwnershipCmd::to_op("change").has_value());
    BOOST_CHECK(!OwnershipCmd::to_op("CREATE").has_value());
}

BOOST_AUTO_TEST_CASE(test_only_reads_are_read_only) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(!OwnershipCmd(OwnershipCmd::Op::READ, "/s").isWrite());
    BOOST_CHECK(OwnershipCmd(OwnershipCmd::Op::CREATE, "/s", "alice").isWrite());
    BOOST_CHECK(OwnershipCmd(OwnershipCmd::Op::UPDATE, "/s", "alice").isWrite());
    BOOST_CHECK(OwnershipCmd(OwnershipCmd::Op::DELETE, "/s").isWrite());
}

BOOST_AUTO_TEST_CASE(test_read) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/s/f/t"), "owner=none\neffective=none\norigin=none\n");
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/"), "effective=none\norigin=none\n");

    x.defs->server_state().set_server_owner(OwnerAttr::create("ecflow-default:ecflow"));
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/s/f/t"),
                      "owner=none\neffective=ecflow-default:ecflow\norigin=server\n");
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/"), "effective=ecflow-default:ecflow\norigin=server\n");

    x.defs->findAbsNode("/s/f")->addOwner(OwnerAttr::create("alice:users"));
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/s/f"),
                      "owner=alice:users\neffective=alice:users\norigin=/s/f\n");
    BOOST_CHECK_EQUAL(x.handle("bob", OwnershipCmd::Op::READ, "/s/f/t"),
                      "owner=none\neffective=alice:users\norigin=/s/f\n");

    BOOST_CHECK_THROW(x.handle("bob", OwnershipCmd::Op::READ, "/s/nothing"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_create_update_delete) {
    ECF_NAME_THIS_TEST();

    Fixture x;

    // create, by the user named
    unsigned int state_change_no = Ecf::state_change_no();
    x.handle("alice", OwnershipCmd::Op::CREATE, "/s/f", "alice");
    BOOST_REQUIRE(x.owner_of("/s/f"));
    BOOST_CHECK_EQUAL(x.owner_of("/s/f")->value(), "alice");
    BOOST_CHECK_MESSAGE(Ecf::state_change_no() != state_change_no, "The change must be visible to clients");
    x.check_refused("bob", OwnershipCmd::Op::CREATE, "/s/f/t", "alice", "naming themselves");
    x.check_refused("alice", OwnershipCmd::Op::CREATE, "/s/f", "alice", "already has an Owner");

    // update, by the current owner only, to another user
    x.check_refused("bob", OwnershipCmd::Op::UPDATE, "/s/f", "bob", "current owner");
    x.check_refused("alice", OwnershipCmd::Op::UPDATE, "/s/f", "ecflow", "server account");
    x.handle("alice", OwnershipCmd::Op::UPDATE, "/s/f", "bob:users");
    BOOST_CHECK_EQUAL(x.owner_of("/s/f")->value(), "bob:users");

    // delete, by the current owner only
    x.check_refused("alice", OwnershipCmd::Op::DELETE, "/s/f", "", "current owner");
    x.handle("bob", OwnershipCmd::Op::DELETE, "/s/f");
    BOOST_CHECK(x.owner_of("/s/f") == nullptr);
    x.check_refused("bob", OwnershipCmd::Op::DELETE, "/s/f", "", "has no Owner");
    x.check_refused("bob", OwnershipCmd::Op::UPDATE, "/s/f", "bob", "has no Owner");

    // unknown node
    x.check_refused("alice", OwnershipCmd::Op::CREATE, "/s/nothing", "alice", "/s/nothing");
}

BOOST_AUTO_TEST_CASE(test_changes_recorded_in_edit_history) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    x.handle("alice", OwnershipCmd::Op::CREATE, "/s/f", "alice");
    x.handle("alice", OwnershipCmd::Op::READ, "/s/f");

    const auto& history = x.defs->get_edit_history("/s/f");
    BOOST_REQUIRE_EQUAL(history.size(), 1u); // reads are not recorded
    BOOST_CHECK_MESSAGE(history.front().find("--ownership=create /s/f alice") != std::string::npos, history.front());
    BOOST_CHECK_MESSAGE(history.front().find("alice") != std::string::npos, history.front());
}

BOOST_AUTO_TEST_CASE(test_server_owner_cannot_be_changed) {
    ECF_NAME_THIS_TEST();

    // Commands built without the client-side checks (e.g. received from another client) are checked again
    Fixture x;
    for (auto op : {OwnershipCmd::Op::CREATE, OwnershipCmd::Op::UPDATE, OwnershipCmd::Op::DELETE}) {
        auto cmd = OwnershipCmd::make_unchecked(op, "/", op == OwnershipCmd::Op::DELETE ? "" : "alice");
        cmd.set_identity(Identity::make_user("alice", ""));
        BOOST_CHECK_EXCEPTION(cmd.handleRequest(&x.server), std::runtime_error, [](const std::runtime_error& e) {
            return std::string(e.what()).find("server Owner") != std::string::npos;
        });
    }
    BOOST_CHECK(x.defs->server_state().server_owner() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_invalid_value_received_is_rejected) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    auto cmd = OwnershipCmd::make_unchecked(OwnershipCmd::Op::CREATE, "/s/f", "alice:");
    cmd.set_identity(Identity::make_user("alice", ""));
    BOOST_CHECK_THROW(cmd.handleRequest(&x.server), std::runtime_error);
    BOOST_CHECK(x.owner_of("/s/f") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_unknown_operation_received_is_rejected) {
    ECF_NAME_THIS_TEST();

    Fixture x;
    auto cmd = OwnershipCmd::make_unchecked(static_cast<OwnershipCmd::Op>(7), "/s/f", "");
    cmd.set_identity(Identity::make_user("alice", ""));
    BOOST_CHECK_EXCEPTION(cmd.handleRequest(&x.server), std::runtime_error, [](const std::runtime_error& e) {
        return std::string(e.what()).find("unknown operation 7") != std::string::npos;
    });
    BOOST_CHECK(x.defs->get_edit_history("/s/f").empty());
    BOOST_CHECK(x.owner_of("/s/f") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_printed_as_command_line) {
    ECF_NAME_THIS_TEST();

    std::string text;
    OwnershipCmd(OwnershipCmd::Op::CREATE, "/s/f", "alice:users").print_only(text);
    BOOST_CHECK_EQUAL(text, "--ownership=create /s/f alice:users");
    text.clear();
    OwnershipCmd(OwnershipCmd::Op::READ, "/").print_only(text);
    BOOST_CHECK_EQUAL(text, "--ownership=read /");
}

BOOST_AUTO_TEST_CASE(test_older_server_error_is_explained) {
    ECF_NAME_THIS_TEST();

    // Reply recorded from a server without the ownership command (5.19.0), which cannot decode the request
    const std::string recorded =
        "Error: request( --ownership=read / ) failed! Server reply:  ->Server:iona@9417 (5.19.0) replied with: Invalid "
        "argument\n";
    auto explained = OwnershipCmd::explain_unsupported(recorded);
    BOOST_REQUIRE(explained.has_value());
    BOOST_CHECK_MESSAGE(explained->find("does not support the ownership command") != std::string::npos, *explained);
    BOOST_CHECK_MESSAGE(explained->find(OwnershipCmd::minimum_server_version()) != std::string::npos, *explained);
    BOOST_CHECK_MESSAGE(explained->find(recorded) != std::string::npos, "The original reply must be kept");

    BOOST_CHECK(
        !OwnershipCmd::explain_unsupported("Ownership create refused on /s: the node has no Owner").has_value());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

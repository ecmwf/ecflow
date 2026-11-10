// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/cts/user/PlugCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::FakeAccountDatabase;
using ecf::test::scaffold::TestLog;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_OwnershipChangeSet)

namespace {

constexpr uid_t server_uid = 500;

/// A server holding /s/f/t and /s/other, whose family /s/f is owned by alice; alice and bob are accounts
struct Fixture
{
    std::string log_path;
    TestLog test_log;
    ecf::test::scaffold::Directory ecf_home;
    defs_ptr defs = Defs::create();
    MockServer server{defs};

    explicit Fixture(const std::string& name)
        : log_path(name + ".log"),
          test_log(log_path),
          ecf_home(ecf::test::scaffold::MakeDirectory{}.create()) {
        suite_ptr s = defs->add_suite("s");
        s->add_variable("ECF_HOME", fs::absolute(ecf_home.path()).string());
        family_ptr f = s->add_family("f");
        f->addOwner(OwnerAttr::create("alice"));
        f->add_task("t");
        s->add_family("other");

        auto accounts = std::make_shared<FakeAccountDatabase>();
        accounts->add_group("ecflow", 500).add_group("users", 100);
        accounts->add_user("ecflow", server_uid, 500).add_user("alice", 1001, 100).add_user("bob", 1002, 100);
        server.set_account_database(accounts, server_uid);
    }

    std::string text() const { return ecf::as_string(*defs, PrintStyle::DEFS); }

    std::string log() const {
        Log::instance()->flush();
        std::ifstream in(log_path);
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    void run(ClientToServerCmd& cmd, const std::string& user) {
        cmd.setup_user_authentification(user, "");
        cmd.set_identity(Identity::make_user(user, ""));
        STC_Cmd_ptr reply = cmd.handleRequest(&server);
        BOOST_REQUIRE_MESSAGE(reply->ok(), "Unexpected error: " << reply->error());
    }

    /// Checks that the command is refused for the expected reason, leaving the definitions and zombies unchanged
    void check_refused(ClientToServerCmd& cmd, const std::string& user, const std::string& expected_in_reason) {
        const std::string before = text();
        cmd.setup_user_authentification(user, "");
        cmd.set_identity(Identity::make_user(user, ""));
        BOOST_CHECK_EXCEPTION(cmd.handleRequest(&server), std::runtime_error, [&](const std::runtime_error& e) {
            bool found = std::string(e.what()).find(expected_in_reason) != std::string::npos;
            BOOST_CHECK_MESSAGE(found, "Unexpected error: " << e.what());
            return found;
        });
        BOOST_CHECK_EQUAL(text(), before);
        std::vector<Zombie> zombies;
        server.zombie_ctrl().get(zombies);
        BOOST_CHECK(zombies.empty());
    }
};

/// Client definitions: suite s, with family f (Owner as given, if any) holding task t
defs_ptr make_client_defs(const std::string& f_owner = "") {
    defs_ptr defs = Defs::create();
    family_ptr f  = defs->add_suite("s")->add_family("f");
    f->add_task("t");
    if (!f_owner.empty()) {
        f->addOwner(OwnerAttr::create(f_owner));
    }
    return defs;
}

const OwnerAttr* owner_of(const Defs& defs, const std::string& path) {
    node_ptr node = defs.findAbsNode(path);
    BOOST_REQUIRE_MESSAGE(node, "No node " << path);
    return node->get_owner();
}

} // namespace

BOOST_AUTO_TEST_CASE(test_load_of_a_new_suite) {
    ECF_NAME_THIS_TEST();

    Fixture x("test_load_of_a_new_suite");
    auto make = [] {
        defs_ptr defs = Defs::create();
        defs->add_suite("n")->add_family("g")->addOwner(OwnerAttr::create("alice:users"));
        return defs;
    };

    LoadDefsCmd by_bob(make(), false);
    x.check_refused(by_bob, "bob", "Ownership load refused on /n/g: users can only bring in Owners naming themselves");

    LoadDefsCmd by_ecflow(make(), false);
    x.check_refused(by_ecflow, "ecflow", "naming themselves");

    LoadDefsCmd by_alice(make(), false);
    x.run(by_alice, "alice");
    BOOST_REQUIRE(owner_of(*x.defs, "/n/g"));
    BOOST_CHECK_EQUAL(owner_of(*x.defs, "/n/g")->value(), "alice:users");
}

BOOST_AUTO_TEST_CASE(test_owners_brought_in_while_ownership_is_disabled_are_reported) {
    ECF_NAME_THIS_TEST();

    auto make = [](const std::string& suite) {
        defs_ptr defs = Defs::create();
        defs->add_suite(suite)->addOwner(OwnerAttr::create("alice"));
        return defs;
    };
    const std::string warning = "Ownership is disabled, but load brings in the Owner 'alice' of /";

    {
        // Disabled, as a server without server Owner: a load after start-up is reported, a refused one is not
        Fixture x("test_owners_brought_in_while_ownership_is_disabled_are_reported");
        LoadDefsCmd by_bob(make("refused"), false);
        x.check_refused(by_bob, "bob", "naming themselves");
        // permitted by the ownership rules, but refused by the load itself (the suite exists, and --force is not
        // given): nothing is brought in
        LoadDefsCmd duplicate(make("s"), false);
        x.check_refused(duplicate, "alice", "already exists");
        LoadDefsCmd by_alice(make("n"), false);
        x.run(by_alice, "alice");
        BOOST_CHECK_MESSAGE(x.log().find(warning + "n: its commands run as the server account") != std::string::npos,
                            x.log());
        BOOST_CHECK_MESSAGE(x.log().find(warning + "s:") == std::string::npos, x.log());

        // a load failing part way keeps the suites absorbed before the failure: their Owners are reported
        defs_ptr partial = Defs::create();
        partial->add_suite("m")->addOwner(OwnerAttr::create("alice"));
        partial->add_suite("s")->addOwner(OwnerAttr::create("alice"));
        LoadDefsCmd partially(partial, false);
        partially.setup_user_authentification("alice", "");
        partially.set_identity(Identity::make_user("alice", ""));
        BOOST_CHECK_THROW(partially.handleRequest(&x.server), std::runtime_error);
        BOOST_REQUIRE(x.defs->findSuite("m"));
        BOOST_CHECK_MESSAGE(x.log().find(warning + "m: its commands run as the server account") != std::string::npos,
                            x.log());
        BOOST_CHECK_MESSAGE(x.log().find(warning + "s:") == std::string::npos, x.log());
        BOOST_CHECK_MESSAGE(x.log().find(warning + "refused") == std::string::npos, x.log());
    }
    {
        Fixture x("test_owners_brought_in_while_ownership_is_enabled");
        x.server.set_ownership_enabled(true);
        LoadDefsCmd by_alice(make("n"), false);
        x.run(by_alice, "alice");
        BOOST_CHECK_MESSAGE(x.log().find("Ownership is disabled") == std::string::npos, x.log());
    }
}

BOOST_AUTO_TEST_CASE(test_load_checks_every_owner) {
    ECF_NAME_THIS_TEST();

    // Nested Owners: one of them names another user
    Fixture x("test_load_checks_every_owner");
    defs_ptr incoming = Defs::create();
    suite_ptr n       = incoming->add_suite("n");
    n->addOwner(OwnerAttr::create("alice"));
    n->add_family("g")->add_task("u")->addOwner(OwnerAttr::create("bob"));
    LoadDefsCmd cmd(incoming, false);
    x.check_refused(cmd, "alice", "Ownership load refused on /n/g/u");
}

BOOST_AUTO_TEST_CASE(test_load_of_an_ineligible_owner) {
    ECF_NAME_THIS_TEST();

    Fixture x("test_load_of_an_ineligible_owner");
    defs_ptr incoming = Defs::create();
    incoming->add_suite("n")->addOwner(OwnerAttr::create("ecflow"));
    LoadDefsCmd cmd(incoming, false);
    x.check_refused(cmd, "ecflow", "server account");
}

BOOST_AUTO_TEST_CASE(test_load_force_removes_owners_of_their_user_only) {
    ECF_NAME_THIS_TEST();

    // Replacing suite s removes the Owner of /s/f
    Fixture x("test_load_force_removes_owners_of_their_user_only");
    LoadDefsCmd by_bob(make_client_defs(), true);
    x.check_refused(by_bob, "bob", "Ownership load refused on /s/f: only the current owner 'alice'");

    LoadDefsCmd by_alice(make_client_defs(), true);
    x.run(by_alice, "alice");
    BOOST_CHECK(owner_of(*x.defs, "/s/f") == nullptr);
    BOOST_CHECK_MESSAGE(x.log().find("Owner alice of /s/f removed by alice (load)") != std::string::npos, x.log());
}

BOOST_AUTO_TEST_CASE(test_load_force_with_identical_owner_of_another_user) {
    ECF_NAME_THIS_TEST();

    // D27: even identical to the existing one, an Owner brought in must name the requester
    Fixture x("test_load_force_with_identical_owner_of_another_user");
    LoadDefsCmd by_bob(make_client_defs("alice"), true);
    x.check_refused(by_bob, "bob", "only the current owner 'alice'");

    // By its user, the Owner is kept, and its removal is not reported
    LoadDefsCmd by_alice(make_client_defs("alice"), true);
    x.run(by_alice, "alice");
    BOOST_REQUIRE(owner_of(*x.defs, "/s/f"));
    BOOST_CHECK_EQUAL(owner_of(*x.defs, "/s/f")->value(), "alice");
    BOOST_CHECK(x.log().find("removed by") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_load_force_checks_owners_brought_in_after_removals) {
    ECF_NAME_THIS_TEST();

    // alice may remove her Owner, but not replace it with one naming bob
    Fixture x("test_load_force_checks_owners_brought_in_after_removals");
    LoadDefsCmd cmd(make_client_defs("bob"), true);
    x.check_refused(cmd, "alice", "Ownership load refused on /s/f: users can only bring in Owners naming themselves");
}

BOOST_AUTO_TEST_CASE(test_load_force_over_an_unreadable_archive) {
    ECF_NAME_THIS_TEST();

    // An archive that cannot be read cannot be restored either: it does not prevent replacing its suite
    Fixture x("test_load_force_over_an_unreadable_archive");
    x.defs->findAbsNode("/s/f")->deleteOwner();
    auto* f                     = x.defs->findAbsNode("/s/f")->isNodeContainer();
    const std::string f_archive = f->archive_path();
    f->archive();
    {
        std::ofstream corrupt(f_archive, std::ios::trunc);
        corrupt << "not an archive";
    }

    LoadDefsCmd cmd(make_client_defs(), true);
    x.run(cmd, "someone-unknown");
    BOOST_CHECK(!x.defs->findAbsNode("/s/f")->get_flag().is_set(ecf::Flag::ARCHIVED));
    BOOST_CHECK_MESSAGE(x.log().find("Could not read the archive of /s/f") != std::string::npos, x.log());
}

BOOST_AUTO_TEST_CASE(test_replace_of_an_owned_node) {
    ECF_NAME_THIS_TEST();

    Fixture x("test_replace_of_an_owned_node");
    ReplaceNodeCmd by_bob("/s/f", false, make_client_defs(), false);
    x.check_refused(by_bob, "bob", "Ownership replace refused on /s/f: only the current owner 'alice'");

    ReplaceNodeCmd by_alice("/s/f", false, make_client_defs(), false);
    x.run(by_alice, "alice");
    BOOST_CHECK(owner_of(*x.defs, "/s/f") == nullptr);
    BOOST_CHECK_MESSAGE(x.log().find("Owner alice of /s/f removed by alice (replace)") != std::string::npos, x.log());
}

BOOST_AUTO_TEST_CASE(test_replace_of_the_parent_of_an_owned_node) {
    ECF_NAME_THIS_TEST();

    Fixture x("test_replace_of_the_parent_of_an_owned_node");
    ReplaceNodeCmd cmd("/s", false, make_client_defs(), false);
    x.check_refused(cmd, "bob", "Ownership replace refused on /s/f");
}

BOOST_AUTO_TEST_CASE(test_replace_with_created_parents) {
    ECF_NAME_THIS_TEST();

    // The missing parents are created from the client definitions, with their Owners
    Fixture x("test_replace_with_created_parents");
    defs_ptr incoming = Defs::create();
    family_ptr g      = incoming->add_suite("s")->add_family("g");
    g->addOwner(OwnerAttr::create("bob"));
    g->add_task("u");

    ReplaceNodeCmd by_alice("/s/g/u", true, incoming, false);
    x.check_refused(by_alice, "alice", "Ownership replace refused on /s/g: users can only bring in Owners");

    ReplaceNodeCmd by_bob("/s/g/u", true, incoming, false);
    x.run(by_bob, "bob");
    BOOST_REQUIRE(owner_of(*x.defs, "/s/g"));
    BOOST_CHECK_EQUAL(owner_of(*x.defs, "/s/g")->value(), "bob");
}

BOOST_AUTO_TEST_CASE(test_refused_replace_creates_no_zombie) {
    ECF_NAME_THIS_TEST();

    // A forced replace of active tasks creates zombies: not when refused
    Fixture x("test_refused_replace_creates_no_zombie");
    x.defs->findAbsNode("/s/f/t")->set_state(NState::ACTIVE);
    ReplaceNodeCmd cmd("/s/f", false, make_client_defs(), true);
    x.check_refused(cmd, "bob", "only the current owner 'alice'");
}

BOOST_AUTO_TEST_CASE(test_move_into_the_server) {
    ECF_NAME_THIS_TEST();

    // The destination of a remote plug checks the Owners brought in, as for the forwarded user
    Fixture x("test_move_into_the_server");
    defs_ptr source = Defs::create();
    family_ptr g    = source->add_suite("elsewhere")->add_family("g");
    g->addOwner(OwnerAttr::create("bob"));

    MoveCmd by_alice(std::make_pair(std::string("localhost"), std::string("3141")), g.get(), "/s/other");
    x.check_refused(by_alice, "alice", "Ownership plug refused on /s/other/g: users can only bring in Owners");

    MoveCmd by_bob(std::make_pair(std::string("localhost"), std::string("3141")), g.get(), "/s/other");
    x.run(by_bob, "bob");
    BOOST_REQUIRE(owner_of(*x.defs, "/s/other/g"));
    BOOST_CHECK_EQUAL(owner_of(*x.defs, "/s/other/g")->value(), "bob");
}

BOOST_AUTO_TEST_CASE(test_remote_plug_of_owners_of_another_user) {
    ECF_NAME_THIS_TEST();

    // A destination on another server: the plug is refused before any connection is attempted
    Fixture x("test_remote_plug_of_owners_of_another_user");
    PlugCmd cmd("/s/f", "//ecflow-no-such-host.invalid:3141/other");
    x.check_refused(cmd, "bob", "Ownership plug refused on /s/f: only the current owner 'alice'");
}

BOOST_AUTO_TEST_CASE(test_remote_plug_of_archived_owners_of_another_user) {
    ECF_NAME_THIS_TEST();

    // The Owner of /s/f/g/u is only in the archive of /s/f/g, itself inside the archive of /s/f
    Fixture x("test_remote_plug_of_archived_owners_of_another_user");
    x.defs->findAbsNode("/s/f")->deleteOwner();
    family_ptr g = x.defs->findAbsNode("/s/f")->isFamily()->add_family("g");
    g->add_task("u")->addOwner(OwnerAttr::create("alice"));
    auto* f                     = x.defs->findAbsNode("/s/f")->isNodeContainer();
    const std::string g_archive = g->archive_path();
    const std::string f_archive = f->archive_path();
    g->archive();
    f->archive();
    BOOST_REQUIRE(fs::exists(g_archive));
    BOOST_REQUIRE(fs::exists(f_archive));

    PlugCmd cmd("/s/f", "//ecflow-no-such-host.invalid:3141/other");
    x.check_refused(cmd, "bob", "Ownership plug refused on /s/f/g/u: only the current owner 'alice'");

    // Nothing was restored: the tree and both archive files are intact
    BOOST_CHECK(f->get_flag().is_set(ecf::Flag::ARCHIVED));
    BOOST_CHECK(f->children().empty());
    BOOST_CHECK(fs::exists(f_archive));
    BOOST_CHECK(fs::exists(g_archive));
}

BOOST_AUTO_TEST_CASE(test_failed_remote_plug_archives_the_source_again) {
    ECF_NAME_THIS_TEST();

    // The move is allowed at the source, and fails at the destination (here, unreachable): the archived nodes
    // restored for the move, at every depth, are archived again
    Fixture x("test_failed_remote_plug_archives_the_source_again");
    family_ptr g = x.defs->findAbsNode("/s/f")->isFamily()->add_family("g");
    g->add_task("u")->addOwner(OwnerAttr::create("alice"));
    family_ptr h = g->add_family("h");
    h->add_task("v");
    auto* f                     = x.defs->findAbsNode("/s/f")->isNodeContainer();
    const std::string f_archive = f->archive_path();
    const std::string g_archive = g->archive_path();
    const std::string h_archive = h->archive_path();
    h->archive();
    g->archive();
    f->archive();
    BOOST_REQUIRE(fs::exists(f_archive));
    BOOST_REQUIRE(fs::exists(g_archive));
    BOOST_REQUIRE(fs::exists(h_archive));

    PlugCmd cmd("/s/f", "//ecflow-no-such-host.invalid:3141/other");
    cmd.setup_user_authentification("alice", "");
    cmd.set_identity(Identity::make_user("alice", ""));
    BOOST_CHECK_THROW(cmd.handleRequest(&x.server), std::runtime_error);

    BOOST_CHECK(x.defs->findAbsNode("/s/f"));
    BOOST_CHECK(f->get_flag().is_set(ecf::Flag::ARCHIVED));
    BOOST_CHECK(f->children().empty());
    BOOST_CHECK(fs::exists(f_archive));
    BOOST_CHECK(fs::exists(g_archive));
    BOOST_CHECK(fs::exists(h_archive));
}

BOOST_AUTO_TEST_CASE(test_plug_failing_to_restore_archives_the_source_again) {
    ECF_NAME_THIS_TEST();

    // The archive of /s/f is valid, but that of /s/f/g, archived within it, is missing: restoring /s/f/g fails, and
    // /s/f, already restored, is archived again, for a remote as for a local plug
    Fixture x("test_plug_failing_to_restore_archives_the_source_again");
    x.defs->add_suite("x");
    family_ptr g = x.defs->findAbsNode("/s/f")->isFamily()->add_family("g");
    g->add_task("u");
    auto* f                     = x.defs->findAbsNode("/s/f")->isNodeContainer();
    const std::string f_archive = f->archive_path();
    const std::string g_archive = g->archive_path();
    g->archive();
    f->archive();
    fs::remove(g_archive);

    for (const std::string destination : {"//ecflow-no-such-host.invalid:3141/other", "/x"}) {
        BOOST_TEST_CONTEXT("destination " << destination) {
            PlugCmd cmd("/s/f", destination);
            cmd.setup_user_authentification("alice", "");
            cmd.set_identity(Identity::make_user("alice", ""));
            BOOST_CHECK_THROW(cmd.handleRequest(&x.server), std::runtime_error);
            BOOST_REQUIRE(x.defs->findAbsNode("/s/f"));
            BOOST_CHECK(f->get_flag().is_set(ecf::Flag::ARCHIVED));
            BOOST_CHECK(f->children().empty());
            BOOST_CHECK(fs::exists(f_archive));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_plug_restoring_an_archive_that_cannot_be_removed) {
    ECF_NAME_THIS_TEST();

    // Restoring a node removes its archive file last: when that fails, the node is still restored, and moved
    if (::geteuid() == 0) {
        BOOST_TEST_MESSAGE("Skipped: the superuser can remove files from a read-only directory");
        return;
    }
    Fixture x("test_plug_restoring_an_archive_that_cannot_be_removed");
    x.defs->add_suite("x");
    auto* f                     = x.defs->findAbsNode("/s/f")->isNodeContainer();
    const std::string f_archive = f->archive_path();
    f->archive();
    BOOST_REQUIRE(fs::exists(f_archive));

    const fs::path home = fs::absolute(x.ecf_home.path());
    fs::permissions(home, fs::perms::owner_read | fs::perms::owner_exec, fs::perm_options::replace);
    PlugCmd cmd("/s/f", "/x");
    cmd.setup_user_authentification("alice", "");
    cmd.set_identity(Identity::make_user("alice", ""));
    STC_Cmd_ptr reply;
    BOOST_CHECK_NO_THROW(reply = cmd.handleRequest(&x.server));
    fs::permissions(home, fs::perms::owner_all, fs::perm_options::replace);

    BOOST_CHECK(reply && reply->ok());
    BOOST_CHECK(x.defs->findAbsNode("/x/f/t"));
    BOOST_CHECK_MESSAGE(x.log().find("restored, but could not remove") != std::string::npos, x.log());
}

BOOST_AUTO_TEST_CASE(test_local_plug_restores_archived_descendants) {
    ECF_NAME_THIS_TEST();

    // An archived family below the moved node is restored, so that its children move with it
    Fixture x("test_local_plug_restores_archived_descendants");
    x.defs->add_suite("x");
    family_ptr g = x.defs->findAbsNode("/s/f")->isFamily()->add_family("g");
    g->add_task("u");
    g->archive();
    BOOST_REQUIRE(g->children().empty());

    PlugCmd cmd("/s/f", "/x");
    x.run(cmd, "bob");
    BOOST_CHECK(x.defs->findAbsNode("/x/f/g/u"));
}

BOOST_AUTO_TEST_CASE(test_ownership_checked_before_locking) {
    ECF_NAME_THIS_TEST();

    // Locking halts the server: a refused plug or move must not take the lock. With the lock already held by
    // another user, the refusal is reported, not the lock.
    Fixture x("test_ownership_checked_before_locking");
    BOOST_REQUIRE(x.server.lock("someone-else"));

    PlugCmd plug("/s/f", "//ecflow-no-such-host.invalid:3141/other");
    x.check_refused(plug, "bob", "Ownership plug refused on /s/f");

    defs_ptr source = Defs::create();
    family_ptr g    = source->add_suite("elsewhere")->add_family("g");
    g->addOwner(OwnerAttr::create("bob"));
    MoveCmd move(std::make_pair(std::string("localhost"), std::string("3141")), g.get(), "/s/other");
    x.check_refused(move, "alice", "Ownership plug refused on /s/other/g");

    x.server.unlock();
}

BOOST_AUTO_TEST_CASE(test_local_plug_is_not_checked) {
    ECF_NAME_THIS_TEST();

    // D35: moving within the server neither adds nor removes Owners
    Fixture x("test_local_plug_is_not_checked");
    x.defs->add_suite("x");
    PlugCmd cmd("/s/f", "/x");
    x.run(cmd, "bob");
    BOOST_REQUIRE(owner_of(*x.defs, "/x/f"));
    BOOST_CHECK_EQUAL(owner_of(*x.defs, "/x/f")->value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_commands_without_owners_are_not_checked) {
    ECF_NAME_THIS_TEST();

    // Without any Owner involved, nothing is looked up, whoever the requester
    Fixture x("test_commands_without_owners_are_not_checked");
    x.defs->findAbsNode("/s/f")->deleteOwner();
    LoadDefsCmd load(make_client_defs(), true);
    x.run(load, "someone-unknown");
    ReplaceNodeCmd replace("/s/f", false, make_client_defs(), false);
    x.run(replace, "someone-unknown");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

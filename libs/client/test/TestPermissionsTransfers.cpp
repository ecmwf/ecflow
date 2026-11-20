// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;
using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The operations that bring nodes with their attributes (R4.3, R4.5): load and replace go through the validation of
/// changes where the nodes go (D2.15, D2.18, D2.20, D4.6), load rejects a server attribute (D4.12), and every change
/// of attributes is logged; after each change, the copies of the clients equal fresh full synchronisations.
///

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};
const Credentials bob{"bob", "bob-secret"};

// admin holds no sticky entry, so that the self-lockout check applies
const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlop"},)"
    R"( {"user": "alice", "permissions": "rwxop"}, {"user": "bob", "permissions": "r"}])";

///
/// @brief Builds definitions holding suite s, with family f and task f/t, and the given attributes.
///
defs_ptr suite_s(const std::string& suite, const std::string& family = {}, const std::string& task = {}) {
    auto defs = Defs::create();
    auto s    = defs->add_suite("s");
    auto f    = s->add_family("f");
    auto t    = f->add_task("t");
    if (!suite.empty()) {
        s->add_permissions(Permissions::parse_cli(suite));
    }
    if (!family.empty()) {
        f->add_permissions(Permissions::parse_cli(family));
    }
    if (!task.empty()) {
        t->add_permissions(Permissions::parse_cli(task));
    }
    return defs;
}

std::vector<std::string> permissions_records(const PermissionsServer& server) {
    std::ifstream log(server.home() / "server.log");
    std::vector<std::string> records;
    for (std::string line; std::getline(log, line);) {
        if (line.rfind("LOG:", 0) == 0 && line.find(" permissions /") != std::string::npos) {
            records.push_back(line.substr(line.find(" permissions /") + 1));
        }
    }
    return records;
}

///
/// @brief Returns the attribute at a path, as read by alice ("-" for none, "missing" when there is no such node).
///
std::string attribute_at(const PermissionsServer& server, const std::string& path) {
    auto client = server.client(alice);
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    const auto node = client->defs()->findAbsNode(path);
    if (!node) {
        return "missing";
    }
    return node->permissions() ? node->permissions()->to_cli() : "-";
}

std::string text_of(const Defs& defs) {
    std::string text;
    defs.write_to_string(text, PrintStyle::DEFS);
    return text;
}

///
/// @brief Holds the clients of a user kept synchronised across changes: one without a handle, one with a handle
///        on the given suites (s by default).
///
struct Copies
{
    const PermissionsServer& server;
    Credentials user;
    std::vector<std::string> suites       = {"s"};
    std::unique_ptr<ClientInvoker> plain  = server.client(user);
    std::unique_ptr<ClientInvoker> handle = server.client(user);

    void start() {
        BOOST_REQUIRE_MESSAGE(plain->sync_local() == 0, plain->errorMsg());
        BOOST_REQUIRE_MESSAGE(handle->ch_register(true, suites) == 0, handle->errorMsg());
        BOOST_REQUIRE_MESSAGE(handle->sync_local() == 0, handle->errorMsg());
    }

    void check(const std::string& step) {
        auto fresh = server.client(user);
        BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
        // The fresh handle holds the suites the kept handle holds now, those added automatically included
        BOOST_REQUIRE_MESSAGE(handle->ch_suites() == 0, handle->errorMsg());
        std::vector<std::string> held;
        for (const auto& [number, names] : handle->server_reply().get_client_handle_suites()) {
            if (static_cast<int>(number) == handle->client_handle()) {
                held = names;
            }
        }
        auto fresh_handle = server.client(user);
        BOOST_REQUIRE_MESSAGE(fresh_handle->ch_register(true, held) == 0, fresh_handle->errorMsg());
        BOOST_REQUIRE_MESSAGE(fresh_handle->sync_local() == 0, fresh_handle->errorMsg());
        const std::vector<std::pair<ClientInvoker*, ClientInvoker*>> pairs{{plain.get(), fresh.get()},
                                                                           {handle.get(), fresh_handle.get()}};
        for (const auto& pair : pairs) {
            BOOST_REQUIRE_MESSAGE(pair.first->sync_local() == 0, pair.first->errorMsg());
            BOOST_CHECK_MESSAGE(text_of(*pair.first->defs()) == text_of(*pair.second->defs()),
                                user.user << ", " << step << ": the copy differs from a fresh synchronisation\n"
                                          << text_of(*pair.first->defs()) << "---\n"
                                          << text_of(*pair.second->defs()));
        }
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsTransfers)

///
/// @brief Verifies the validation of the attributes brought by --load, and the log records of a load.
///
BOOST_AUTO_TEST_CASE(test_load) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    auto client = server.client(alice);

    // A7.8: an entry naming a user unknown to the server attribute rejects the load, naming the user (D2.15)
    BOOST_CHECK(client->load(suite_s("alice:rwxp,dave:r")) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("names the user 'dave'") != std::string::npos, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s"), "missing");

    // A user not listed above the node (D2.18)
    BOOST_CHECK(client->load(suite_s("alice:rwxp", "alice:rp,bob:r")) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("/s/f names the user 'bob'") != std::string::npos, client->errorMsg());

    // A suite nobody could change (D4.6)
    BOOST_CHECK(client->load(suite_s("alice:rwx,bob:r")) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("nobody able to change the Permissions attribute of /s") !=
                            std::string::npos,
                        client->errorMsg());

    // A server attribute in the definitions (D4.12)
    {
        auto defs = suite_s("alice:rwxp");
        defs->server_state().set_permissions(Permissions::parse_cli("alice:rwxlops"));
        BOOST_CHECK(client->load(defs) != 0);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("server Permissions attribute") != std::string::npos,
                            client->errorMsg());
    }
    BOOST_CHECK_EQUAL(attribute_at(server, "/s"), "missing");

    // Several suites, one of them already loaded, without --force: nothing is loaded, nothing logged
    {
        auto first = Defs::create();
        first->add_suite("s");
        BOOST_REQUIRE_MESSAGE(client->load(first) == 0, client->errorMsg());
        auto defs = Defs::create();
        defs->add_suite("n")->add_permissions(Permissions::parse_cli("alice:rwxp"));
        defs->add_suite("s");
        BOOST_CHECK(client->load(defs) != 0);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("already exists") != std::string::npos, client->errorMsg());
        BOOST_CHECK_EQUAL(attribute_at(server, "/n"), "missing");
        BOOST_CHECK(permissions_records(server).empty());
        BOOST_REQUIRE_MESSAGE(client->delete_node("/s") == 0, client->errorMsg());
    }

    // A valid load, then a load --force replacing the suite: the attributes changed are logged
    BOOST_REQUIRE_MESSAGE(client->load(suite_s("alice:rwxp,bob:r", "alice:rwxp,bob:r")) == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "alice:rwxp,bob:r");
    Copies copies_of_alice{server, alice};
    Copies copies_of_bob{server, bob};
    copies_of_alice.start();
    copies_of_bob.start();

    BOOST_REQUIRE_MESSAGE(client->load(suite_s("alice:rwxp,bob:r", "", "alice:rwxp"), true) == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "-");
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f/t"), "alice:rwxp");
    copies_of_alice.check("load --force");
    copies_of_bob.check("load --force");

    const auto records = permissions_records(server);
    const std::vector<std::string> expected{
        "permissions /s (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s/f (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s/f alice:rwxp,bob:r -> (none) by alice@",
        "permissions /s/f/t (none) -> alice:rwxp by alice@",
    };
    BOOST_REQUIRE_EQUAL(records.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        BOOST_CHECK_MESSAGE(records[i].rfind(expected[i], 0) == 0,
                            records[i] << " does not start with " << expected[i]);
    }

    // load --force removing the attributes of a suite: logged as removed, and bob reads /s again from the server
    BOOST_REQUIRE_MESSAGE(client->load(suite_s(""), true) == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s"), "-");
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f/t"), "-");
    copies_of_alice.check("load --force without attributes");
    copies_of_bob.check("load --force without attributes");
    const auto last = permissions_records(server);
    BOOST_REQUIRE_EQUAL(last.size(), expected.size() + 2);
    BOOST_CHECK_MESSAGE(last[4].rfind("permissions /s alice:rwxp,bob:r -> (none) by alice@", 0) == 0, last[4]);
    BOOST_CHECK_MESSAGE(last[5].rfind("permissions /s/f/t alice:rwxp -> (none) by alice@", 0) == 0, last[5]);
}

///
/// @brief Verifies the validation of the attributes brought by --replace, at the node replaced and at the nodes
///        created with their parents.
///
BOOST_AUTO_TEST_CASE(test_replace) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    auto client = server.client(alice);
    BOOST_REQUIRE_MESSAGE(client->load(suite_s("alice:rwxp,bob:r")) == 0, client->errorMsg());
    Copies copies_of_alice{server, alice};
    Copies copies_of_bob{server, bob};
    copies_of_alice.start();
    copies_of_bob.start();

    // A family naming a user not listed above it, or not known to the server (D2.15, D2.18)
    BOOST_CHECK(client->replace_1("/s/f", suite_s("", "alice:rwxp,carol:r"), false) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("/s/f names the user 'carol'") != std::string::npos,
                        client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "-");

    // A rejected replace --force over an active task: no zombie, the task unchanged
    {
        BOOST_REQUIRE_MESSAGE(client->force("/s/f/t", "active") == 0, client->errorMsg());
        BOOST_CHECK(client->replace_1("/s/f", suite_s("", "alice:rwxp,carol:r"), false, true) != 0);
        BOOST_REQUIRE_MESSAGE(client->zombieGet() == 0, client->errorMsg());
        BOOST_CHECK(client->server_reply().zombies().empty());
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
        const auto task = client->defs()->findAbsNode("/s/f/t");
        BOOST_REQUIRE(task);
        BOOST_CHECK_EQUAL(NState::toString(task->state()), "active");
        BOOST_CHECK(!task->get_flag().is_set(ecf::Flag::ZOMBIE));
        BOOST_REQUIRE_MESSAGE(client->force("/s/f/t", "queued") == 0, client->errorMsg());
    }

    // A family nobody could change (D4.6)
    BOOST_CHECK(client->replace_1("/s/f", suite_s("", "alice:rwx,bob:r"), false) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("nobody able to change the Permissions attribute of /s/f") !=
                            std::string::npos,
                        client->errorMsg());

    // A valid family: replaced, logged, synchronised
    BOOST_REQUIRE_MESSAGE(client->replace_1("/s/f", suite_s("", "alice:rwxp"), false) == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "alice:rwxp");
    copies_of_alice.check("replace /s/f");
    copies_of_bob.check("replace /s/f");

    // Created with its parents: the attributes of the subtree created are validated under the existing parent /s
    {
        auto defs = suite_s("");
        auto g    = defs->findSuite("s")->add_family("g");
        g->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        g->add_family("h")->add_task("t");
        BOOST_REQUIRE_MESSAGE(client->replace_1("/s/g/h/t", defs, true) == 0, client->errorMsg());
    }
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/g"), "alice:rwxp,bob:r");
    copies_of_alice.check("replace creating /s/g");
    copies_of_bob.check("replace creating /s/g");

    // Replacing the suite without attributes on /s/f: logged as removed
    BOOST_REQUIRE_MESSAGE(client->replace_1("/s", suite_s("alice:rwxp,bob:r"), false) == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "-");
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/g"), "missing");
    copies_of_alice.check("replace /s");
    copies_of_bob.check("replace /s");

    const auto records = permissions_records(server);
    const std::vector<std::string> expected{
        "permissions /s (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s/f (none) -> alice:rwxp by alice@",
        "permissions /s/g (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s/f alice:rwxp -> (none) by alice@",
        "permissions /s/g alice:rwxp,bob:r -> (none) by alice@",
    };
    BOOST_REQUIRE_EQUAL(records.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        BOOST_CHECK_MESSAGE(records[i].rfind(expected[i], 0) == 0,
                            records[i] << " does not start with " << expected[i]);
    }
}

///
/// @brief Builds the definitions of A7.9 and of the other plugs: /pub, open to alice and carol, with family f, which
///        inherits from /pub, and its task t, naming alice only; families g (naming carol too), h (whose family k
///        names carol only), x (whose family y names carol too) and w (whose family v names alice only, as its family
///        u does); /restr, listing alice only.
///
defs_ptr pub_and_restr(const std::string& home = {}) {
    auto defs = Defs::create();
    auto pub  = defs->add_suite("pub");
    if (!home.empty()) {
        // The archives of /pub go to a directory of their own, away from the working directory of the server
        pub->add_variable(ecf::environment::ECF_HOME, home);
    }
    pub->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,carol:rwxp"));
    auto f = pub->add_family("f");
    f->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp"));
    auto g = pub->add_family("g");
    g->add_permissions(Permissions::parse_cli("alice:rwxp,carol:r"));
    g->add_task("t");
    auto h = pub->add_family("h");
    h->add_permissions(Permissions::parse_cli("alice:rwxp,carol:rwxp"));
    h->add_family("k")->add_permissions(Permissions::parse_cli("carol:rwxp"));
    auto x = pub->add_family("x");
    x->add_family("y")->add_permissions(Permissions::parse_cli("alice:rwxp,carol:r"));
    x->add_task("t");
    auto w = pub->add_family("w");
    auto v = w->add_family("v");
    v->add_permissions(Permissions::parse_cli("alice:rwxp"));
    v->add_family("u")->add_permissions(Permissions::parse_cli("alice:rp"));
    auto restr = defs->add_suite("restr");
    restr->add_permissions(Permissions::parse_cli("alice:rwxp"));
    restr->add_task("t");
    return defs;
}

const Credentials carol{"carol", "carol-secret"};

///
/// @brief Returns an absolute directory, outside the working directory of the server, for the archives of /pub.
///
std::string archives_of(const PermissionsServer& server) {
    const auto directory = server.directory() / "pub archives";
    fs::create_directories(directory);
    return directory.string();
}

const std::string plug_server_attribute = R"([{"user": "admin", "permissions": "rwxlop"},)"
                                          R"( {"user": "alice", "permissions": "rwxop"},)"
                                          R"( {"user": "carol", "permissions": "r"}])";

void check_records(const std::vector<std::string>& records, const std::vector<std::string>& expected) {
    BOOST_REQUIRE_EQUAL(records.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        BOOST_CHECK_MESSAGE(records[i].rfind(expected[i], 0) == 0,
                            records[i] << " does not start with " << expected[i]);
    }
}

bool archived(const PermissionsServer& server, const std::string& path) {
    auto check = server.client(alice);
    BOOST_REQUIRE_MESSAGE(check->sync_local() == 0, check->errorMsg());
    const auto node = check->defs()->findAbsNode(path);
    return node && node->get_flag().is_set(ecf::Flag::ARCHIVED);
}

///
/// @brief A7.9 Plug into a restricted suite, and the validation of local plugs: carol, with w and p on /pub/f but
///        not on /restr, is denied because of the destination; a user not listed where the node goes, or a node left
///        without any holder of p, rejects the plug, archived parts included; a valid plug is logged at both paths,
///        archived descendants included, and the copies of the clients follow.
///
BOOST_AUTO_TEST_CASE(test_a7_9_local_plug) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, carol}, plug_server_attribute);
    {
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(pub_and_restr(archives_of(server))) == 0, client->errorMsg());
    }
    Copies copies_of_carol{server, carol, {"pub"}};
    Copies copies_of_alice{server, alice, {"pub", "restr"}};
    copies_of_carol.start();
    copies_of_alice.start();
    BOOST_REQUIRE(copies_of_carol.plain->defs()->findAbsNode("/pub/f"));
    BOOST_REQUIRE(!copies_of_carol.plain->defs()->findAbsNode("/pub/f/t"));

    // carol, with w and p on /pub/f but not on /restr, is denied because of the destination (D4.11)
    auto client_of_carol = server.client(carol);
    BOOST_CHECK(client_of_carol->plug("/pub/f", "/restr") != 0);
    BOOST_CHECK(client_of_carol->server_reply().permission_denied());
    BOOST_CHECK_MESSAGE(client_of_carol->errorMsg().find("required at /restr)") != std::string::npos,
                        client_of_carol->errorMsg());

    auto client_of_alice = server.client(alice);
    // g names carol, who is not listed above /restr/g: rejected, never removed (D2.18)
    BOOST_CHECK(client_of_alice->plug("/pub/g", "/restr") != 0);
    BOOST_CHECK_MESSAGE(client_of_alice->errorMsg().find("/restr/g names the user 'carol'") != std::string::npos,
                        client_of_alice->errorMsg());
    // h/k, where only carol holds p, would be left without any holder of p under /pub/g, where carol holds r only
    // (D4.6); alice, holding p at h and at g, is allowed to plug, but holds nothing at k
    BOOST_CHECK(client_of_alice->plug("/pub/h", "/pub/g/t") != 0); // a task: under its parent g
    BOOST_CHECK_MESSAGE(client_of_alice->errorMsg().find(
                            "nobody able to change the Permissions attribute of /pub/g/h/k") != std::string::npos,
                        client_of_alice->errorMsg());
    // An archived family is validated too: x/y, restored from the archive of x, names carol (D2.18); x stays archived
    BOOST_REQUIRE_MESSAGE(client_of_alice->archive("/pub/x") == 0, client_of_alice->errorMsg());
    BOOST_CHECK(client_of_alice->plug("/pub/x", "/restr") != 0);
    BOOST_CHECK_MESSAGE(client_of_alice->errorMsg().find("/restr/x/y names the user 'carol'") != std::string::npos,
                        client_of_alice->errorMsg());
    BOOST_CHECK(archived(server, "/pub/x"));
    BOOST_CHECK_EQUAL(attribute_at(server, "/pub/g"), "alice:rwxp,carol:r");
    BOOST_CHECK_EQUAL(attribute_at(server, "/pub/h"), "alice:rwxp,carol:rwxp");

    // alice plugs f into /restr: carol had /pub/f, and never receives /restr/f
    BOOST_REQUIRE_MESSAGE(client_of_alice->plug("/pub/f", "/restr") == 0, client_of_alice->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/restr/f/t"), "alice:rwxp");
    copies_of_carol.check("plug of f");
    copies_of_alice.check("plug of f");
    BOOST_CHECK(!copies_of_carol.plain->defs()->findAbsNode("/pub/f"));
    BOOST_CHECK(!copies_of_carol.plain->defs()->findAbsNode("/restr/f"));

    // w, archived with v archived inside it, moves too: both archives are read for the records
    BOOST_REQUIRE_MESSAGE(client_of_alice->archive("/pub/w/v") == 0, client_of_alice->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_alice->archive("/pub/w") == 0, client_of_alice->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_alice->plug("/pub/w", "/restr") == 0, client_of_alice->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/restr/w/v/u"), "alice:rp");
    copies_of_carol.check("plug of w");
    copies_of_alice.check("plug of w");

    check_records(permissions_records(server),
                  {
                      "permissions /pub (none) -> admin:rwxlop,alice:rwxp,carol:rwxp by admin@",
                      "permissions /pub/f/t (none) -> alice:rwxp by admin@",
                      "permissions /pub/g (none) -> alice:rwxp,carol:r by admin@",
                      "permissions /pub/h (none) -> alice:rwxp,carol:rwxp by admin@",
                      "permissions /pub/h/k (none) -> carol:rwxp by admin@",
                      "permissions /pub/x/y (none) -> alice:rwxp,carol:r by admin@",
                      "permissions /pub/w/v (none) -> alice:rwxp by admin@",
                      "permissions /pub/w/v/u (none) -> alice:rp by admin@",
                      "permissions /restr (none) -> alice:rwxp by admin@",
                      "permissions /pub/f/t alice:rwxp -> (none) by alice@",
                      "permissions /restr/f/t (none) -> alice:rwxp by alice@",
                      "permissions /pub/w/v alice:rwxp -> (none) by alice@",
                      "permissions /pub/w/v/u alice:rp -> (none) by alice@",
                      "permissions /restr/w/v (none) -> alice:rwxp by alice@",
                      "permissions /restr/w/v/u (none) -> alice:rp by alice@",
                  });
}

///
/// @brief Verifies the validation of a plug to another server, where the node goes: refused there, the source stays
///        on the sending server; accepted, the attributes are logged on both servers, a whole suite included, and the
///        copies of the clients of both servers follow.
///
BOOST_AUTO_TEST_CASE(test_remote_plug) {
    ECF_NAME_THIS_TEST();

    PermissionsServer sender(admin, {alice, carol}, plug_server_attribute);
    {
        auto client = sender.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(pub_and_restr()) == 0, client->errorMsg());
    }
    // The receiving server lists alice, not carol
    PermissionsServer receiver(admin,
                               {alice, carol},
                               R"([{"user": "admin", "permissions": "rwxlop"},)"
                               R"( {"user": "alice", "permissions": "rwxop"}])");
    {
        auto defs = Defs::create();
        defs->add_suite("s2")->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp"));
        auto client = receiver.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    Copies sender_copies{sender, alice, {"pub", "restr"}};
    Copies receiver_copies{receiver, alice, {"s2"}}; // restr is added to the handle when it arrives
    sender_copies.start();
    receiver_copies.start();
    const auto receiving   = "localhost:" + receiver.port();
    const auto destination = receiving + "/s2";
    auto client_of_alice   = sender.client(alice);

    // g names carol, unknown to the receiving server: refused there, g stays on the sending server
    BOOST_CHECK(client_of_alice->plug("/pub/g", destination) != 0);
    BOOST_CHECK_MESSAGE(client_of_alice->errorMsg().find("names the user 'carol'") != std::string::npos,
                        client_of_alice->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(sender, "/pub/g"), "alice:rwxp,carol:r");

    // f moves, and the whole suite /restr; the attributes are logged where they go, and as removed where they were
    BOOST_REQUIRE_MESSAGE(client_of_alice->plug("/pub/f", destination) == 0, client_of_alice->errorMsg());
    sender_copies.check("remote plug of a family, sending server");
    receiver_copies.check("remote plug of a family, receiving server");
    BOOST_REQUIRE_MESSAGE(client_of_alice->plug("/restr", receiving) == 0, client_of_alice->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(sender, "/pub/f"), "missing");
    BOOST_CHECK_EQUAL(attribute_at(receiver, "/s2/f/t"), "alice:rwxp");
    BOOST_CHECK_EQUAL(attribute_at(receiver, "/restr"), "alice:rwxp");
    sender_copies.check("remote plug of a suite, sending server");
    receiver_copies.check("remote plug of a suite, receiving server");

    auto received = permissions_records(receiver);
    received.erase(received.begin()); // the load of s2
    check_records(received,
                  {
                      "permissions /s2/f/t (none) -> alice:rwxp by alice@",
                      "permissions /restr (none) -> alice:rwxp by alice@",
                  });
    auto sent = permissions_records(sender);
    BOOST_REQUIRE_GE(sent.size(), 2U);
    sent.erase(sent.begin(), sent.end() - 2);
    check_records(sent,
                  {
                      "permissions /pub/f/t alice:rwxp -> (none) by alice@",
                      "permissions /restr alice:rwxp -> (none) by alice@",
                  });
}

///
/// @brief Verifies that deleting nodes logs the attributes removed with them, archived ones included, and deleting
///        every suite logs all of them (R4.5); the copies of the clients follow.
///
BOOST_AUTO_TEST_CASE(test_delete) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, carol}, plug_server_attribute);
    {
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(pub_and_restr(archives_of(server))) == 0, client->errorMsg());
    }
    Copies copies_of_carol{server, carol, {"pub"}};
    copies_of_carol.start();
    const auto loaded = permissions_records(server).size();

    auto client_of_alice = server.client(alice);
    BOOST_REQUIRE_MESSAGE(client_of_alice->delete_node("/pub/g") == 0, client_of_alice->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/pub/g"), "missing");
    copies_of_carol.check("delete of g");
    BOOST_REQUIRE_MESSAGE(client_of_alice->archive("/pub/w/v") == 0, client_of_alice->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_alice->archive("/pub/w") == 0, client_of_alice->errorMsg());
    BOOST_REQUIRE_MESSAGE(client_of_alice->delete_node("/pub/w") == 0, client_of_alice->errorMsg());
    copies_of_carol.check("delete of w, archived");

    auto records = permissions_records(server);
    records.erase(records.begin(), records.begin() + static_cast<std::ptrdiff_t>(loaded));
    check_records(records,
                  {
                      "permissions /pub/g alice:rwxp,carol:r -> (none) by alice@",
                      "permissions /pub/w/v alice:rwxp -> (none) by alice@",
                      "permissions /pub/w/v/u alice:rp -> (none) by alice@",
                  });

    // Deleting every suite logs every attribute removed
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->delete_all(true) == 0, administrator->errorMsg());
    copies_of_carol.check("delete of all suites");
    BOOST_CHECK(copies_of_carol.plain->defs()->suites().empty());
    BOOST_CHECK(copies_of_carol.handle->defs()->suites().empty());
    records = permissions_records(server);
    records.erase(records.begin(), records.begin() + static_cast<std::ptrdiff_t>(loaded) + 3);
    check_records(records,
                  {
                      "permissions /pub admin:rwxlop,alice:rwxp,carol:rwxp -> (none) by admin@",
                      "permissions /pub/f/t alice:rwxp -> (none) by admin@",
                      "permissions /pub/h alice:rwxp,carol:rwxp -> (none) by admin@",
                      "permissions /pub/h/k carol:rwxp -> (none) by admin@",
                      "permissions /pub/x/y alice:rwxp,carol:r -> (none) by admin@",
                      "permissions /restr alice:rwxp -> (none) by admin@",
                  });
}

///
/// @brief Verifies that restoring archived nodes removes the entries of users no longer listed above (D3.5), logs it,
///        and that the copies of the clients equal fresh synchronisations afterwards.
///
BOOST_AUTO_TEST_CASE(test_restore) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    auto client = server.client(alice);
    BOOST_REQUIRE_MESSAGE(client->load(suite_s("alice:rwxp,bob:r", "", "alice:rwxp,bob:r")) == 0, client->errorMsg());
    Copies copies_of_alice{server, alice};
    Copies copies_of_bob{server, bob};
    copies_of_alice.start();
    copies_of_bob.start();

    BOOST_REQUIRE_MESSAGE(client->archive("/s/f") == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->permissions("update", "/s", "alice:rwxp", false) == 0, client->errorMsg());
    copies_of_alice.check("archive and update");
    copies_of_bob.check("archive and update");

    BOOST_REQUIRE_MESSAGE(client->restore("/s/f") == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f/t"), "alice:rwxp");
    copies_of_alice.check("restore");
    copies_of_bob.check("restore");
    const auto records = permissions_records(server);
    BOOST_REQUIRE(!records.empty());
    BOOST_CHECK_MESSAGE(
        records.back().rfind(
            "permissions /s/f/t alice:rwxp,bob:r -> alice:rwxp (removal of users above) by restore of /s/f", 0) == 0,
        records.back());
}

///
/// @brief Verifies that a plug moves the archived nodes as they were validated where they go: their attributes are not
///        cut by the restoration policy of the place they leave (where the users they name may no longer be listed);
///        a plug among siblings, which leaves the nodes where they are, restores them under that policy.
///
BOOST_AUTO_TEST_CASE(test_plug_of_archived_nodes) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    auto client = server.client(alice);
    {
        auto defs = Defs::create();
        auto src  = defs->add_suite("src");
        src->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        src->add_family("m")->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        defs->add_suite("dst")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    // m is archived, then bob is removed from /src: the archive still names bob, listed at /dst
    BOOST_REQUIRE_MESSAGE(client->archive("/src/m") == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->permissions("update", "/src", "alice:rwxp", false) == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->plug("/src/m", "/dst") == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/dst/m/t"), "alice:rwxp,bob:r");
    const auto records = permissions_records(server);
    BOOST_REQUIRE(!records.empty());
    BOOST_CHECK_MESSAGE(records.back().rfind("permissions /dst/m/t (none) -> alice:rwxp,bob:r by alice@", 0) == 0,
                        records.back());

    // Among siblings, the source stays where it is: it is restored as an explicit restore would, and the entries of
    // users no longer listed above are removed (D3.5)
    {
        auto defs = Defs::create();
        auto sib  = defs->add_suite("sib");
        sib->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        sib->add_family("m")->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        sib->add_family("n");
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    BOOST_REQUIRE_MESSAGE(client->archive("/sib/m") == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->permissions("update", "/sib", "alice:rwxp", false) == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->plug("/sib/m", "/sib/n") == 0, client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/sib/m/t"), "alice:rwxp");
    const auto last = permissions_records(server);
    BOOST_REQUIRE(!last.empty());
    BOOST_CHECK_MESSAGE(
        last.back().rfind("permissions /sib/m/t alice:rwxp,bob:r -> alice:rwxp (removal of users above) "
                          "by restore of /sib/m",
                          0) == 0,
        last.back());
}

///
/// @brief Verifies --load --force and --replace over nodes whose attributes are all archived: they require p, as if the
///        attributes were in the tree (R4.5), and the records compare with the archived attributes, nested archives
///        included.
///
BOOST_AUTO_TEST_CASE(test_load_and_replace_over_archived_attributes) {
    ECF_NAME_THIS_TEST();

    const Credentials owner{"owner", "owner-secret"};
    PermissionsServer server(
        admin,
        {alice, owner},
        R"([{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "rwxop"},)"
        R"( {"user": "owner", "permissions": "rwxo"}])");
    auto client = server.client(alice);
    {
        auto defs = Defs::create();
        auto f    = defs->add_suite("s")->add_family("f");
        f->add_family("g")->add_task("t")->add_permissions(Permissions::parse_cli("alice:rp"));
        f->add_task("u")->add_permissions(Permissions::parse_cli("alice:rwxp"));
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    BOOST_REQUIRE_MESSAGE(client->archive("/s/f/g") == 0, client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->archive("/s/f") == 0, client->errorMsg());
    const auto loaded = permissions_records(server).size();

    // owner, with o and w but not p, overwrites neither the suite nor the family
    auto client_of_owner = server.client(owner);
    BOOST_CHECK(client_of_owner->load(suite_s(""), true) != 0);
    BOOST_CHECK_MESSAGE(client_of_owner->errorMsg().find("role p required at /s") != std::string::npos,
                        client_of_owner->errorMsg());
    BOOST_CHECK(client_of_owner->replace_1("/s/f", suite_s(""), false) != 0);
    BOOST_CHECK_MESSAGE(client_of_owner->errorMsg().find("role p required at /s/f") != std::string::npos,
                        client_of_owner->errorMsg());
    {
        auto check = server.client(alice);
        BOOST_REQUIRE_MESSAGE(check->sync_local() == 0, check->errorMsg());
        const auto f = check->defs()->findAbsNode("/s/f");
        BOOST_REQUIRE(f);
        BOOST_CHECK(f->get_flag().is_set(ecf::Flag::ARCHIVED));
    }
    BOOST_CHECK_EQUAL(permissions_records(server).size(), loaded);

    // An archive that cannot be read (invalid, holding /s/f as a task, missing) counts as carrying attributes: p is
    // still required
    {
        const auto archive = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), ":s:f.check");
        BOOST_REQUIRE(fs::exists(archive));
        std::string bytes;
        {
            std::ifstream in(archive);
            bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        }
        for (const std::string kind : {"invalid", "wrong type", "missing"}) {
            BOOST_TEST_CONTEXT(kind << " archive") {
                if (kind == "missing") {
                    fs::remove(archive);
                }
                else if (kind == "wrong type") {
                    std::ofstream(archive) << "suite s\n  task f\nendsuite\n"; // /s/f, but as a task
                }
                else {
                    std::ofstream(archive) << "this is not a definition\n";
                }
                BOOST_CHECK(client_of_owner->load(suite_s(""), true) != 0);
                BOOST_CHECK_MESSAGE(client_of_owner->errorMsg().find("role p required at /s") != std::string::npos,
                                    client_of_owner->errorMsg());
            }
        }
        std::ofstream(archive) << bytes;
    }

    // alice overwrites the suite: the records compare with the archived attributes
    {
        auto defs = Defs::create();
        defs->add_suite("s")->add_family("f")->add_task("u")->add_permissions(Permissions::parse_cli("alice:rp"));
        BOOST_REQUIRE_MESSAGE(client->load(defs, true) == 0, client->errorMsg());
    }
    auto records = permissions_records(server);
    records.erase(records.begin(), records.begin() + static_cast<std::ptrdiff_t>(loaded));
    const std::vector<std::string> expected{
        "permissions /s/f/u alice:rwxp -> alice:rp by alice@",
        "permissions /s/f/g/t alice:rp -> (none) by alice@",
    };
    BOOST_REQUIRE_EQUAL(records.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        BOOST_CHECK_MESSAGE(records[i].rfind(expected[i], 0) == 0,
                            records[i] << " does not start with " << expected[i]);
    }
}

///
/// @brief Verifies that on a server without a server attribute, definitions carrying node attributes are refused by
///        --load and --replace (D2.20), and other definitions are accepted as before; the start-up summary counts no
///        node with an attribute.
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    auto client = server.client(alice);
    BOOST_CHECK(client->load(suite_s("alice:rwxp")) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("requires a server Permissions attribute") != std::string::npos,
                        client->errorMsg());
    BOOST_REQUIRE_MESSAGE(client->load(suite_s("")) == 0, client->errorMsg());
    BOOST_CHECK(client->replace_1("/s/f", suite_s("", "alice:rwxp"), false) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("requires a server Permissions attribute") != std::string::npos,
                        client->errorMsg());
    BOOST_CHECK_EQUAL(attribute_at(server, "/s/f"), "-");

    std::ifstream log(server.home() / "server.log");
    const std::string text{std::istreambuf_iterator<char>(log), std::istreambuf_iterator<char>()};
    BOOST_CHECK_MESSAGE(text.find("Nodes with permissions = 0") != std::string::npos, text);
}

///
/// @brief Verifies that on a server without a server attribute, a local plug of an archived family whose archive
///        carries a node attribute (written while the server had a server attribute) is refused (D2.20, D3.6): under
///        another parent, naming the node where it would go; among its siblings, as an explicit restore. The family
///        stays archived, its archive in place.
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server_plug_of_archived_attributes) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    auto client = server.client(alice);
    {
        auto defs = Defs::create();
        auto s    = defs->add_suite("s");
        s->add_family("f")->add_task("t");
        s->add_family("g");
        defs->add_suite("u");
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }
    BOOST_REQUIRE_MESSAGE(client->archive("/s/f") == 0, client->errorMsg());
    const auto archive = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), ":s:f.check");
    BOOST_REQUIRE(fs::exists(archive));
    {
        // The archive as written by a server holding a server attribute
        Defs stale;
        stale.add_suite("s")->add_family("f")->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp"));
        stale.write_to_checkpt_file(archive.string());
    }

    BOOST_CHECK(client->plug("/s/f", "/u") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("The Permissions attribute of /u/f/t requires a server Permissions "
                                                "attribute") != std::string::npos,
                        client->errorMsg());
    BOOST_CHECK(archived(server, "/s/f"));
    BOOST_CHECK(fs::exists(archive));

    BOOST_CHECK(client->plug("/s/f", "/s/g") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("The restore of /s/f brings Permissions attributes, which require a "
                                                "server Permissions attribute: /s/f/t") != std::string::npos,
                        client->errorMsg());
    BOOST_CHECK(archived(server, "/s/f"));
    BOOST_CHECK(fs::exists(archive));
}

///
/// @brief Verifies that on a server without a server attribute, deleting archived nodes reads no archive: a missing
///        archive logs no warning, for the delete of a node and of every suite (NP7).
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server_delete_of_missing_archives) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    auto client = server.client(alice);
    for (const std::string all : {"", "all"}) {
        BOOST_TEST_CONTEXT("delete " << (all.empty() ? "of /s" : "of every suite")) {
            {
                auto defs = Defs::create();
                defs->add_suite("s")->add_family("f")->add_task("t");
                BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
            }
            BOOST_REQUIRE_MESSAGE(client->archive("/s/f") == 0, client->errorMsg());
            const auto archive = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), ":s:f.check");
            BOOST_REQUIRE(fs::exists(archive));
            fs::remove(archive);
            if (all.empty()) {
                BOOST_REQUIRE_MESSAGE(client->delete_node("/s") == 0, client->errorMsg());
            }
            else {
                auto administrator = server.administrator();
                BOOST_REQUIRE_MESSAGE(administrator->delete_all(true) == 0, administrator->errorMsg());
            }
            BOOST_CHECK_EQUAL(attribute_at(server, "/s"), "missing");
        }
    }

    std::ifstream log(server.home() / "server.log");
    const std::string text{std::istreambuf_iterator<char>(log), std::istreambuf_iterator<char>()};
    BOOST_CHECK_MESSAGE(text.find("could not be read") == std::string::npos, text);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

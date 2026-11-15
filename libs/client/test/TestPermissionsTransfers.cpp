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
///        on suite s.
///
struct Copies
{
    const PermissionsServer& server;
    Credentials user;
    std::unique_ptr<ClientInvoker> plain  = server.client(user);
    std::unique_ptr<ClientInvoker> handle = server.client(user);

    void start() {
        BOOST_REQUIRE_MESSAGE(plain->sync_local() == 0, plain->errorMsg());
        BOOST_REQUIRE_MESSAGE(handle->ch_register(true, {"s"}) == 0, handle->errorMsg());
        BOOST_REQUIRE_MESSAGE(handle->sync_local() == 0, handle->errorMsg());
    }

    void check(const std::string& step) {
        auto fresh = server.client(user);
        BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
        auto fresh_handle = server.client(user);
        BOOST_REQUIRE_MESSAGE(fresh_handle->ch_register(true, {"s"}) == 0, fresh_handle->errorMsg());
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

/// @brief Verifies that on a server without a server attribute, definitions carrying node attributes are refused by
///        --load and --replace (D2.20), and other definitions are accepted as before.
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
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

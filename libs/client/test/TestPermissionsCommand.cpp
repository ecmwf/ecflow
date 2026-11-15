// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ReadableSet.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/cts/user/PermissionsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The command --permissions (D4.3, D4.4, D5.23, D5.24, D10.2): each operation and its errors, the roles it
/// requires, the log records of changes, and the synchronisation of clients after each change (A7.2).
///

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};
const Credentials bob{"bob", "bob-secret"};

// admin holds no sticky entry, so that the self-lockout check applies to the others
const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlop"},)"
    R"( {"user": "alice", "permissions": "rwxp"}, {"user": "bob", "permissions": "r"}])";

///
/// @brief Loads suite s, with family f and task t, and suite u, both without attribute.
///
void load(const PermissionsServer& server) {
    auto defs = Defs::create();
    defs->add_suite("s")->add_family("f")->add_task("t");
    defs->add_suite("u")->add_task("t");
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

///
/// @brief Runs --permissions as a user, and returns its output, or "error: <message>" when it fails.
///
std::string run(const PermissionsServer& server,
                const Credentials& user,
                const std::string& operation,
                const std::string& path,
                const std::string& value = {},
                bool json                = false) {
    auto client = server.client(user);
    if (client->permissions(operation, path, value, json) != 0) {
        return "error: " + client->errorMsg();
    }
    return client->get_string();
}

bool failed(const std::string& output, const std::string& reason) {
    return output.rfind("error: ", 0) == 0 && output.find(reason) != std::string::npos;
}

std::vector<std::string> permissions_records(const PermissionsServer& server) {
    std::ifstream log(server.home() / "server.log");
    std::vector<std::string> records;
    for (std::string line; std::getline(log, line);) {
        if (line.find(" permissions /") != std::string::npos && line.rfind("LOG:", 0) == 0) {
            records.push_back(line.substr(line.find(" permissions /") + 1));
        }
    }
    return records;
}

///
/// @brief Returns the definitions of a copy as text (structure and attributes), for comparison with a fresh one.
///
std::string text_of(const Defs& defs) {
    std::string text;
    defs.write_to_string(text, PrintStyle::DEFS);
    if (const auto* server = defs.server_state().permissions(); server) {
        text += "server " + server->to_cli();
    }
    return text;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsCommand)

///
/// @brief Verifies each operation of --permissions, and its errors.
///
BOOST_AUTO_TEST_CASE(test_operations) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load(server);

    // read
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/"), "admin:rwxlop,alice:rwxp,bob:r");
    BOOST_CHECK(failed(run(server, alice, "read", "/s"), "/s has no Permissions attribute"));
    BOOST_CHECK(failed(run(server, alice, "read", "/missing"), "/missing"));

    // create
    BOOST_CHECK_EQUAL(run(server, alice, "create", "/s", "alice:rwxp,bob:r"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s"), "alice:rwxp,bob:r");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s", "", true),
                      R"([{"permissions":"rwxp","user":"alice"},{"permissions":"r","user":"bob"}])");
    BOOST_CHECK(failed(run(server, alice, "create", "/s", "alice:rwxp"), "/s already has a Permissions attribute"));
    BOOST_CHECK_EQUAL(run(server, alice, "create", "/s/f", R"([{"user": "alice", "permissions": "rwxp"}])"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s/f"), "alice:rwxp");
    BOOST_CHECK(failed(run(server, alice, "create", "/s/f/t", "alice:rp,carol:r"), "names the user 'carol'"));
    BOOST_CHECK(failed(run(server, alice, "create", "/s/f/t", "alice:"), "alice"));
    BOOST_CHECK(failed(run(server, alice, "read", "/s/f/t"), "/s/f/t has no Permissions attribute"));

    // update
    BOOST_CHECK(failed(run(server, alice, "update", "/u", "alice:rwxp"), "/u has no Permissions attribute"));
    BOOST_CHECK_EQUAL(run(server, alice, "update", "/s", "alice:rwxp,bob:rw"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s"), "alice:rwxp,bob:rw");
    BOOST_CHECK(failed(run(server, alice, "update", "/s", "alice:rwx,bob:rw"), "nobody able to change"));
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s"), "alice:rwxp,bob:rw");

    // delete
    BOOST_CHECK(failed(run(server, alice, "delete", "/u"), "/u has no Permissions attribute"));
    BOOST_CHECK_EQUAL(run(server, alice, "delete", "/s/f"), "");
    BOOST_CHECK(failed(run(server, alice, "read", "/s/f"), "/s/f has no Permissions attribute"));

    // At the server: update only (D10.2)
    BOOST_CHECK(failed(run(server, admin, "create", "/", "admin:rwxlop"), "/ already has a Permissions attribute"));
    BOOST_CHECK(failed(run(server, admin, "delete", "/"), "cannot be deleted at run time"));
    BOOST_CHECK_EQUAL(run(server, admin, "update", "/", "admin:rwxlop,alice:rwxp,bob:r,carol:r"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/"), "admin:rwxlop,alice:rwxp,bob:r,carol:r");

    // effective
    BOOST_CHECK_EQUAL(run(server, alice, "effective", "/s"), "rwxp");
    BOOST_CHECK_EQUAL(run(server, alice, "effective", "/s", "bob"), "rw");
    BOOST_CHECK_EQUAL(run(server, alice, "effective", "/u", "carol"), "r");
    BOOST_CHECK_EQUAL(run(server, alice, "effective", "/s", "carol"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "effective", "/s", "bob", true),
                      R"({"path":"/s","permissions":"rw","user":"bob"})");

    // Client-side errors: unknown operation, missing value, relative path, unknown format
    auto client = server.client(alice);
    BOOST_CHECK(client->invoke(std::vector<std::string>{"--permissions=grant", "/s", "bob:r"}) != 0);
    BOOST_CHECK(client->invoke(std::vector<std::string>{"--permissions=update", "/s"}) != 0);
    BOOST_CHECK(client->invoke(std::vector<std::string>{"--permissions=read", "s"}) != 0);
    BOOST_CHECK(client->invoke(std::vector<std::string>{"--permissions=read", "/s", "--fmt=xml"}) != 0);
    for (const std::string name : {"*", "al ice", "a:b"}) {
        BOOST_TEST_CONTEXT("user '" << name << "'") {
            BOOST_CHECK(client->invoke(std::vector<std::string>{"--permissions=effective", "/s", name}) != 0);
            BOOST_CHECK_MESSAGE(client->errorMsg().find("invalid user name") != std::string::npos, client->errorMsg());
        }
    }

    // The same checks apply to requests built directly; the server repeats them
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/u", "alice:rwxp"), "");
    BOOST_CHECK(failed(run(server, alice, "delete", "/u", "unexpected"), "delete takes no value"));
    BOOST_CHECK(failed(run(server, alice, "read", "u"), "the path must be absolute"));
    BOOST_CHECK(failed(run(server, alice, "grant", "/u", "bob:r"), "unknown operation 'grant'"));
    BOOST_CHECK(failed(run(server, alice, "effective", "/u", "a:b"), "invalid user name"));
    BOOST_CHECK(failed(run(server, alice, "effective", "/u", "*"), "invalid user name"));
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/u"), "alice:rwxp");
    {
        PermissionsCmd command("delete", "/u", "unexpected");
        BOOST_CHECK_THROW(PermissionsCmd::check_arguments(command.operation(), command.path(), command.value()),
                          std::runtime_error);
    }

    // A value starting with "[" that is not JSON is read in the command-line form
    BOOST_CHECK_EQUAL(run(server, admin, "update", "/", "[x:r,admin:rwxlop,alice:rwxp,bob:r,carol:r"), "");
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/"), "[x:r,admin:rwxlop,alice:rwxp,bob:r,carol:r");
    BOOST_CHECK(failed(run(server, admin, "update", "/", "[x:"), "as the JSON form"));

    // A request refused by the client leaves no state of the earlier request of the same client
    {
        auto reused = server.client(bob);
        BOOST_REQUIRE_EQUAL(reused->permissions("read", "/s", "", false), 0);
        BOOST_CHECK_EQUAL(reused->get_string(), "alice:rwxp,bob:rw");
        BOOST_CHECK(reused->permissions("delete", "/s", "", false) != 0);
        BOOST_CHECK(reused->server_reply().permission_denied());
        BOOST_CHECK(reused->permissions("read", "s", "", false) != 0);
        BOOST_CHECK_MESSAGE(reused->errorMsg().find("the path must be absolute") != std::string::npos,
                            reused->errorMsg());
        BOOST_CHECK(!reused->server_reply().permission_denied());
        BOOST_CHECK(reused->connection_diagnosis().failure != ecf::ConnectionFailure::PermissionDenied);
        BOOST_CHECK(reused->get_string().empty());
        BOOST_CHECK_EQUAL(run(server, alice, "read", "/s"), "alice:rwxp,bob:rw");
    }
    BOOST_CHECK_EQUAL(run(server, alice, "update", "/u", "\n [{\"user\": \"alice\", \"permissions\": \"rwxp\"}]"), "");
}

///
/// @brief Verifies the roles required: r for read and effective, p for create, update and delete.
///
BOOST_AUTO_TEST_CASE(test_roles_required) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load(server);
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/s", "alice:rwxp,bob:rwx"), "");

    BOOST_CHECK_EQUAL(run(server, bob, "read", "/s"), "alice:rwxp,bob:rwx");
    BOOST_CHECK_EQUAL(run(server, bob, "effective", "/s"), "rwx");
    for (const auto& [operation, value] : std::vector<std::pair<std::string, std::string>>{
             {"update", "alice:rwxp,bob:rwxp"}, {"delete", ""}, {"create", "bob:r"}}) {
        auto client       = server.client(bob);
        const auto path   = operation == "create" ? std::string("/s/f") : std::string("/s");
        const auto status = client->permissions(operation, path, value, false);
        BOOST_CHECK_MESSAGE(status != 0 && client->server_reply().permission_denied(), operation);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("role p required at " + path) != std::string::npos,
                            client->errorMsg());
    }
    BOOST_CHECK(failed(run(server, bob, "update", "/", "admin:rwxlop"), "role p required at /"));
    BOOST_CHECK_EQUAL(run(server, alice, "read", "/s"), "alice:rwxp,bob:rwx");

    // A node bob cannot read: neither its attribute nor the effective permissions there
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/u", "alice:rwxp"), "");
    BOOST_CHECK(failed(run(server, bob, "read", "/u"), "role r required at /u"));
    BOOST_CHECK(failed(run(server, bob, "effective", "/u", "alice"), "role r required at /u"));
}

///
/// @brief Verifies the log records of changes, with requester, path, old and new values, and the cascade (R4.4).
///
BOOST_AUTO_TEST_CASE(test_log_records) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load(server);
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/s", "alice:rwxp,bob:r"), "");
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/s/f", "alice:rwxp,bob:r"), "");
    BOOST_REQUIRE_EQUAL(run(server, alice, "update", "/s", "alice:rwxp"), "");
    BOOST_REQUIRE_EQUAL(run(server, alice, "delete", "/s/f"), "");
    BOOST_REQUIRE(failed(run(server, alice, "update", "/s", "alice:rwxp,dave:r"), "dave"));

    const auto records = permissions_records(server);
    const std::vector<std::string> expected{
        "permissions /s (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s/f (none) -> alice:rwxp,bob:r by alice@",
        "permissions /s alice:rwxp,bob:r -> alice:rwxp by alice@",
        "permissions /s/f alice:rwxp,bob:r -> alice:rwxp (removal of users above) by alice@",
        "permissions /s/f alice:rwxp -> (none) by alice@",
    };
    BOOST_REQUIRE_EQUAL(records.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        BOOST_CHECK_MESSAGE(records[i].rfind(expected[i], 0) == 0,
                            records[i] << " does not start with " << expected[i]);
    }
}

///
/// @brief Verifies that after create, update, delete and cascades, every client copy, two of them sharing a handle,
///        equals a fresh full synchronisation; and the handover of A7.2.
///
BOOST_AUTO_TEST_CASE(test_a7_2_handover_and_synchronisation) {
    ECF_NAME_THIS_TEST();

    const Credentials a{"teamA", "a-secret"};
    const Credentials b{"teamB", "b-secret"};
    PermissionsServer server(admin,
                             {a, b},
                             R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "teamA", "permissions": "r"},)"
                             R"( {"user": "teamB", "permissions": "r"}])");
    {
        auto defs = Defs::create();
        defs->add_suite("sX")->add_family("f")->add_task("t");
        defs->add_suite("other")->add_task("t");
        defs->findSuite("sX")->add_permissions(ecf::Permissions::parse_cli("teamA:rwxp,teamB:r"));
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    }

    // The clients of a: without handle, and two copies sharing a handle on sX
    auto plain = server.client(a);
    auto first = server.client(a);
    BOOST_REQUIRE_MESSAGE(first->ch_register(false, {"sX"}) == 0, first->errorMsg());
    auto second = server.client(a);
    const_cast<ServerReply&>(second->server_reply()).set_client_handle(first->client_handle());
    std::vector<ClientInvoker*> clients{plain.get(), first.get(), second.get()};
    for (auto* client : clients) {
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
    }

    const auto check_copies = [&](const std::string& step) {
        auto fresh = server.client(a);
        BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
        auto fresh_handle = server.client(a);
        BOOST_REQUIRE_MESSAGE(fresh_handle->ch_register(false, {"sX"}) == 0, fresh_handle->errorMsg());
        BOOST_REQUIRE_MESSAGE(fresh_handle->sync_local() == 0, fresh_handle->errorMsg());
        for (auto* client : clients) {
            BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
            const auto& reference = client == plain.get() ? fresh : fresh_handle;
            BOOST_CHECK_MESSAGE(text_of(*client->defs()) == text_of(*reference->defs()),
                                step << ": the copy differs from a fresh synchronisation\n"
                                     << text_of(*client->defs()) << "\n---\n"
                                     << text_of(*reference->defs()));
        }
    };

    BOOST_REQUIRE_EQUAL(run(server, a, "create", "/sX/f", "teamA:rwxp,teamB:r"), "");
    check_copies("create");
    BOOST_REQUIRE_EQUAL(run(server, a, "update", "/sX/f", "teamA:rwxp"), "");
    check_copies("update");
    BOOST_REQUIRE_EQUAL(run(server, a, "update", "/sX/f", "teamA:rwxp,teamB:r"), "");
    BOOST_REQUIRE_EQUAL(run(server, a, "update", "/sX", "teamA:rwxp"), ""); // cascade: b removed from /sX/f
    BOOST_CHECK_EQUAL(run(server, a, "read", "/sX/f"), "teamA:rwxp");
    check_copies("cascade");
    BOOST_REQUIRE_EQUAL(run(server, a, "delete", "/sX/f"), "");
    check_copies("delete");
    BOOST_REQUIRE_EQUAL(run(server, admin, "update", "/", "admin:rwxlops,teamA:r,teamB:r,c:r"), "");
    check_copies("server update");

    // A7.2: a hands /sX over to b, who then holds rwx and p, while a holds nothing there
    BOOST_REQUIRE_EQUAL(run(server, a, "update", "/sX", "teamB:rwxp"), "");
    const auto check_lost = [&](const std::string& step) {
        // a cannot register sX any more: the copies through the handle equal a fresh handle holding nothing
        auto fresh = server.client(a);
        BOOST_REQUIRE_MESSAGE(fresh->sync_local() == 0, fresh->errorMsg());
        auto fresh_handle = server.client(a);
        BOOST_REQUIRE_MESSAGE(fresh_handle->ch_register(false, {}) == 0, fresh_handle->errorMsg());
        BOOST_REQUIRE_MESSAGE(fresh_handle->sync_local() == 0, fresh_handle->errorMsg());
        for (auto* client : clients) {
            BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
            BOOST_CHECK_MESSAGE(!client->defs()->findAbsNode("/sX"), step);
            const auto& reference = client == plain.get() ? fresh : fresh_handle;
            BOOST_CHECK_MESSAGE(text_of(*client->defs()) == text_of(*reference->defs()),
                                step << ": the copy differs from a fresh synchronisation\n"
                                     << text_of(*client->defs()) << "\n---\n"
                                     << text_of(*reference->defs()));
        }
    };
    check_lost("handover");
    auto client_of_b = server.client(b);
    BOOST_REQUIRE_MESSAGE(client_of_b->sync_local() == 0, client_of_b->errorMsg());
    BOOST_CHECK(client_of_b->defs()->findAbsNode("/sX/f/t"));
    const auto records = permissions_records(server);
    BOOST_REQUIRE(!records.empty());
    BOOST_CHECK_MESSAGE(records.back().rfind("permissions /sX teamA:rwxp -> teamB:rwxp by teamA@", 0) == 0,
                        records.back());

    // b gives a its access back: the handle of a, which still holds sX, sends it again to both copies
    BOOST_REQUIRE_EQUAL(run(server, b, "update", "/sX", "teamA:rwxp,teamB:rwxp"), "");
    check_copies("access restored");
    for (auto* client : clients) {
        BOOST_CHECK(client->defs()->findAbsNode("/sX/f/t"));
    }
}

///
/// @brief Verifies that changes are write requests: checkpointed at once with the checkpoint mode "always", and
///        followed by the automatic synchronisation of a client that enables it; read and effective are read-only.
///
BOOST_AUTO_TEST_CASE(test_changes_are_writes) {
    ECF_NAME_THIS_TEST();

    for (const auto& operation : {"create", "update", "delete"}) {
        BOOST_CHECK_MESSAGE(PermissionsCmd(operation, "/s", "alice:rwxp").isWrite(), operation);
    }
    for (const auto& operation : {"read", "effective"}) {
        BOOST_CHECK_MESSAGE(!PermissionsCmd(operation, "/s").isWrite(), operation);
    }

    PermissionsServer server(admin, {alice, bob}, server_attribute);
    load(server);
    auto administrator = server.administrator();
    BOOST_REQUIRE_MESSAGE(administrator->checkPtDefs(ecf::CheckPt::ALWAYS) == 0, administrator->errorMsg());

    auto client = server.client(alice);
    client->set_auto_sync(true);
    BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
    const auto checkpoint   = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), Ecf::CHECKPT());
    const auto checkpointed = [&]() {
        std::ifstream file(checkpoint);
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };

    const auto& values = std::vector<std::pair<std::string, std::string>>{
        {"create", "alice:rwxp,bob:r"}, {"update", "alice:rwxp,bob:rw"}, {"delete", ""}};
    for (const auto& [operation, value] : values) {
        BOOST_TEST_CONTEXT(operation) {
            BOOST_REQUIRE_MESSAGE(client->permissions(operation, "/s", value, false) == 0, client->errorMsg());
            // The copy of the client follows, without an explicit synchronisation
            const auto* copy = client->defs()->findAbsNode("/s")->permissions();
            BOOST_CHECK_EQUAL(copy ? copy->to_cli() : std::string(), value);
            // The checkpoint holds the change
            const auto text = checkpointed();
            if (value.empty()) {
                const auto removed = ecf::Permissions::parse_cli("alice:rwxp,bob:rw").to_json();
                BOOST_CHECK_MESSAGE(text.find("permissions " + removed) == std::string::npos, text);
            }
            else {
                BOOST_CHECK_MESSAGE(text.find("permissions " + ecf::Permissions::parse_cli(value).to_json()) !=
                                        std::string::npos,
                                    text);
            }
        }
    }
}

///
/// @brief Verifies that the command works over HTTP as over TCP: a change, its effect, and a denial.
///
BOOST_AUTO_TEST_CASE(test_over_http) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin,
                             {alice, bob},
                             server_attribute,
                             std::chrono::seconds(30),
                             std::nullopt,
                             std::nullopt,
                             false,
                             R"("http": true)");
    load(server);
    BOOST_CHECK_EQUAL(run(server, alice, "create", "/s", "alice:rwxp,bob:r"), "");
    BOOST_CHECK_EQUAL(run(server, bob, "read", "/s", "", true),
                      R"([{"permissions":"rwxp","user":"alice"},{"permissions":"r","user":"bob"}])");
    BOOST_CHECK_EQUAL(run(server, bob, "effective", "/s"), "r");
    BOOST_CHECK(failed(run(server, bob, "delete", "/s"), "role p required at /s"));
    BOOST_CHECK_EQUAL(run(server, alice, "delete", "/s"), "");
    BOOST_CHECK(failed(run(server, bob, "read", "/s"), "/s has no Permissions attribute"));
}

///
/// @brief Verifies --permissions on a server without a server attribute: node attributes cannot be created
///        (D2.20), the server attribute cannot be created (D10.2), and effective reports that Permissions do not
///        apply.
///
BOOST_AUTO_TEST_CASE(test_unrestricted_server) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    load(server);
    BOOST_CHECK(failed(run(server, alice, "create", "/s", "alice:rwxp"), "requires a server Permissions attribute"));
    BOOST_CHECK(failed(run(server, alice, "create", "/", "alice:rwxp"), "cannot be created at run time"));
    BOOST_CHECK(failed(run(server, alice, "update", "/", "alice:rwxp"), "/ has no Permissions attribute"));
    BOOST_CHECK(failed(run(server, alice, "read", "/"), "/ has no Permissions attribute"));
    BOOST_CHECK(failed(run(server, alice, "effective", "/s"), "does not use Permissions attributes"));
}

///
/// @brief Verifies the output of the command-line client, in both formats.
///
BOOST_AUTO_TEST_CASE(test_command_line_output) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, server_attribute);
    load(server);
    BOOST_REQUIRE_EQUAL(run(server, alice, "create", "/s", "alice:rwxp"), "");

    using namespace ecf::test::scaffold;
    const auto passwords = server.directory() / "alice.custom_passwd";
    std::ofstream(passwords) << "4.5.0\nalice localhost " << server.port() << " alice-secret\n";
    WithTestEnvironmentVariable host("ECF_HOST", "localhost");
    WithTestEnvironmentVariable port("ECF_PORT", server.port());
    WithTestEnvironmentVariable user("ECF_USER", "alice");
    WithTestEnvironmentVariable password_file("ECF_CUSTOM_PASSWD", passwords.string());
    const auto client = fs::path(ecf::File::find_ecf_client_path());
    {
        Process read(client, {"--permissions", "read", "/s"}, server.directory());
        BOOST_CHECK_EQUAL(read.wait(), 0);
        BOOST_CHECK_EQUAL(read.read_stdout(), "alice:rwxp\n");
    }
    {
        Process read(client, {"--permissions", "read", "/s", "--fmt", "json"}, server.directory());
        BOOST_CHECK_EQUAL(read.wait(), 0);
        BOOST_CHECK_EQUAL(read.read_stdout(), "[{\"permissions\":\"rwxp\",\"user\":\"alice\"}]\n");
    }
    {
        Process update(client, {"--permissions", "update", "/s", "alice:rwxp,bob:r"}, server.directory());
        BOOST_CHECK_EQUAL(update.wait(), 0);
    }
    {
        Process effective(client, {"--permissions=effective", "/s", "bob"}, server.directory());
        BOOST_CHECK_EQUAL(effective.wait(), 0);
        BOOST_CHECK_EQUAL(effective.read_stdout(), "r\n");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

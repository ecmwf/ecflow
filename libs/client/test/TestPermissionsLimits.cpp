// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};

/// The server attribute of every test: alice may read, write and change attributes at server level
const std::string server_attribute =
    R"([{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "rwxop"},
        {"user": "bob", "permissions": "r"}, {"user": "carol", "permissions": "r"}])";

///
/// @brief Starts a server with the given limits, and the server attribute above.
///
/// @param[in] limits The members of `/ecflow/server/limits`, as JSON text, e.g. `"handles_per_user": 2`
/// @param[in] http True for a server, and clients, using HTTP
/// @return The server, stopped on destruction.
///
std::unique_ptr<PermissionsServer> start(const std::string& limits, bool http = false) {
    return std::make_unique<PermissionsServer>(admin,
                                               std::vector<Credentials>{alice},
                                               server_attribute,
                                               std::chrono::seconds(30),
                                               std::nullopt,
                                               std::nullopt,
                                               false,
                                               R"("limits": {)" + limits + "}" + (http ? R"(, "http": true)" : ""));
}

///
/// @brief Returns definitions holding one suite, with the given number of tasks.
///
/// @param[in] name The name of the suite
/// @param[in] tasks The number of tasks, each named after its index
/// @param[in] permissions The Permissions attribute of the suite, in the command-line form; none when empty
/// @return The definitions.
///
defs_ptr suite(const std::string& name, int tasks, const std::string& permissions = {}) {
    auto defs = Defs::create();
    auto s    = defs->add_suite(name);
    for (int i = 0; i < tasks; ++i) {
        s->add_task("task_with_a_long_name_" + std::to_string(i));
    }
    if (!permissions.empty()) {
        s->add_permissions(ecf::Permissions::parse_cli(permissions));
    }
    return defs;
}

///
/// @brief Returns the names of the suites of the server, as the administrator reads them.
///
/// @param[in] server The server
/// @return The names.
///
std::vector<std::string> suites_of(const PermissionsServer& server) {
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
    std::vector<std::string> names;
    for (const auto& s : client->defs()->suites()) {
        names.push_back(s->name());
    }
    return names;
}

///
/// @brief Sends raw bytes to the server, and returns what it answers until it closes the connection.
///
/// The exchange is bounded: when the server neither answers nor closes the connection within 30 seconds, the test
/// fails, and the connection is closed.
///
/// @param[in] port The port of the server, on localhost
/// @param[in] bytes The bytes sent
/// @param[in] wait_for_reply False to close the connection at once, without reading
/// @return The bytes received.
///
std::string send_raw(const std::string& port, const std::string& bytes, bool wait_for_reply = true) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::connect(socket, resolver.resolve("localhost", port));
    boost::asio::write(socket, boost::asio::buffer(bytes));
    std::string received;
    if (!wait_for_reply) {
        return received;
    }

    std::array<char, 4096> buffer{};
    bool closed = false;
    std::function<void()> read;
    read = [&]() {
        socket.async_read_some(boost::asio::buffer(buffer), [&](const boost::system::error_code& ec, std::size_t n) {
            received.append(buffer.data(), n);
            if (ec) {
                closed = true;
                return;
            }
            read();
        });
    };
    read();
    io.run_for(std::chrono::seconds(30));
    if (!closed) {
        boost::system::error_code ignored;
        socket.close(ignored);
        BOOST_FAIL("the server neither answered nor closed the connection within 30 seconds");
    }
    return received;
}

///
/// @brief Returns a request of --ping by the administrator, serialised, padded with spaces to the given size.
///
/// @param[in] size The size of the request, in bytes; at least that of the serialised request
/// @return The request.
///
std::string ping_request(std::size_t size) {
    auto cmd = std::make_shared<CtsCmd>(CtsCmd::PING);
    cmd->setup_user_authentification(admin.user, PasswordEncryption::encrypt(admin.password, admin.user));
    ClientToServerRequest request;
    request.set_cmd(cmd);
    std::string text;
    ecf::save_as_string(text, request);
    BOOST_REQUIRE_LE(text.size(), size);
    return text + std::string(size - text.size(), ' ');
}

///
/// @brief Returns a header of the TCP/IP protocol announcing the given size: 8 characters, the size in hexadecimal.
///
/// @param[in] size The size announced, in bytes
/// @return The header.
///
std::string header(std::size_t size) {
    std::ostringstream os;
    os << std::setw(8) << std::hex << size;
    return os.str();
}

///
/// @brief Returns the number of lines of the server log containing the given text.
///
/// @param[in] server The server
/// @param[in] text The text searched
/// @return The number of lines.
///
std::size_t log_count(const PermissionsServer& server, const std::string& text) {
    std::ifstream in(server.home() / "server.log");
    std::size_t count = 0;
    for (std::string line; std::getline(in, line);) {
        count += line.find(text) != std::string::npos ? 1 : 0;
    }
    return count;
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsLimits)

///
/// @brief Verifies the limit on the number of client handles of each user (R11.10).
///
BOOST_AUTO_TEST_CASE(test_handles_per_user) {
    ECF_NAME_THIS_TEST();

    auto server = start(R"("handles_per_user": 2)");
    BOOST_REQUIRE_EQUAL(server->administrator()->load(suite("a", 1)), 0);

    auto first  = server->client(alice);
    auto second = server->client(alice);
    auto third  = server->client(alice);
    BOOST_REQUIRE_MESSAGE(first->ch_register(false, {"a"}) == 0, first->errorMsg());
    BOOST_REQUIRE_MESSAGE(second->ch_register(false, {"a"}) == 0, second->errorMsg());
    BOOST_CHECK(third->ch_register(false, {"a"}) != 0);
    BOOST_CHECK_MESSAGE(third->errorMsg().find("limit of 2 client handles per user") != std::string::npos,
                        third->errorMsg());

    // Replacing a handle of the user is not a new one
    BOOST_CHECK_MESSAGE(first->ch1_register(false, {"a"}) == 0, first->errorMsg());

    // Each user has its own count; dropping a handle frees its place
    auto other = server->administrator();
    BOOST_CHECK_MESSAGE(other->ch_register(false, {"a"}) == 0, other->errorMsg());
    BOOST_REQUIRE_EQUAL(second->ch_drop(second->client_handle()), 0);
    BOOST_CHECK_MESSAGE(third->ch_register(false, {"a"}) == 0, third->errorMsg());
}

///
/// @brief Verifies the limit on the number of entries of a Permissions attribute, wherever attributes arrive.
///
BOOST_AUTO_TEST_CASE(test_permissions_entries) {
    ECF_NAME_THIS_TEST();

    auto server = start(R"("permissions_entries": 3)");
    auto client = server->client(alice);

    // Load: refused beyond the limit, with nothing loaded
    BOOST_CHECK(client->load(suite("big", 1, "admin:rwxlop,alice:rwxp,bob:r,carol:r")) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("more than 3 entries") != std::string::npos, client->errorMsg());
    BOOST_CHECK(suites_of(*server).empty());
    BOOST_REQUIRE_MESSAGE(client->load(suite("a", 1, "admin:rwxlop,alice:rwxp,bob:r")) == 0, client->errorMsg());

    // --permissions: refused beyond the limit, the attribute unchanged
    BOOST_CHECK(client->permissions("update", "/a", "admin:rwxlop,alice:rwxp,bob:r,carol:r", false) != 0);
    BOOST_REQUIRE_EQUAL(client->permissions("read", "/a", "", false), 0);
    BOOST_CHECK_EQUAL(client->get_string(), "admin:rwxlop,alice:rwxp,bob:r");

    // --permissions create: refused beyond the limit, nothing created; c inherits the server attribute, which lists the
    // four users, so that only the size refuses the value
    BOOST_REQUIRE_MESSAGE(client->load(suite("c", 1)) == 0, client->errorMsg());
    BOOST_CHECK(client->permissions(
                    "create", "/c/task_with_a_long_name_0", "admin:rwxlop,alice:rwxp,bob:r,carol:r", false) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("more than 3 entries") != std::string::npos, client->errorMsg());
    BOOST_CHECK(client->permissions("read", "/c/task_with_a_long_name_0", "", false) != 0);
    BOOST_CHECK_MESSAGE(
        client->permissions("create", "/c/task_with_a_long_name_0", "admin:rwxlop,alice:rwxp,bob:r", false) == 0,
        client->errorMsg());

    // Replace: refused beyond the limit, the node unchanged
    auto replacement = suite("a", 2, "admin:rwxlop,alice:rwxp,bob:r,carol:r");
    BOOST_CHECK(client->replace_1("/a", replacement) != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("more than 3 entries") != std::string::npos, client->errorMsg());
    auto administrator = server->administrator();
    BOOST_REQUIRE_EQUAL(administrator->getDefs(), 0);
    BOOST_CHECK(!administrator->defs()->findAbsNode("/a/task_with_a_long_name_1"));
}

///
/// @brief Verifies the limit on the size of a request, over TCP/IP and over HTTP: the request is refused, unread, and
///        the server keeps serving the other requests.
///
BOOST_AUTO_TEST_CASE(test_request_size) {
    ECF_NAME_THIS_TEST();

    for (bool http : {false, true}) {
        BOOST_TEST_CONTEXT("http: " << http) {
            auto server = start(R"("request_size": 4096)", http);
            auto client = server->administrator();
            BOOST_CHECK(client->load(suite("big", 200)) != 0);
            BOOST_CHECK_EQUAL(client->pingServer(), 0);
            BOOST_CHECK(suites_of(*server).empty());
            BOOST_CHECK_MESSAGE(client->load(suite("small", 1)) == 0, client->errorMsg());
            BOOST_CHECK(suites_of(*server) == std::vector<std::string>{"small"});
        }
    }
}

///
/// @brief Verifies that the limit on the entries of an attribute applies to every node of the subtree brought in:
///        families, tasks and aliases, also when the replacement creates the missing parents.
///
BOOST_AUTO_TEST_CASE(test_permissions_entries_below_the_root) {
    ECF_NAME_THIS_TEST();

    auto server           = start(R"("permissions_entries": 3)");
    auto client           = server->client(alice);
    const std::string big = "admin:rwxlop,alice:rwxp,bob:r,carol:r";

    { // A family
        auto defs = suite("f", 1);
        defs->findAbsNode("/f")->isSuite()->add_family("g")->add_permissions(ecf::Permissions::parse_cli(big));
        BOOST_CHECK(client->load(defs) != 0);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("/f/g has more than 3 entries") != std::string::npos,
                            client->errorMsg());
    }
    { // An alias
        auto defs = suite("al", 1);
        auto task = defs->findAbsNode("/al/task_with_a_long_name_0")->isTask();
        task->add_alias_only()->add_permissions(ecf::Permissions::parse_cli(big));
        BOOST_CHECK(client->load(defs) != 0);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("more than 3 entries") != std::string::npos, client->errorMsg());
    }
    BOOST_CHECK(suites_of(*server).empty());

    { // A replacement creating its parents
        BOOST_REQUIRE_MESSAGE(client->load(suite("r", 1)) == 0, client->errorMsg());
        auto defs = suite("r", 1);
        auto x    = defs->findAbsNode("/r")->isSuite()->add_family("x");
        x->add_task("y")->add_permissions(ecf::Permissions::parse_cli(big));
        BOOST_CHECK(client->replace_1("/r/x/y", defs, true) != 0);
        BOOST_CHECK_MESSAGE(client->errorMsg().find("/r/x/y has more than 3 entries") != std::string::npos,
                            client->errorMsg());
        auto administrator = server->administrator();
        BOOST_REQUIRE_EQUAL(administrator->getDefs(), 0);
        BOOST_CHECK(!administrator->defs()->findAbsNode("/r/x"));
    }
}

///
/// @brief Verifies that the receiving server of a plug applies its limit on the entries of an attribute: the plug is
///        refused, and both servers are unchanged.
///
BOOST_AUTO_TEST_CASE(test_permissions_entries_remote_plug) {
    ECF_NAME_THIS_TEST();

    auto sender   = start(R"("permissions_entries": 0)");
    auto receiver = start(R"("permissions_entries": 3)");
    auto client   = sender->client(alice);
    auto defs     = suite("s", 1);
    defs->findAbsNode("/s/task_with_a_long_name_0")
        ->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r,carol:r"));
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    BOOST_REQUIRE_EQUAL(receiver->client(alice)->load(suite("d", 0)), 0);

    BOOST_CHECK(client->plug("/s/task_with_a_long_name_0", "localhost:" + receiver->port() + "/d") != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("more than 3 entries") != std::string::npos, client->errorMsg());

    auto from = sender->administrator();
    BOOST_REQUIRE_EQUAL(from->getDefs(), 0);
    BOOST_CHECK(from->defs()->findAbsNode("/s/task_with_a_long_name_0"));
    auto to = receiver->administrator();
    BOOST_REQUIRE_EQUAL(to->getDefs(), 0);
    BOOST_CHECK(!to->defs()->findAbsNode("/d/task_with_a_long_name_0"));
}

///
/// @brief Verifies the framing of the limit on the size of a request over TCP/IP: a request at the limit is read, one
///        byte more is refused unread, even when announced alone or followed by a disconnection; the server keeps
///        serving.
///
BOOST_AUTO_TEST_CASE(test_request_size_tcp_framing) {
    ECF_NAME_THIS_TEST();

    auto server            = start(R"("request_size": 4096)");
    const std::string over = "exceeds the limit of 4096 bytes";

    // At the limit: read, decoded and answered (a --ping, padded to the limit)
    const auto accepted = send_raw(server->port(), header(4096) + ping_request(4096));
    BOOST_CHECK_MESSAGE(accepted.find("StcCmd") != std::string::npos, accepted);
    BOOST_CHECK(accepted.find("ErrorCmd") == std::string::npos);
    BOOST_CHECK_EQUAL(log_count(*server, over), 0U);

    // One byte more, announced without any data: refused at once, with a reply
    const auto reply = send_raw(server->port(), header(4097));
    BOOST_CHECK(!reply.empty());
    BOOST_CHECK_EQUAL(log_count(*server, over), 1U);

    // Announced, then disconnected without reading the reply
    (void)send_raw(server->port(), header(1000000), false);

    auto client = server->administrator();
    BOOST_CHECK_EQUAL(client->pingServer(), 0);
    BOOST_CHECK_MESSAGE(client->load(suite("small", 1)) == 0, client->errorMsg());
}

///
/// @brief Verifies the limit on the size of a request over HTTP: a larger request is answered with the status 413, one
///        at the limit is read; the server keeps serving.
///
BOOST_AUTO_TEST_CASE(test_request_size_http_status) {
    ECF_NAME_THIS_TEST();

    auto server = start(R"("request_size": 4096)", true);
    auto post   = [](std::size_t size) {
        return "POST /v1/ecflow HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/octet-stream\r\n"
                 "Content-Length: " +
               std::to_string(size) + "\r\n\r\n";
    };

    const auto refused = send_raw(server->port(), post(4097));
    BOOST_CHECK_MESSAGE(refused.rfind("HTTP/1.1 413", 0) == 0, refused);

    const auto read = send_raw(server->port(), post(4096) + ping_request(4096));
    BOOST_CHECK_MESSAGE(read.rfind("HTTP/1.1 200", 0) == 0, read);

    BOOST_CHECK_EQUAL(server->administrator()->pingServer(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

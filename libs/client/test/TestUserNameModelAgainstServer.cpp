// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/HttpLibrary.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

namespace {

const Credentials admin{"admin", "admin-secret"};
const Credentials alice{"alice", "alice-secret"};

const std::vector<std::string>& invalid_names() {
    static const std::vector<std::string> names{"al ice",
                                                "-bob",
                                                "*",
                                                "a:b",
                                                "a;b",
                                                "a,b",
                                                "a#b",
                                                std::string("a\x01") + "b",
                                                std::string("a\xc2\xa0") + "b",
                                                "a\xff"};
    return names;
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Sends a request over HTTP with the identity headers given, bypassing the checks of the client library; returns the
// error of the reply, empty when the request succeeded
std::string
send_with_headers(const std::string& port, CtsCmd::Api api, const std::string& user, const std::string& password) {
    auto cmd = std::make_shared<CtsCmd>(api);
    cmd->setup_user_authentification(admin.user, PasswordEncryption::encrypt(admin.password, admin.user));
    ClientToServerRequest request;
    request.set_cmd(cmd);
    std::string body;
    ecf::save_as_string(body, request);

    httplib::Client client("localhost", std::stoi(port));
    httplib::Headers headers{{"X-Auth-Username", user},
                             {"X-Auth-Password", ecf::test::encrypted_password(password, user)}};
    auto result = client.Post("/v1/ecflow", headers, body, "application/json");
    BOOST_REQUIRE_MESSAGE(result, "no HTTP reply");
    BOOST_REQUIRE_EQUAL(result->status, 200);
    ServerToClientResponse response;
    ecf::restore_from_string(result->body, response);
    BOOST_REQUIRE(response.get_cmd());
    return response.get_cmd()->error();
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_UserNameModel)

///
/// @brief Verifies that the client library refuses a user name outside the model before sending any request.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_by_client) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    for (const auto& name : invalid_names()) {
        auto client = server.client(Credentials{name, "secret"});
        BOOST_CHECK_MESSAGE(client->pingServer() != 0, "accepted: " << ecf::printable_user_name(name));
        BOOST_CHECK_MESSAGE(client->errorMsg().find("invalid user name") != std::string::npos,
                            ecf::printable_user_name(name) << ": " << client->errorMsg());
    }
    // Nothing reached the server: no authentication refusal was logged
    BOOST_CHECK_MESSAGE(read_file(server.home() / "server.log").find("invalid user name") == std::string::npos,
                        "a request reached the server");

    auto valid = server.client(alice);
    BOOST_CHECK_MESSAGE(valid->pingServer() == 0, valid->errorMsg());
}

///
/// @brief Verifies that the server refuses a request whose HTTP identity headers carry a user name outside the model.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_by_server_over_http) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(
        admin, {alice}, "", std::chrono::seconds(30), std::nullopt, std::nullopt, false, R"("http": true)");

    for (const auto api : {CtsCmd::PING, CtsCmd::STATS}) {
        BOOST_CHECK_EQUAL(send_with_headers(server.port(), api, alice.user, alice.password), "");
        for (const auto& name : invalid_names()) {
            if (name.find_first_of("\x01\xff") != std::string::npos) {
                continue; // not representable in an HTTP header field
            }
            const auto error = send_with_headers(server.port(), api, name, "secret");
            BOOST_CHECK_MESSAGE(error.find("Authentication (user) failed") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << error);
            BOOST_CHECK_MESSAGE(error.find("invalid user name") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << error);
        }
    }
}

// Writes a password file listing one user under the given password, for the host and localhost, optionally followed
// by an invalid entry; returns the number of the line holding the invalid entry
int write_password_file(const fs::path& path,
                        const std::string& port,
                        const std::vector<std::pair<std::string, std::string>>& entries,
                        bool with_invalid_entry) {
    std::ofstream file(path);
    file << "4.5.0\n";
    int lines = 1;
    for (const auto& [user, password] : entries) {
        for (const auto& host : {ecf::Host{}.name(), std::string("localhost")}) {
            file << user << " " << host << " " << port << " " << password << "\n";
            ++lines;
        }
    }
    if (with_invalid_entry) {
        file << "al:ice localhost " << port << " secret\n";
    }
    return lines + 1;
}

// Returns whether an authenticated request of the client succeeds
bool authenticated(ClientInvoker& client) {
    return client.getDefs() == 0;
}

///
/// @brief Verifies that reloading a custom password file holding a user name outside the model fails, naming the line,
///        and leaves the previous content in force: the previous password of a user is still accepted, the new one not.
///
BOOST_AUTO_TEST_CASE(test_reload_custom_password_file_with_invalid_user_name) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    const auto file = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), Ecf::CUSTOM_PASSWD_FILE());
    const auto line = write_password_file(
        file, server.port(), {{admin.user, admin.password}, {alice.user, "alice-replacement"}}, true);

    auto client = server.administrator();
    BOOST_CHECK(client->reloadcustompasswdfile() != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("invalid user name 'al:ice'") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("line " + std::to_string(line)) != std::string::npos,
                        client->errorMsg());

    BOOST_CHECK_MESSAGE(authenticated(*server.client(alice)), "previous password refused");
    BOOST_CHECK_MESSAGE(!authenticated(*server.client(Credentials{alice.user, "alice-replacement"})),
                        "replacement password accepted");
}

///
/// @brief Verifies that reloading a password file holding a user name outside the model fails, naming the line, and
///        leaves the previous content in force: the previous password of the login user is still accepted, the new
///        one not.
///
BOOST_AUTO_TEST_CASE(test_reload_password_file_with_invalid_user_name) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    const auto login = ecf::get_login_name();
    const auto file  = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), Ecf::PASSWD_FILE());

    auto client = server.administrator();
    write_password_file(file, server.port(), {{login, "login-previous"}}, false);
    BOOST_REQUIRE_MESSAGE(client->reloadpasswdfile() == 0, client->errorMsg());

    const auto line = write_password_file(file, server.port(), {{login, "login-replacement"}}, true);
    BOOST_CHECK(client->reloadpasswdfile() != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("invalid user name 'al:ice'") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("line " + std::to_string(line)) != std::string::npos,
                        client->errorMsg());

    auto as_login = [&](const std::string& password) {
        auto c = server.client(alice);
        c->set_user_name(""); // the login user, authenticated by the password file
        c->set_password(PasswordEncryption::encrypt(password, login));
        return authenticated(*c);
    };
    BOOST_CHECK_MESSAGE(as_login("login-previous"), "previous password refused");
    BOOST_CHECK_MESSAGE(!as_login("login-replacement"), "replacement password accepted");
}

///
/// @brief Verifies that the command line refuses a user name outside the model given with --user, ECF_USER or
///        --ch_drop_user, before anything is sent.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_by_command_line) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice}, "");
    const std::vector<std::string> base{"ecflow_client", "--host=localhost", "--port=" + server.port()};

    for (const auto& option : {std::string("--user=a:b"), std::string("--ch_drop_user=a b")}) {
        ClientInvoker client;
        client.set_throw_on_error(false);
        auto args = base;
        args.push_back(option);
        if (option.rfind("--user", 0) == 0) {
            args.emplace_back("--ping");
        }
        BOOST_CHECK_MESSAGE(client.invoke(args) != 0, option << " accepted");
        BOOST_CHECK_MESSAGE(client.errorMsg().find("invalid user name") != std::string::npos,
                            option << ": " << client.errorMsg());
    }
    {
        ecf::test::scaffold::WithTestEnvironmentVariable user("ECF_USER", "a;b");
        ClientInvoker client;
        client.set_throw_on_error(false);
        auto args = base;
        args.emplace_back("--ping");
        BOOST_CHECK_MESSAGE(client.invoke(args) != 0, "ECF_USER accepted");
        BOOST_CHECK_MESSAGE(client.errorMsg().find("invalid user name 'a;b'") != std::string::npos, client.errorMsg());
    }
    BOOST_CHECK_MESSAGE(read_file(server.home() / "server.log").find("invalid user name") == std::string::npos,
                        "a request reached the server");
}

///
/// @brief Verifies that reloading a white list holding a user name outside the model fails, naming the line, and
///        leaves the previous content in force.
///
BOOST_AUTO_TEST_CASE(test_reload_white_list_with_invalid_user_name) {
    ECF_NAME_THIS_TEST();

    PermissionsServer server(admin, {alice, Credentials{"bob", "bob-secret"}}, "");
    const auto file = server.home() / ecf::Host{}.ecf_lists_file(server.port());
    std::ofstream(file) << "4.4.14\nadmin\nalice\n";
    auto client = server.administrator();
    BOOST_REQUIRE_MESSAGE(client->reloadwsfile() == 0, client->errorMsg());

    std::ofstream(file) << "4.4.14\nadmin\nalice\nbob\n-a"
                           "\xc2\xa0"
                           "b\n";
    BOOST_CHECK(client->reloadwsfile() != 0);
    BOOST_CHECK_MESSAGE(client->errorMsg().find("invalid user name") != std::string::npos, client->errorMsg());
    BOOST_CHECK_MESSAGE(client->errorMsg().find("line 5") != std::string::npos, client->errorMsg());

    // The previous content stays in force: alice may write, bob may not
    BOOST_CHECK_MESSAGE(server.client(alice)->checkPtDefs(ecf::CheckPt::NEVER) == 0, "alice refused");
    BOOST_CHECK_MESSAGE(server.client(Credentials{"bob", "bob-secret"})->checkPtDefs(ecf::CheckPt::NEVER) != 0,
                        "bob accepted");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

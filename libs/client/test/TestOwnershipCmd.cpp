// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <unistd.h>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_OwnershipCmd)

namespace {

// Custom users, authenticated by the custom password file; none of them is an account of the host
const std::string writer = "ecfownernosuchuser"; // write access
const std::string reader = "ecfownerreader";     // read access only (white list)

/// The lines of a file, joined for WithTestFile
std::string as_content(const std::vector<std::string>& lines) {
    std::string content;
    for (const auto& line : lines) {
        content += line + "\n";
    }
    return content;
}

std::string server_defs(ClientInvoker& client) {
    client.set_user_name(get_login_name());
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    return ecf::as_string(*client.defs(), PrintStyle::DEFS);
}

/// Runs an ownership operation as the given user, expecting it to be refused for the expected reason
void check_refused(ClientInvoker& client,
                   const std::string& user,
                   const std::string& op,
                   const std::string& path,
                   const std::string& value,
                   const std::string& expected_in_reason) {
    const std::string before = server_defs(client);
    client.set_user_name(user);
    BOOST_CHECK_MESSAGE(client.ownership(op, path, value) == 1, "Expected " << user << " to be refused: " << op);
    BOOST_CHECK_MESSAGE(client.errorMsg().find(expected_in_reason) != std::string::npos,
                        "Unexpected error: " << client.errorMsg());
    BOOST_CHECK_EQUAL(server_defs(client), before);
}

} // namespace

BOOST_AUTO_TEST_CASE(test_ownership_against_server) {
    ECF_NAME_THIS_TEST();

    if (!ClientEnvironment::hostSpecified().empty()) {
        std::cout << "...Client:: ...test_ownership_against_server ignoring when HOST specified\n";
        return; // the password and white list files must be set up for the server
    }

    Host the_host;
    const std::string port        = SCPort::next();
    const std::string passwd_file = the_host.ecf_custom_passwd_file(port);
    const std::string lists_file  = the_host.ecf_lists_file(port);
    const std::string login       = get_login_name();
    const std::string password    = "xxxx";

    // Removed, and the environment restored, even when the test stops early
    std::vector<std::string> passwd_lines{"4.5.0"};
    for (const auto& user : {login, writer, reader}) {
        passwd_lines.push_back(user + " " + the_host.name() + " " + port + " " + password);
        passwd_lines.push_back(user + " localhost " + port + " " + password);
    }
    ecf::test::scaffold::WithTestFile passwd(ecf::test::scaffold::NamedTestFile{passwd_file}, as_content(passwd_lines));
    ecf::test::scaffold::WithTestFile lists(ecf::test::scaffold::NamedTestFile{lists_file},
                                            as_content({"4.4.14", login, writer, "-" + reader}));
    ecf::test::scaffold::WithTestEnvironmentVariable custom_passwd("ECF_CUSTOM_PASSWD", passwd.path().string());
    ecf::test::scaffold::WithTestEnvironmentVariable custom_user("ECF_USER", login);

    {
        InvokeServer invokeServer("Client:: ...test_ownership_against_server", port);
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                              "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

        ClientInvoker client(invokeServer.host(), invokeServer.port());
        client.set_throw_on_error(false);

        defs_ptr defs = Defs::create();
        defs->add_suite("s")->add_family("f")->add_task("t");
        BOOST_REQUIRE_MESSAGE(client.load(defs) == 0, client.errorMsg());

        // read, by anyone with read access
        for (const auto& user : {login, reader}) {
            client.set_user_name(user);
            BOOST_REQUIRE_MESSAGE(client.ownership("read", "/s/f/t") == 0, client.errorMsg());
            BOOST_CHECK_EQUAL(client.get_string(), "owner=none\neffective=none\norigin=none\n");
            BOOST_REQUIRE_MESSAGE(client.ownership("read", "/") == 0, client.errorMsg());
            BOOST_CHECK_EQUAL(client.get_string(), "effective=none\norigin=none\n");
        }

        // the login runs the server: it cannot be an owner (nor can the superuser, when tests run as root)
        check_refused(client, login, "create", "/s", login, ::geteuid() == 0 ? "superuser" : "server account");
        // a custom user claiming an account that does not exist on the host
        check_refused(client, writer, "create", "/s", writer, "unknown user");
        // a user without write access is stopped by the existing authorisation
        check_refused(client, reader, "create", "/s", reader, "Command not accepted");
        // nothing to update or delete
        check_refused(client, writer, "delete", "/s", "", "has no Owner");
        // the server Owner is configuration (checked by the client)
        client.set_user_name(login);
        BOOST_CHECK(client.ownership("delete", "/") == 1);
        BOOST_CHECK_MESSAGE(client.errorMsg().find("server Owner") != std::string::npos, client.errorMsg());

        client.set_user_name(login);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

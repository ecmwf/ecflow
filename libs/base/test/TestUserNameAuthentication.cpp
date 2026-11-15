// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_user_name_authentication.log"};
};

const std::vector<std::string>& invalid_names() {
    static const std::vector<std::string> names{"",
                                                "al ice",
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

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_UserNameAuthentication, LogFixture)

///
/// @brief Verifies that a request carrying a user name outside the model is refused by the authentication, with the
///        reason, whatever the kind of identity, and that the request has no effect.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_by_authentication) {
    ECF_NAME_THIS_TEST();

    for (const auto& name : invalid_names()) {
        for (const auto& identity : {ecf::Identity::make_user(name, ""),
                                     ecf::Identity::make_custom_user(name, ""),
                                     ecf::Identity::make_secure_user(name)}) {
            auto defs = Defs::create();
            defs->add_suite("s");
            MockServer server(defs);

            auto cmd = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s");
            ClientToServerRequest request;
            request.set_cmd(cmd);        // which sets up the identity of the login user,
            cmd->set_identity(identity); // replaced by the identity under test
            const auto reply = request.handleRequest(&server);

            BOOST_REQUIRE(reply);
            const auto& error = reply->error();
            BOOST_CHECK_MESSAGE(error.find("Authentication (user) failed") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << error);
            BOOST_CHECK_MESSAGE(error.find("invalid user name") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << error);
            BOOST_CHECK_MESSAGE(!defs->findAbsNode("/s")->isSuspended(), ecf::printable_user_name(name));
            // The reply, which is also the log record, shows the name in its printable form only
            for (unsigned char c : error) {
                BOOST_CHECK_MESSAGE(c >= 0x20 && c != 0x7F && c != 0xFF,
                                    ecf::printable_user_name(name)
                                        << ": raw byte in " << ecf::printable_user_name(error));
            }
        }
    }
}

///
/// @brief Verifies that a request whose own user name is outside the model, and printed by the request (e.g.
///        --ch_drop_user without argument), is refused with a reply holding no raw control character or invalid byte.
///
BOOST_AUTO_TEST_CASE(test_invalid_request_user_never_reaches_the_reply_raw) {
    ECF_NAME_THIS_TEST();

    for (const auto& name : invalid_names()) {
        if (name.empty()) {
            continue; // an empty request user is replaced by the login name; see the identity cases above
        }
        for (const Cmd_ptr& cmd : {Cmd_ptr(std::make_shared<ClientHandleCmd>(std::string{})),
                                   Cmd_ptr(std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s"))}) {
            auto defs = Defs::create();
            defs->add_suite("s");
            MockServer server(defs);

            cmd->setup_user_authentification(name, ""); // the user of the request, and its identity
            ClientToServerRequest request;
            request.set_cmd(cmd);
            const auto reply = request.handleRequest(&server);

            const auto& error = reply->error();
            BOOST_CHECK_MESSAGE(error.find("invalid user name") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << error);
            for (unsigned char c : error) {
                BOOST_CHECK_MESSAGE(c >= 0x20 && c != 0x7F && c != 0xFF,
                                    ecf::printable_user_name(name)
                                        << ": raw byte in " << ecf::printable_user_name(error));
            }
        }
    }
}

///
/// @brief Verifies that a ping, which needs no credentials, is also refused for a user name outside the model.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_for_ping) {
    ECF_NAME_THIS_TEST();

    for (const auto& name : invalid_names()) {
        auto defs = Defs::create();
        MockServer server(defs);

        auto cmd = std::make_shared<CtsCmd>(CtsCmd::PING);
        ClientToServerRequest request;
        request.set_cmd(cmd);
        cmd->set_identity(ecf::Identity::make_user(name, ""));
        const auto reply = request.handleRequest(&server);
        BOOST_CHECK_MESSAGE(reply->error().find("invalid user name") != std::string::npos,
                            ecf::printable_user_name(name) << ": " << reply->error());
    }

    auto defs = Defs::create();
    MockServer server(defs);
    const auto reply = TestHelper::invokeRequestAs(&server, std::make_shared<CtsCmd>(CtsCmd::PING), "alice");
    BOOST_CHECK_MESSAGE(reply->error().empty(), reply->error());
}

///
/// @brief Verifies that a valid user name passes the check of the model.
///
BOOST_AUTO_TEST_CASE(test_valid_user_name_accepted_by_authentication) {
    ECF_NAME_THIS_TEST();

    for (const std::string name : {"alice", "jos\xc3\xa9", "*bob", "x-y"}) {
        auto defs = Defs::create();
        defs->add_suite("s");
        MockServer server(defs);

        auto cmd         = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s");
        const auto reply = TestHelper::invokeRequestAs(&server, cmd, name);
        BOOST_CHECK_MESSAGE(reply->error().empty(), name << ": " << reply->error());
        BOOST_CHECK_MESSAGE(defs->findAbsNode("/s")->isSuspended(), name);
    }
}

///
/// @brief Verifies that a user name given as the argument of --ch_drop_user follows the model.
///
BOOST_AUTO_TEST_CASE(test_invalid_user_name_refused_as_argument) {
    ECF_NAME_THIS_TEST();

    for (const auto& name : invalid_names()) {
        if (name.empty()) {
            continue; // an empty argument means the requester itself
        }
        auto defs = Defs::create();
        MockServer server(defs);
        auto cmd = std::make_shared<ClientHandleCmd>(name);
        // The request fails; the server answers the client with the message of the exception
        try {
            const auto reply = TestHelper::invokeRequestAs(&server, cmd, "alice");
            BOOST_ERROR("accepted: " << ecf::printable_user_name(name) << ": " << reply->error());
        }
        catch (const std::runtime_error& e) {
            BOOST_CHECK_MESSAGE(std::string(e.what()).find("invalid user name") != std::string::npos,
                                ecf::printable_user_name(name) << ": " << e.what());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

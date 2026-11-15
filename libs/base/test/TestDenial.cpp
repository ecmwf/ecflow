// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_denial.log"};
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_Denial, LogFixture)

///
/// @brief Verifies that the kind of an error reply survives serialisation, and that a reply without it is an error.
///
BOOST_AUTO_TEST_CASE(test_denial_kind_serialisation) {
    ECF_NAME_THIS_TEST();

    for (const auto kind : {ErrorCmd::Kind::Error, ErrorCmd::Kind::PermissionDenied}) {
        const ErrorCmd original("message", kind);
        std::string data;
        ecf::save_as_string(data, original);
        BOOST_CHECK_EQUAL(data.find("kind_") != std::string::npos, kind == ErrorCmd::Kind::PermissionDenied);
        ErrorCmd restored;
        ecf::restore_from_string(data, restored);
        BOOST_CHECK(restored.kind() == kind);
        BOOST_CHECK_EQUAL(restored.error(), "message");
    }
}

///
/// @brief Verifies that a request refused by the authorisation is denied, with a message naming the reason, the request
///        and its requester, also written to the server log (D5.29, D5.30).
///
BOOST_AUTO_TEST_CASE(test_denial_reply) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto defs = Defs::create();
    defs->add_suite("secret_suite");
    MockServer server(defs);
    WithTestFile file(AutomaticTestFile{"whitelist"}, "4.5.0\n-bob\n");
    WhiteListFile whitelist;
    std::string error;
    BOOST_REQUIRE_MESSAGE(whitelist.load(file.path().string(), error), error);
    server.set_authorisation(ecf::AuthorisationService::load_permissions_from_whitelist(whitelist).value());

    auto cmd           = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/secret_suite");
    const auto reply   = TestHelper::invokeRequestAs(&server, cmd, "bob");
    const auto* denied = dynamic_cast<const ErrorCmd*>(reply.get());
    BOOST_REQUIRE(denied != nullptr);
    BOOST_CHECK(denied->kind() == ErrorCmd::Kind::PermissionDenied);
    const std::string expected = "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient "
                                 "permissions (write access required at /secret_suite) [--suspend /secret_suite :bob@";
    BOOST_CHECK_MESSAGE(denied->error().rfind(expected, 0) == 0, denied->error());

    // The server log holds the same message
    { ecf::LogFlusher flusher; }
    std::ifstream log(ecf::Log::instance()->path());
    const std::string text{std::istreambuf_iterator<char>(log), {}};
    BOOST_CHECK_MESSAGE(text.find(denied->error()) != std::string::npos, text);

    // The client records the denial
    ServerReply client;
    reply->handle_server_response(client, cmd, false);
    BOOST_CHECK(client.permission_denied());
    BOOST_CHECK_MESSAGE(client.get_error_msg().find(expected) != std::string::npos, client.get_error_msg());

    // An error that is not a denial is not recorded as one
    ServerReply other;
    ErrorCmd("failed", ErrorCmd::Kind::Error).handle_server_response(other, cmd, false);
    BOOST_CHECK(!other.permission_denied());
}

namespace {

///
/// @brief Mirrors ErrorCmd as serialised before the kind existed, to check compatibility in both directions.
///
struct ErrorCmdBeforeKind
{
    std::string error_msg_;

    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(&base_), CEREAL_NVP(error_msg_));
    }

    struct Base : ServerToClientCmd
    {
        bool handle_server_response(ServerReply&, Cmd_ptr, bool) const override { return false; }
        bool equals(ServerToClientCmd*) const override { return true; }
        std::string print() const override { return {}; }
    } base_;
};

} // namespace

///
/// @brief Verifies that a denial whose request holds line breaks is answered, and logged, on one line.
///
BOOST_AUTO_TEST_CASE(test_denial_on_one_line) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto defs = Defs::create();
    defs->add_suite("s");
    MockServer server(defs);
    WithTestFile file(AutomaticTestFile{"whitelist"}, "4.5.0\n-bob\n");
    WhiteListFile whitelist;
    std::string error;
    BOOST_REQUIRE_MESSAGE(whitelist.load(file.path().string(), error), error);
    server.set_authorisation(ecf::AuthorisationService::load_permissions_from_whitelist(whitelist).value());

    auto cmd         = std::make_shared<PathsCmd>(PathsCmd::SUSPEND, "/s\nERR:[00:00:00 1.1.2026] forged\r\nline");
    const auto reply = TestHelper::invokeRequestAs(&server, cmd, "bob");
    BOOST_REQUIRE(reply->error().find("Command not accepted") == 0);
    BOOST_CHECK(reply->error().find_first_of("\r\n") == std::string::npos);

    { ecf::LogFlusher flusher; }
    std::ifstream log(ecf::Log::instance()->path());
    std::size_t lines = 0;
    for (std::string line; std::getline(log, line);) {
        if (line.find("forged") != std::string::npos) {
            ++lines;
            BOOST_CHECK_MESSAGE(line.find("Command not accepted") != std::string::npos, line);
        }
    }
    BOOST_CHECK_EQUAL(lines, 1U);
}

///
/// @brief Verifies that an error reply is read by a peer that predates the kind, and that a reply of such a peer is
///        read as an error.
///
BOOST_AUTO_TEST_CASE(test_denial_kind_compatibility) {
    ECF_NAME_THIS_TEST();

    // A peer that predates the kind reads the message of a denial, and ignores the kind
    for (const auto kind : {ErrorCmd::Kind::Error, ErrorCmd::Kind::PermissionDenied}) {
        std::string data;
        ecf::save_as_string(data, ErrorCmd("Permission denied: --suspend", kind));
        ErrorCmdBeforeKind older;
        ecf::restore_from_string(data, older);
        BOOST_CHECK_EQUAL(older.error_msg_, "Permission denied: --suspend");
    }

    // A reply written by such a peer is read as an error
    ErrorCmdBeforeKind older;
    older.error_msg_ = "message";
    std::string data;
    ecf::save_as_string(data, older);
    ErrorCmd restored;
    ecf::restore_from_string(data, restored);
    BOOST_CHECK(restored.kind() == ErrorCmd::Kind::Error);
    BOOST_CHECK_EQUAL(restored.error(), "message");

    // The kind survives the polymorphic reply envelope
    ServerToClientResponse response;
    response.set_cmd(std::make_shared<ErrorCmd>("Permission denied: --suspend", ErrorCmd::Kind::PermissionDenied));
    std::string envelope;
    ecf::save_as_string(envelope, response);
    ServerToClientResponse received;
    ecf::restore_from_string(envelope, received);
    const auto* denial = dynamic_cast<const ErrorCmd*>(received.get_cmd().get());
    BOOST_REQUIRE(denial != nullptr);
    BOOST_CHECK(denial->kind() == ErrorCmd::Kind::PermissionDenied);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

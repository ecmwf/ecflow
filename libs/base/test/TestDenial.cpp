// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/DenialLog.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
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
/// @brief Verifies that a request refused by the authorisation is denied, with a message naming no node.
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
    BOOST_CHECK_EQUAL(denied->error(), "Permission denied: --suspend");

    // The client records the denial
    ServerReply client;
    reply->handle_server_response(client, cmd, false);
    BOOST_CHECK(client.permission_denied());
    // The part of the message from the server names no node; the client prefixes the request as given
    const auto& message    = client.get_error_msg();
    const auto server_part = message.substr(message.find("Server reply: "));
    BOOST_CHECK_EQUAL(server_part, "Server reply: Permission denied: --suspend\n");

    // An error that is not a denial is not recorded as one
    ServerReply other;
    ErrorCmd("failed", ErrorCmd::Kind::Error).handle_server_response(other, cmd, false);
    BOOST_CHECK(!other.permission_denied());
}

///
/// @brief Verifies that denials are recorded at a bounded rate per key, counting those suppressed.
///
BOOST_AUTO_TEST_CASE(test_denial_log_rate) {
    ECF_NAME_THIS_TEST();

    using std::chrono::seconds;
    ecf::DenialLog log(seconds(10), 3);
    const auto t0          = ecf::DenialLog::Clock::time_point{};
    std::size_t suppressed = 99;

    BOOST_CHECK(log.record("bob --suspend", t0, suppressed));
    BOOST_CHECK_EQUAL(suppressed, 0U);
    BOOST_CHECK(!log.record("bob --suspend", t0 + seconds(1), suppressed));
    BOOST_CHECK(!log.record("bob --suspend", t0 + seconds(9), suppressed));
    BOOST_CHECK(log.record("bob --resume", t0 + seconds(9), suppressed));
    BOOST_CHECK(log.record("bob --suspend", t0 + seconds(10), suppressed));
    BOOST_CHECK_EQUAL(suppressed, 2U);

    // The number of keys stays bounded
    BOOST_CHECK(log.record("carol --suspend", t0 + seconds(11), suppressed));
    BOOST_CHECK(log.record("dave --suspend", t0 + seconds(12), suppressed));
    BOOST_CHECK_LE(log.keys(), 3U);
}

///
/// @brief Verifies that, with every key tracked within its period, new keys share one limited overflow key, and the
///        keys tracked keep their limits.
///
BOOST_AUTO_TEST_CASE(test_denial_log_saturated) {
    ECF_NAME_THIS_TEST();

    using std::chrono::seconds;
    ecf::DenialLog log(seconds(10), 2);
    const auto t0          = ecf::DenialLog::Clock::time_point{};
    std::size_t suppressed = 99;
    bool overflow          = true;

    BOOST_CHECK(log.record("a", t0, suppressed, overflow));
    BOOST_CHECK(!overflow);
    BOOST_CHECK(log.record("b", t0, suppressed, overflow));

    // Churn through new keys: one record for all of them per period, marked as an overflow
    BOOST_CHECK(log.record("c", t0 + seconds(1), suppressed, overflow));
    BOOST_CHECK_EQUAL(suppressed, 0U);
    BOOST_CHECK(overflow);
    for (int i = 0; i < 1000; ++i) {
        BOOST_CHECK(!log.record("x" + std::to_string(i), t0 + seconds(2), suppressed, overflow));
    }
    BOOST_CHECK_EQUAL(log.keys(), 2U);

    // The keys tracked are still limited, whatever the churn
    BOOST_CHECK(!log.record("a", t0 + seconds(3), suppressed, overflow));
    BOOST_CHECK(!log.record("b", t0 + seconds(3), suppressed, overflow));

    // Once its period has elapsed, a key tracked reports what was suppressed
    BOOST_CHECK(log.record("a", t0 + seconds(10), suppressed, overflow));
    BOOST_CHECK_EQUAL(suppressed, 1U);
    BOOST_CHECK(!overflow);

    // b has expired, which makes room for c; c was recorded by the overflow key 9 s ago, and is not recorded again
    // before the period has elapsed
    BOOST_CHECK(!log.record("c", t0 + seconds(10), suppressed, overflow));
    BOOST_CHECK_EQUAL(log.keys(), 2U);
    BOOST_CHECK(log.record("c", t0 + seconds(11), suppressed, overflow));
    BOOST_CHECK_EQUAL(suppressed, 1U);
    BOOST_CHECK(!overflow);

    // The overflow key reports every denial it suppressed, of whichever key, marked as an overflow
    BOOST_CHECK(log.record("z", t0 + seconds(11), suppressed, overflow)); // a and c are within their period
    BOOST_CHECK_EQUAL(suppressed, 1000U);
    BOOST_CHECK(overflow);
}

///
/// @brief Parses a record of a denial into its fields, undoing the quoting.
///
std::map<std::string, std::string> parse_denial(const std::string& record) {
    std::map<std::string, std::string> fields;
    BOOST_REQUIRE(record.rfind("DENIED ", 0) == 0);
    std::size_t i = 7;
    while (i < record.size()) {
        const auto equal = record.find('=', i);
        BOOST_REQUIRE(equal != std::string::npos);
        const auto name = record.substr(i, equal - i);
        std::string value;
        i = equal + 1;
        if (record[i] == '\'') {
            for (++i; record.at(i) != '\''; ++i) {
                if (record[i] == '\\') {
                    ++i;
                }
                value += record.at(i);
            }
            ++i;
        }
        else {
            const auto end = record.find(' ', i);
            value          = record.substr(i, end == std::string::npos ? std::string::npos : end - i);
            i              = end == std::string::npos ? record.size() : end;
        }
        fields[name] = value;
        if (i < record.size()) {
            BOOST_REQUIRE_EQUAL(record[i], ' ');
            ++i;
        }
    }
    return fields;
}

///
/// @brief Verifies that a record of a denial is a single line, parsed back into its fields, whatever their content.
///
BOOST_AUTO_TEST_CASE(test_denial_record_format) {
    ECF_NAME_THIS_TEST();

    const auto record = ecf::DenialLog::format(
        "o'brien", "host\\name", "--alter", "--alter change variable X 'a b' /s'\nx", "reason with = and '", 7);
    BOOST_CHECK(record.find('\n') == std::string::npos);
    BOOST_CHECK(record.find('\r') == std::string::npos);

    const auto fields = parse_denial(record);
    BOOST_CHECK_EQUAL(fields.size(), 6U);
    BOOST_CHECK_EQUAL(fields.at("user"), "o'brien");
    BOOST_CHECK_EQUAL(fields.at("host"), "host\\name");
    BOOST_CHECK_EQUAL(fields.at("command"), "--alter");
    BOOST_CHECK_EQUAL(fields.at("request"), "--alter change variable X 'a b' /s' x"); // the line break is a space
    BOOST_CHECK_EQUAL(fields.at("reason"), "reason with = and '");
    BOOST_CHECK_EQUAL(fields.at("suppressed"), "7");

    // A record decided by the overflow key says so
    const auto overflow = parse_denial(ecf::DenialLog::format("bob", "h", "--resume", "--resume /s", "r", 3, true));
    BOOST_CHECK_EQUAL(overflow.at("overflow"), "yes");
    BOOST_CHECK_EQUAL(fields.count("overflow"), 0U);
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

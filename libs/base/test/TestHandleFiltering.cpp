// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The client handles in Permissions mode (R5.2, D10.7): registering a suite the requester cannot read is denied,
/// the suites a handle holds are sent only while readable, the suites added automatically are filtered, and
/// --ch_suites lists the requester's own handles with their readable suites.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_handle_filtering.log"};
};

const std::string alice_only = "admin:rwxlop,alice:r";

///
/// @brief Builds the definitions of the tests: suite pub without attribute, and suite privsuite, readable by alice
///        only unless open.
///
defs_ptr reference(bool open = false) {
    auto defs = Defs::create();
    defs->add_suite("pub")->add_task("t");
    auto priv = defs->add_suite("privsuite");
    priv->add_task("t");
    if (!open) {
        priv->add_permissions(Permissions::parse_cli(alice_only));
    }
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlops,alice:r,bob:r"));
    return defs;
}

///
/// @brief Handles a request as a user, and returns the reply as received by the client.
///
/// The client side builds nodes, which moves the change numbers of the process; they are restored afterwards, since
/// the client and the server of these tests share the process, unlike real ones.
///
STC_Cmd_ptr request(MockServer& server, const Cmd_ptr& command, const std::string& user, std::string* wire = nullptr) {
    const auto reply = TestHelper::invokeRequestAs(&server, command, user);
    ServerToClientResponse response;
    response.set_cmd(reply);
    std::string data;
    ecf::save_as_string(data, response);
    if (wire) {
        *wire = data;
    }
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    ServerToClientResponse received;
    ecf::restore_from_string(data, received);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    return received.get_cmd();
}

bool denied(const STC_Cmd_ptr& reply) {
    const auto* error = dynamic_cast<const ErrorCmd*>(reply.get());
    return error && error->kind() == ErrorCmd::Kind::PermissionDenied;
}

int register_handle(MockServer& server,
                    const std::string& user,
                    const std::vector<std::string>& suites,
                    bool auto_add) {
    const auto reply = request(server, std::make_shared<ClientHandleCmd>(0, suites, auto_add), user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    return static_cast<int>(server.defs()->client_suite_mgr().clientSuites().back().handle());
}

///
/// @brief Returns the suites received by a full synchronisation through a handle.
///
std::set<std::string> suites_received(MockServer& server, const std::string& user, int handle) {
    const auto command = std::make_shared<CSyncCmd>(handle);
    const auto reply   = request(server, command, user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    ServerReply server_reply;
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    reply->handle_server_response(server_reply, command, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    std::set<std::string> result;
    for (const auto& suite : server_reply.client_defs()->suites()) {
        result.insert(suite->name());
    }
    return result;
}

///
/// @brief Returns the names of the suites a handle holds, as recorded by the server.
///
std::set<std::string> registered(MockServer& server, int handle) {
    std::vector<std::string> names;
    server.defs()->client_suite_mgr().suites(static_cast<unsigned int>(handle), names);
    return {names.begin(), names.end()};
}

///
/// @brief Loads definitions holding the given suites, each with a task; those named in hidden are readable by alice
///        only.
///
void load(MockServer& server, const std::vector<std::string>& suites, const std::vector<std::string>& hidden) {
    auto defs = Defs::create();
    for (const auto& name : suites) {
        auto suite = defs->add_suite(name);
        suite->add_task("t");
        if (std::find(hidden.begin(), hidden.end(), name) != hidden.end()) {
            suite->add_permissions(Permissions::parse_cli(alice_only));
        }
    }
    const auto reply = request(server, std::make_shared<LoadDefsCmd>(defs, true), "admin");
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
}

///
/// @brief Replaces suite privsuite by one with the given attribute (none when empty), as --replace does.
///
void replace_privsuite(MockServer& server, const std::string& attribute) {
    auto replacement = Defs::create();
    auto suite       = replacement->add_suite("privsuite");
    suite->add_task("t");
    if (!attribute.empty()) {
        suite->add_permissions(Permissions::parse_cli(attribute));
    }
    const auto reply =
        request(server, std::make_shared<ReplaceNodeCmd>("/privsuite", false, replacement, true), "admin");
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
}

#define CHECK_SUITES(actual, ...)                                              \
    do {                                                                       \
        const auto a = (actual);                                               \
        const std::set<std::string> e __VA_ARGS__;                             \
        BOOST_CHECK_EQUAL_COLLECTIONS(a.begin(), a.end(), e.begin(), e.end()); \
    } while (false)

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_HandleFiltering, LogFixture)

///
/// @brief Verifies that registering, or adding to a handle, a suite the requester cannot read is denied, naming the
///        suite, and changes no handle.
///
BOOST_AUTO_TEST_CASE(test_register_unreadable_suite_is_denied) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    const auto handles = server.defs()->client_suite_mgr().clientSuites().size();

    const auto refused = request(
        server, std::make_shared<ClientHandleCmd>(0, std::vector<std::string>{"pub", "privsuite"}, false), "bob");
    BOOST_CHECK(denied(refused));
    BOOST_CHECK_MESSAGE(refused->error().find("role r required at /privsuite") != std::string::npos, refused->error());
    BOOST_CHECK_EQUAL(server.defs()->client_suite_mgr().clientSuites().size(), handles);

    const auto handle = register_handle(server, "bob", {"pub"}, false);
    const auto added =
        request(server,
                std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"privsuite"}, ClientHandleCmd::ADD),
                "bob");
    BOOST_CHECK(denied(added));
    CHECK_SUITES(registered(server, handle), {"pub"});
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub"});

    // A request naming a readable and an unreadable suite is denied as a whole
    const auto mixed = request(server,
                               std::make_shared<ClientHandleCmd>(
                                   handle, std::vector<std::string>{"future", "privsuite"}, ClientHandleCmd::ADD),
                               "bob");
    BOOST_CHECK(denied(mixed));
    CHECK_SUITES(registered(server, handle), {"pub"});

    // alice reads both
    const auto alice = register_handle(server, "alice", {"pub", "privsuite"}, false);
    CHECK_SUITES(suites_received(server, "alice", alice), {"pub", "privsuite"});
}

///
/// @brief Verifies that a registered suite that becomes unreadable is no longer sent, and is sent again once readable,
///        while it stays registered.
///
BOOST_AUTO_TEST_CASE(test_registered_suite_follows_read) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference(true));
    const auto handle = register_handle(server, "bob", {"pub", "privsuite"}, false);
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub", "privsuite"});

    replace_privsuite(server, alice_only); // loses read
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub"});

    replace_privsuite(server, ""); // regains read
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub", "privsuite"});
}

///
/// @brief Verifies that the suites added automatically to a handle are sent only when readable.
///
BOOST_AUTO_TEST_CASE(test_auto_added_suites_are_filtered) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    const auto handle = register_handle(server, "bob", {"pub"}, true);

    auto more = Defs::create();
    more->add_suite("newpub")->add_task("t");
    auto newpriv = more->add_suite("newpriv");
    newpriv->add_task("t");
    newpriv->add_permissions(Permissions::parse_cli(alice_only));
    const auto loaded = request(server, std::make_shared<LoadDefsCmd>(more, false), "admin");
    BOOST_REQUIRE_MESSAGE(loaded->error().empty(), loaded->error());

    CHECK_SUITES(suites_received(server, "bob", handle), {"pub", "newpub"});
    CHECK_SUITES(registered(server, handle), {"pub", "newpub", "newpriv"}); // added, though not sent

    // Once readable, the suite added automatically is sent, without registering it again
    load(server, {"newpriv"}, {});
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub", "newpub", "newpriv"});
}

///
/// @brief Verifies that suites that do not exist yet can be registered and added, are listed, and once loaded are sent
///        only when readable.
///
BOOST_AUTO_TEST_CASE(test_pre_registration) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference());
    const auto handle = register_handle(server, "bob", {"pub", "future"}, false);
    const auto added =
        request(server,
                std::make_shared<ClientHandleCmd>(handle, std::vector<std::string>{"future2"}, ClientHandleCmd::ADD),
                "bob");
    BOOST_REQUIRE_MESSAGE(added->error().empty(), added->error());
    CHECK_SUITES(registered(server, handle), {"pub", "future", "future2"});

    std::string wire;
    request(server, std::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES), "bob", &wire);
    BOOST_CHECK_MESSAGE(wire.find("future2") != std::string::npos, "the listing misses a suite not loaded yet");

    load(server, {"future", "future2"}, {"future2"});
    CHECK_SUITES(suites_received(server, "bob", handle), {"pub", "future"});
    request(server, std::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES), "bob", &wire);
    BOOST_CHECK_MESSAGE(wire.find("future2") == std::string::npos, "the listing names an unreadable suite");
}

///
/// @brief Verifies that --ch_suites lists the requester's own handles, with the suites the requester can read.
///
BOOST_AUTO_TEST_CASE(test_listing_of_handles) {
    ECF_NAME_THIS_TEST();

    MockServer server(reference(true));
    register_handle(server, "alice", {"pub", "privsuite"}, false);
    register_handle(server, "bob", {"pub", "privsuite"}, false);
    replace_privsuite(server, alice_only);

    std::string wire;
    const auto reply = request(server, std::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES), "bob", &wire);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    BOOST_CHECK_MESSAGE(wire.find("alice") == std::string::npos, "the listing names the handles of alice");
    BOOST_CHECK_MESSAGE(wire.find("privsuite") == std::string::npos, "the listing names an unreadable suite");
    BOOST_CHECK_MESSAGE(wire.find("pub") != std::string::npos, "the listing misses the readable suite");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

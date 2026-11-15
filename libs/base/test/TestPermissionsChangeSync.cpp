// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
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
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The synchronisation of clients after a change of Permissions attributes, a modify change (D3.7): clients without a
/// handle, and with a handle holding some suites or none, synchronise fully once, and then have no news.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_permissions_change_sync.log"};
};

///
/// @brief Handles a request as a user, and returns the reply as received by the client.
///
/// The client side builds nodes, which moves the change numbers of the process; they are restored afterwards, since
/// the client and the server of these tests share the process, unlike real ones.
///
STC_Cmd_ptr request(MockServer& server, const Cmd_ptr& command, const std::string& user) {
    const auto reply = TestHelper::invokeRequestAs(&server, command, user);
    ServerToClientResponse response;
    response.set_cmd(reply);
    std::string data;
    ecf::save_as_string(data, response);
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    ServerToClientResponse received;
    ecf::restore_from_string(data, received);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    return received.get_cmd();
}

///
/// @brief Synchronises the copy of a client through a handle (0 for none): fully when there is no copy, otherwise
///        incrementally, and returns whether the synchronisation was full.
///
bool synchronise(MockServer& server, const std::string& user, unsigned int handle, defs_ptr& copy) {
    const auto command =
        copy ? std::make_shared<CSyncCmd>(CSyncCmd::SYNC, handle, copy->state_change_no(), copy->modify_change_no())
             : std::make_shared<CSyncCmd>(handle);
    const auto reply = request(server, command, user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    ServerReply server_reply;
    if (copy) {
        server_reply.set_client_defs(copy);
    }
    const auto state_change_no  = Ecf::state_change_no();
    const auto modify_change_no = Ecf::modify_change_no();
    reply->handle_server_response(server_reply, command, false);
    Ecf::set_state_change_no(state_change_no);
    Ecf::set_modify_change_no(modify_change_no);
    copy = server_reply.client_defs();
    return server_reply.full_sync();
}

///
/// @brief Returns whether the news of the copy of a client, through a handle (0 for none), reports a change.
///
bool news(MockServer& server, const std::string& user, unsigned int handle, const defs_ptr& copy) {
    const auto command =
        std::make_shared<CSyncCmd>(CSyncCmd::NEWS, handle, copy->state_change_no(), copy->modify_change_no());
    const auto reply = request(server, command, user);
    ServerReply server_reply;
    reply->handle_server_response(server_reply, command, false);
    return server_reply.get_news() != ServerReply::NO_NEWS;
}

unsigned int register_handle(MockServer& server, const std::string& user, const std::vector<std::string>& suites) {
    const auto reply = request(server, std::make_shared<ClientHandleCmd>(0, suites, false), user);
    BOOST_REQUIRE_MESSAGE(reply->error().empty(), reply->error());
    return server.defs()->client_suite_mgr().clientSuites().back().handle();
}

///
/// @brief Describes a client of the tests: its handle (0 for none) and its copy of the definitions.
///
struct Client
{
    std::string name;
    unsigned int handle{0};
    defs_ptr copy;
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_PermissionsChangeSync, LogFixture)

///
/// @brief Verifies that after a change of the server attribute, or of a node attribute, the clients concerned
///        synchronise fully once, receive the new attribute, and then have no news.
///
BOOST_AUTO_TEST_CASE(test_clients_synchronise_after_a_change) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s")->add_task("t");
    defs->findSuite("s")->add_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp"));
    defs->add_suite("u")->add_task("t");
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlops,alice:rwxp,bob:r"));
    MockServer server(defs);

    std::vector<Client> clients{{"no handle", 0, nullptr},
                                {"handle on u", register_handle(server, "alice", {"u"}), nullptr},
                                {"empty handle", register_handle(server, "alice", {}), nullptr}};
    for (auto& client : clients) {
        BOOST_TEST_CONTEXT(client.name) {
            synchronise(server, "alice", client.handle, client.copy);
            BOOST_CHECK(!news(server, "alice", client.handle, client.copy));
        }
    }

    // A change of the server attribute: every client synchronises fully, once
    {
        ecf::PermissionsChange change(*server.defs());
        change.set_server(Permissions::parse_cli("admin:rwxlops,alice:rwxp,bob:r,carol:r"));
        change.commit("admin@host");
    }
    for (auto& client : clients) {
        BOOST_TEST_CONTEXT(client.name) {
            BOOST_CHECK(news(server, "alice", client.handle, client.copy));
            BOOST_CHECK(synchronise(server, "alice", client.handle, client.copy));
            BOOST_REQUIRE(client.copy->server_state().permissions());
            BOOST_CHECK_EQUAL(client.copy->server_state().permissions()->to_cli(),
                              "admin:rwxlops,alice:rwxp,bob:r,carol:r");
            BOOST_CHECK(!news(server, "alice", client.handle, client.copy));
            BOOST_CHECK(!synchronise(server, "alice", client.handle, client.copy));
        }
    }

    // A change of the attribute of /s: the clients receiving /s synchronise fully, the others have no news
    {
        ecf::PermissionsChange change(*server.defs());
        change.set(*server.defs()->findAbsNode("/s"), Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r"));
        change.commit("admin@host");
    }
    for (auto& client : clients) {
        BOOST_TEST_CONTEXT(client.name) {
            const bool receives_s = client.handle == 0;
            BOOST_CHECK_EQUAL(news(server, "alice", client.handle, client.copy), receives_s);
            BOOST_CHECK_EQUAL(synchronise(server, "alice", client.handle, client.copy), receives_s);
            if (receives_s) {
                BOOST_CHECK_EQUAL(client.copy->findAbsNode("/s")->permissions()->to_cli(),
                                  "admin:rwxlop,alice:rwxp,bob:r");
            }
            BOOST_CHECK(!news(server, "alice", client.handle, client.copy));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

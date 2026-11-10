// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::TestLog;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_OwnershipGate)

namespace {

/// Definitions held by the server: one suite, without any Owner
defs_ptr make_server_defs() {
    defs_ptr defs = Defs::create();
    suite_ptr s   = defs->add_suite("s");
    s->add_family("f")->add_task("t");
    return defs;
}

/// Definitions sent by a client: one suite, whose family has an Owner
defs_ptr make_client_defs(const std::string& suite_name) {
    defs_ptr defs = Defs::create();
    suite_ptr s   = defs->add_suite(suite_name);
    family_ptr f  = s->add_family("f");
    f->add_task("t");
    f->addOwner(OwnerAttr::create("alice"));
    return defs;
}

/// Checks that handling the command is refused with the gate message; the server turns the exception into an
/// error reply to the client
void check_rejected(const ClientToServerCmd& cmd, MockServer& server) {
    BOOST_CHECK_EXCEPTION(cmd.handleRequest(&server), std::runtime_error, [](const std::runtime_error& e) {
        return std::string(e.what()).find("Owner attributes are not yet supported") != std::string::npos;
    });
}

} // namespace

BOOST_AUTO_TEST_CASE(test_load_with_owner_is_rejected) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_load_with_owner_is_rejected.log");

    defs_ptr server_defs = make_server_defs();
    MockServer server(server_defs);
    // Only the structure is compared: logging the command may set flags on the definitions
    const std::string expected = ecf::as_string(*server_defs, PrintStyle::DEFS);

    for (bool force : {false, true}) {
        LoadDefsCmd cmd(make_client_defs("other"), force);
        cmd.setup_user_authentification();
        check_rejected(cmd, server);
        BOOST_CHECK_EQUAL(ecf::as_string(*server_defs, PrintStyle::DEFS), expected);
    }
}

BOOST_AUTO_TEST_CASE(test_replace_with_owner_is_rejected) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_replace_with_owner_is_rejected.log");

    defs_ptr server_defs = make_server_defs();
    MockServer server(server_defs);
    // Only the structure is compared: logging the command may set flags on the definitions
    const std::string expected = ecf::as_string(*server_defs, PrintStyle::DEFS);

    ReplaceNodeCmd cmd("/s/f", false, make_client_defs("s"), true);
    cmd.setup_user_authentification();
    check_rejected(cmd, server);
    BOOST_CHECK_EQUAL(ecf::as_string(*server_defs, PrintStyle::DEFS), expected);
}

BOOST_AUTO_TEST_CASE(test_move_with_owner_is_rejected) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_move_with_owner_is_rejected.log");

    defs_ptr server_defs = make_server_defs();
    MockServer server(server_defs);
    // Only the structure is compared: logging the command may set flags on the definitions
    const std::string expected = ecf::as_string(*server_defs, PrintStyle::DEFS);

    defs_ptr source = make_client_defs("other");
    MoveCmd cmd(
        std::make_pair(std::string("localhost"), std::string("3141")), source->findAbsNode("/other/f").get(), "/s");
    cmd.setup_user_authentification();
    check_rejected(cmd, server);
    BOOST_CHECK_EQUAL(ecf::as_string(*server_defs, PrintStyle::DEFS), expected);
}

BOOST_AUTO_TEST_CASE(test_load_without_owner_is_accepted) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_load_without_owner_is_accepted.log");

    defs_ptr server_defs = make_server_defs();
    MockServer server(server_defs);

    defs_ptr client_defs = Defs::create();
    client_defs->add_suite("other")->add_task("t");
    LoadDefsCmd cmd(client_defs, false);
    cmd.setup_user_authentification();
    STC_Cmd_ptr reply = cmd.handleRequest(&server);
    BOOST_CHECK_MESSAGE(reply->ok(), "Unexpected error: " << reply->error());
    BOOST_CHECK(server_defs->findSuite("other"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

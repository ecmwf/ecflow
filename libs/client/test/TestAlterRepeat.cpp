// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_AlterRepeat)

namespace {

///
/// @brief Runs a command line against the server, as ecflow_client does.
///
int alter(ClientInvoker& client, const std::string& value, const std::string& path) {
    std::vector<std::string> args = {"ClientInvoker", "--alter=change", "repeat", value, path};
    return client.invoke(args);
}

const Repeat& repeat_of(ClientInvoker& client, const std::string& path) {
    BOOST_REQUIRE_MESSAGE(client.sync_local() == 0, "sync failed\n" << client.errorMsg());
    auto node = client.defs()->findAbsNode(path);
    BOOST_REQUIRE(node);
    return node->repeat();
}

} // namespace

BOOST_AUTO_TEST_CASE(help_examples_of_change_repeat) {
    ECF_NAME_THIS_TEST();

    InvokeServer server("Client:: ...help_examples_of_change_repeat:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(server.server_started(),
                          "Server failed to start on " << server.host() << ":" << server.port());

    ClientInvoker client(server.host(), server.port());
    auto defs = Defs::create();
    defs->add_suite("s")->add_family("f")->addRepeat(RepeatDate("YMD", 20250101, 20251231, 1));
    BOOST_REQUIRE_MESSAGE(client.load(defs) == 0, "load failed\n" << client.errorMsg());

    // The examples of --help alter, in an order valid for the same Repeat
    BOOST_REQUIRE_MESSAGE(alter(client, "--begin 20260101 --end 20261231", "/s/f") == 0, client.errorMsg());
    {
        const auto& rep = repeat_of(client, "/s/f");
        BOOST_CHECK_EQUAL(rep.start(), 20260101);
        BOOST_CHECK_EQUAL(rep.end(), 20261231);
        BOOST_CHECK_EQUAL(rep.value(), 20260101); // clamped into the new range
    }

    BOOST_REQUIRE_MESSAGE(alter(client, "20260115", "/s/f") == 0, client.errorMsg());
    BOOST_CHECK_EQUAL(repeat_of(client, "/s/f").value(), 20260115);

    BOOST_REQUIRE_MESSAGE(alter(client, "--begin 20260101 --end 20261231 --current 20260115", "/s/f") == 0,
                          client.errorMsg());
    BOOST_CHECK_EQUAL(repeat_of(client, "/s/f").value(), 20260115);

    BOOST_REQUIRE_MESSAGE(alter(client, "--step 7", "/s/f") == 0, client.errorMsg());
    {
        const auto& rep = repeat_of(client, "/s/f");
        BOOST_CHECK_EQUAL(rep.step(), 7);
        BOOST_CHECK_EQUAL(rep.value(), 20260115); // 14 days after the begin, on the new grid
    }

    // A refused change leaves the Repeat unchanged
    client.set_throw_on_error(false);
    BOOST_CHECK(alter(client, "--step 0", "/s/f") != 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("the delta cannot be zero") != std::string::npos, client.errorMsg());
    client.set_throw_on_error(true);
    BOOST_CHECK_EQUAL(repeat_of(client, "/s/f").step(), 7);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

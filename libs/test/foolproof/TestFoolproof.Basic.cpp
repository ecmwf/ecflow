/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "scaffold/Naming.hpp"
#include "scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_Basic)

BOOST_AUTO_TEST_CASE(test_e2e_ping) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandPing{});
        BOOST_CHECK(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --ping:\n" << c.stdout_buffer);
    }

    // Notice the automatic cleaned up when exiting scope!
    // . shutdown server
    // . remove port lock file
    // . remove working directory
}

BOOST_AUTO_TEST_CASE(test_e2e_load) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(
                        R"--(
suite s
  family f
    task task
  endfamily
endsuite;
)--")
                    .create();

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();
        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("family f"));
        BOOST_CHECK(c.stdout_contains("task task"));

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();
        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("family f"));
        BOOST_CHECK(c.stdout_contains("task task"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

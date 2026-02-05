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

BOOST_AUTO_TEST_SUITE(T_Mirror)

BOOST_AUTO_TEST_CASE(test_e2e_disallows_loading_mirror_to_own_server) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies a suite with mirrors pointing to own server is marked as invalid and cannot be loaded.
     *
     * Requirements
     *
     * - ecFlow disallows loading suite with mirror attribute pointing to own server.
     *
     */

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

    std::ostringstream suite;
    suite << "suite mirror" << "\n";
    suite << "  edit ECF_MIRROR_REMOTE_HOST " << host.value() << "\n"; // Mirror points to own host
    suite << "  edit ECF_MIRROR_REMOTE_PORT " << port.value() << "\n"; // Mirror points to own port
    suite << "  edit ECF_MIRROR_REMOTE_POLLING '30'\n";
    suite << "  edit ECF_MIRROR_REMOTE_AUTH ''\n";
    suite << "  family f\n";
    suite << "    task t\n";
    suite << "      mirror --name mirror --remote_path /x/y/z\n";
    suite << "  endfamily\n";
    suite << "endsuite\n";

    auto defs = MakeTestFile{}.with(SpecificFileLocation{"suite.mirror.def", cwd}).with(suite.str()).create();

    { // load the full suite definition
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(!client.ok());

        auto expect = [](std::string_view s) -> bool {
            std::regex re{"Server reply: Mirror '[^']*' on node '[^']*' mirrors own server '[^']*'"};
            return std::regex_search(s.data(), re);
        };
        BOOST_CHECK(expect(client.reason()));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientTimeout)

// ************************************************************************************
// Note : If you make edits to node tree, they will have no effect until the server is rebuilt /
// ************************************************************************************

// This test will check the timeoout feature of the Client
//
// The timeout feature allows the client to fail gracefully in the case where the server has died/crashed.
// After timeout, the socket is closed allowing the server to be restarted without getting the `address in use` error.
//
BOOST_AUTO_TEST_CASE(test_client_timeout, *boost::unit_test::disabled()) {

    //
    // Important: This test is disabled because ClientInvoker doesn't actually allow to set an overall timeout
    //            Until this functionality is available, this test cannot be effectively implemented
    //

    // The following removes check pt and backup file before server start,
    // to avoid the server from loading previous test data
    InvokeServer invokeServer("Client:: ...test_client_timeout", SCPort::next());

    // The timeout is configured to vary according the the client request
    // however we can override for testing.
    // Here we set a timeout for 1 second, then attempt to load a very large definition into the server.
    // Note: the timeout of 1 second means we have 1 second for each communication, hence:
    //    connect        : 1 second
    //    send request   : 1 second
    //    receive reply  : 1 second
    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_connect_timeout(0);

    std::string path = File::test_data("ANode/test/parser/data/single_defs/mega.def", "parser");
    BOOST_REQUIRE_THROW(theClient.loadDefs(path),
                        std::runtime_error); // Expect load defs to fail with a timeout of 1 second

    /// Now see what timeout value we succeed with
    bool loaded_defs = false;
    for (int i = 2; i < 30; ++i) {
        theClient.set_connect_timeout(i);
        try {
            cout << "Trying with timeout of " << i << " seconds\n";
            theClient.loadDefs(path);
            cout << "   loaded mega defs with a timeout of " << i << " seconds\n";
            loaded_defs = true;
            break;
        }
        catch (...) {
        }
    }
    BOOST_REQUIRE_MESSAGE(loaded_defs, "Expected load of defs to succeed\n");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

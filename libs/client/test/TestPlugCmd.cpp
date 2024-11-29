/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/PlugCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

static void get_defs(Defs& defs) {
    // suite suite1
    //   family family1
    //       task a
    //         event 1 myEvent
    //         meter myMeter 0 100
    //       task b
    //          trigger a == complete
    //    endfamily
    //    family family2
    //          task aa
    //             trigger ../family1/a:myMeter >= 20 and ../family1/a:myEvent
    //          task bb
    //             trigger ../family1/a:myMeter >= 50 || ../family1/a:myEvent
    //     endfamily
    // endsuite
    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");
    std::string errorMsg, warningMsg;
    bool parse = defs.restore(path, errorMsg, warningMsg);
    if (!parse)
        std::cerr << errorMsg;
    BOOST_CHECK(parse);
}

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PlugCmd)

BOOST_AUTO_TEST_CASE(test_plug_cmd) {
    ECF_NAME_THIS_TEST();

    TestLog test_log("test_plug_cmd.log"); // will create log file, and destroy log and remove file at end of scope

    {
        Defs defs;
        get_defs(defs);

        /// Test failure modes, MockServer defaults to localhost:3141
        // test source node that does not exist, fails
        TestHelper::invokeFailureRequest(&defs, Cmd_ptr(new PlugCmd("I/dont/exist/on/the server", "suite1/family2")));

        // test dest node that does not exist, fails
        TestHelper::invokeFailureRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1/a", "i/dont/exist/on/server")));

        // test source node same as dest node, fails
        TestHelper::invokeFailureRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family1/a")));

        // test dest node that matches local server host and port, but where path does not exist, fails
        TestHelper::invokeFailureRequest(
            &defs, Cmd_ptr(new PlugCmd("suite1/family1/a", "//localhost:3141/i/dont/exist/on/local_server")));
    }
    {
        Defs defs;
        get_defs(defs);

        // Lock server as another user. Invoke a valid request that should fail, due to a lock
        MockServer server(&defs);
        BOOST_REQUIRE_MESSAGE(server.lock("A user"), "Lock expected to succeed");
        TestHelper::invokeFailureRequest(server, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));
    }
    {
        Defs defs;
        get_defs(defs);
        MockServer server(&defs);

        // Move the TASKS: on family1 --> family2
        // Note: if on the destination node we select a task 'suite1/family2/aa', then node is moved to its parent
        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));
        TestHelper::invokeRequest(&defs,
                                  Cmd_ptr(new PlugCmd("suite1/family1/b", "//localhost:3141/suite1/family2/aa")));

        node_ptr node = defs.findAbsNode("suite1/family1");
        BOOST_REQUIRE_MESSAGE(node.get() && node->isFamily(), "Could not find suite1/family1");
        BOOST_REQUIRE_MESSAGE(node->isFamily()->taskVec().size() == 0, "Failed to move task to other family");

        node_ptr node2 = defs.findAbsNode("suite1/family2");
        BOOST_REQUIRE_MESSAGE(node2.get() && node->isFamily(), "Could not find suite1/family2");
        BOOST_REQUIRE_MESSAGE(node2->isFamily()->taskVec().size() == 4, "family2 two should have 4 tasks");
    }

    {
        Defs defs;
        get_defs(defs);
        MockServer server(&defs);

        // Move FAMILIES: Add a new suite and move family1 and  family2 to it.
        defs.addSuite(Suite::create("suite2"));

        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1", "suite2")));
        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family2", "suite2")));

        node_ptr suite1 = defs.findAbsNode("suite1");
        BOOST_REQUIRE_MESSAGE(suite1.get() && suite1->isSuite(), "Could not find suite1");
        BOOST_REQUIRE_MESSAGE(suite1->isSuite()->familyVec().size() == 0,
                              "Expected suite1 to have '0' families as they should have been moved to suite2");

        node_ptr suite2 = defs.findAbsNode("suite2");
        BOOST_REQUIRE_MESSAGE(suite2.get() && suite2->isSuite(), "Could not find suite2");
        BOOST_REQUIRE_MESSAGE(suite2->isSuite()->familyVec().size() == 2, "Expected suite2 to have '2' families");
    }
}

BOOST_AUTO_TEST_CASE(test_plug_cmd_preserves_server_state) {
    ECF_NAME_THIS_TEST();

    TestLog test_log("test_plug_cmd_preserves_server_state.log"); // will create log file, and destroy log and remove
                                                                  // file at end of scope

    {
        Defs defs;
        get_defs(defs);
        MockServer server(&defs);
        server.shutdown();

        // Move the TASKS: on family1 --> family2
        TestHelper::invokeRequest(&server, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));

        BOOST_CHECK_MESSAGE(server.state() == SState::SHUTDOWN,
                            "Expected server state SHUTDOWN to be preserved after plug, but found state "
                                << SState::to_string(server.state()));
    }
    {
        Defs defs;
        get_defs(defs);
        MockServer server(&defs);
        server.halted();

        // Move the TASKS: on family1 --> family2
        TestHelper::invokeRequest(&server, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));

        BOOST_CHECK_MESSAGE(server.state() == SState::HALTED,
                            "Expected server state HALTED to be preserved after plug, but found state "
                                << SState::to_string(server.state()));
    }
    {
        Defs defs;
        get_defs(defs);
        MockServer server(&defs);
        server.restart();

        // Move the TASKS: on family1 --> family2
        TestHelper::invokeRequest(&server, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));

        BOOST_CHECK_MESSAGE(server.state() == SState::RUNNING,
                            "Expected server state RUNNING to be preserved after plug, but found state "
                                << SState::to_string(server.state()));
    }
}

BOOST_AUTO_TEST_CASE(test_plug_cmd_with_handles) {
    ECF_NAME_THIS_TEST();

    TestLog test_log(
        "test_plug_cmd_with_handles.log"); // will create log file, and destroy log and remove file at end of scope

    Defs defs;
    get_defs(defs);

    /// create client handle which references suites suite in the server defs
    std::vector<std::string> suite_names;
    suite_names.emplace_back("suite");
    suite_names.emplace_back("suite2");
    TestHelper::invokeRequest(
        &defs, Cmd_ptr(new ClientHandleCmd(0, suite_names, false)), false /* bypass_state_modify_change_check */);

    { // Move the TASKS: on family1 --> family2
        // Note: if on the destination node we select a task 'suite1/family2/aa', then node is moved to its parent
        Suite* suite                  = defs.findAbsNode("suite1")->isSuite();
        unsigned int state_change_no  = suite->state_change_no();
        unsigned int modify_change_no = suite->modify_change_no();

        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1/a", "suite1/family2")));
        TestHelper::invokeRequest(&defs,
                                  Cmd_ptr(new PlugCmd("suite1/family1/b", "//localhost:3141/suite1/family2/aa")));

        BOOST_CHECK_MESSAGE(state_change_no != suite->state_change_no() ||
                                modify_change_no != suite->modify_change_no(),
                            "state and modify change numbers unaltered by plug command when using handles");
    }

    { // Move FAMILIES: Add a new suite and move family1 and  family2 to it.
        suite_ptr suite2              = defs.add_suite("suite2");
        unsigned int state_change_no  = suite2->state_change_no();
        unsigned int modify_change_no = suite2->modify_change_no();
        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family1", "suite2")));
        TestHelper::invokeRequest(&defs, Cmd_ptr(new PlugCmd("suite1/family2", "suite2")));
        BOOST_CHECK_MESSAGE(state_change_no != suite2->state_change_no() ||
                                modify_change_no != suite2->modify_change_no(),
                            "state and modify change numbers unaltered by plug command when using handles");
    }
}

static void test_plug_on_multiple_server(const std::string& host1,
                                         const std::string& port1,
                                         const std::string& host2,
                                         const std::string& port2) {
    std::cout << " on host1(" << host1 << ":" << port1 << ") host2(" << host2 << ":" << port2 << ")" << endl;
    ClientInvoker server1Client(host1, port1);
    server1Client.set_throw_on_error(false);
    ClientInvoker server2Client(host2, port2);
    server2Client.set_throw_on_error(false);

    // std::cout << " restartServer the FIRST and SECOND servers" << endl;
    BOOST_REQUIRE_MESSAGE(server1Client.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0 server not started, or connection refused\n"
                                                  << server1Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0 server not started, or connection refused\n"
                                                  << server2Client.errorMsg());

    // std::cout << " LOAD the defs into FIRST server(" << host1 << ":" << port1 << ") There is NO DEFS in the second
    // server." << endl;
    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");
    BOOST_REQUIRE_MESSAGE(server1Client.loadDefs(path) == 0, "load defs failed \n" << server1Client.errorMsg());

    // cout << " Test the ERROR conditions in MoveCmd" << endl;
    std::string sourcePath = "/suite1";
    std::string secondServerHostPort =
        "//localhost:" + port2; // The destination path must encode the host:port path to the second server

    std::string destPath = secondServerHostPort + "/A/made/up/dest/path/that/does/not/exist/on/server2";
    // cout << " Plug/Move from server1(" << host1 << ":" << port1 << ") to destination server " << destPath << endl;
    int theResult = server1Client.plug(sourcePath, destPath);
    BOOST_REQUIRE_MESSAGE(theResult == 1, CtsApi::plugArg() << "Expected to fail since no defs in server 2\n");
    // 	std::cout << "Error message = " << server1Client.errorMsg() << "\n";

    // cout << " *** Load Defs into SECOND server ***, ie both servers have the same definitions" << endl;
    BOOST_REQUIRE_MESSAGE(server2Client.loadDefs(path) == 0, "load defs failed \n" << server2Client.errorMsg());

    destPath  = secondServerHostPort + "/suite1";
    theResult = server1Client.plug(sourcePath, destPath);
    //  cout << "server1Client.errorMsg() = " << server1Client.errorMsg() << "\n";
    BOOST_REQUIRE_MESSAGE(theResult == 1,
                          CtsApi::plugArg() << " Expected to fail, since 'suite1' already exists in the server  \n");

    destPath  = secondServerHostPort + "/suite1/family1";
    theResult = server1Client.plug(sourcePath, destPath);
    //  cout << "server1Client.errorMsg() = " << server1Client.errorMsg() << "\n";
    BOOST_REQUIRE_MESSAGE(theResult == 1,
                          CtsApi::plugArg() << " Expected to fail, since can't move a suite into a family\n");

    destPath  = secondServerHostPort + "/suite1";
    theResult = server1Client.plug("/suite1/family1", destPath);
    //  cout << "server1Client.errorMsg() = " << server1Client.errorMsg() << "\n";
    BOOST_REQUIRE_MESSAGE(theResult == 1,
                          CtsApi::plugArg() << " Expected to fail, Destination already has a family1 \n");

    destPath  = secondServerHostPort + "/A/made/up/dest/path/that/does/not/exist/on/server2";
    theResult = server1Client.plug(sourcePath, destPath);
    //  	cout << "server1Client.errorMsg() = " << server1Client.errorMsg() << "\n";
    BOOST_REQUIRE_MESSAGE(theResult == 1,
                          CtsApi::plugArg() << " Expected to fail, since destination path does not exist\n");

    destPath  = secondServerHostPort;
    theResult = server1Client.plug("/suite1/family1", destPath);
    //  cout << "server1Client.errorMsg() = " << server1Client.errorMsg() << "\n";
    BOOST_REQUIRE_MESSAGE(
        theResult == 1,
        CtsApi::plugArg()
            << " Expected to fail,since source path must be a suite, if the destination path name is empty\n");

    // ==========================================================================================
    // Test Plug command works
    // ==========================================================================================

    // Completely remove the 'suite1' file in the second server
    BOOST_REQUIRE_MESSAGE(server2Client.delete_node(sourcePath) == 0,
                          CtsApi::to_string(CtsApi::delete_node(sourcePath)) << " failed \n"
                                                                             << server2Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << server2Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.defs().get(), "Server returned a NULL defs");
    BOOST_REQUIRE_MESSAGE(server2Client.defs()->suiteVec().size() == 0, " Expected server2 to have zero suite");

    // Move the suite FROM the FIRST server TO the SECOND server and check that it worked
    destPath  = secondServerHostPort;
    theResult = server1Client.plug(sourcePath, destPath);
    BOOST_REQUIRE_MESSAGE(theResult == 0, CtsApi::plugArg() << " failed \n" << server1Client.errorMsg());

    BOOST_REQUIRE_MESSAGE(server1Client.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << server1Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server1Client.defs().get(), "Server returned a NULL defs");
    BOOST_REQUIRE_MESSAGE(server1Client.defs()->suiteVec().size() == 0, " Expected server1 to have no suites");

    BOOST_REQUIRE_MESSAGE(server2Client.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << server2Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.defs().get(), "Server returned a NULL defs");
    BOOST_REQUIRE_MESSAGE(server2Client.defs()->suiteVec().size() == 1, " Expected server2 to have one suite");

    // ==========================================================================
    // Do it again, but with no defs file in second server. reload defs into server1
    BOOST_REQUIRE_MESSAGE(server1Client.delete_all() == 0,
                          CtsApi::to_string(CtsApi::delete_node()) << " failed \n"
                                                                   << server1Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.delete_all() == 0,
                          CtsApi::to_string(CtsApi::delete_node()) << " failed \n"
                                                                   << server2Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server1Client.loadDefs(path) == 0, "load defs failed \n" << server1Client.errorMsg());

    destPath  = secondServerHostPort;
    theResult = server1Client.plug(sourcePath, destPath);
    BOOST_REQUIRE_MESSAGE(theResult == 0, CtsApi::plugArg() << " failed \n" << server1Client.errorMsg());

    BOOST_REQUIRE_MESSAGE(server1Client.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << server1Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server1Client.defs().get(), "Server returned a NULL defs");
    BOOST_REQUIRE_MESSAGE(server1Client.defs()->suiteVec().size() == 0, " Expected server1 to have no suites");

    BOOST_REQUIRE_MESSAGE(server2Client.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << server2Client.errorMsg());
    BOOST_REQUIRE_MESSAGE(server2Client.defs().get(), "Server returned a NULL defs");
    BOOST_REQUIRE_MESSAGE(server2Client.defs()->suiteVec().size() == 1, " Expected server2 to have one suite");
}

BOOST_AUTO_TEST_CASE(test_server_plug_cmd) {
    ECF_NAME_THIS_TEST();

    if (ClientEnvironment::hostSpecified().empty()) {

        // Invoke two servers. *which* will both terminate at the end of this scope
        // This will remove check pt and backup file before server start, to avoid the server from loading previous test
        // data
        InvokeServer invokeServer1("", SCPort::next());
        InvokeServer invokeServer2("", SCPort::next());
        BOOST_REQUIRE_MESSAGE(invokeServer1.server_started(),
                              "Server failed to start on " << invokeServer1.host() << ":" << invokeServer1.port());
        BOOST_REQUIRE_MESSAGE(invokeServer2.server_started(),
                              "Server failed to start on " << invokeServer2.host() << ":" << invokeServer2.port());

        test_plug_on_multiple_server(
            invokeServer1.host(), invokeServer1.port(), invokeServer2.host(), invokeServer2.port());
    }
    else {

        // Remote server all ready running, start one more additional server
        {
            // remove any suites on the remote server. Since this test requires it.
            ClientInvoker theClient(ClientEnvironment::hostSpecified(), ClientEnvironment::portSpecified());
            BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                                  CtsApi::to_string(CtsApi::delete_node())
                                      << " failed should return 0. Should Delete ALL existing defs in the server\n"
                                      << theClient.errorMsg());
        }

        // Start additional local server, special constructor. need false flag, to avoid ambiguity, with the other
        // constructor.
        std::string port2 = SCPort::next_only();
        InvokeServer invokeServer2(port2, false);
        BOOST_REQUIRE_MESSAGE(invokeServer2.server_started(),
                              "Server failed to start on " << invokeServer2.host() << ":" << invokeServer2.port());

        test_plug_on_multiple_server(
            ClientEnvironment::hostSpecified(), ClientEnvironment::portSpecified(), Str::LOCALHOST(), port2);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

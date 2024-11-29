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
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_LoadDefsCmd)

//=============================================================================
// This test the LoadDefsCmd. This command will merge/absorb a defs file
// Since we are merging 2 files. It will give us an opportunity to resolve
// the extern node paths in the trigger expressions.
//
// ********************************************************************
// In the real server, we dont store, externs
// *******************************************************************
BOOST_AUTO_TEST_CASE(test_load_defs_cmd_handleRequest) {
    ECF_NAME_THIS_TEST();

    TestLog test_log("test_load_defs_cmd_handleRequest.log"); // will create log file, and destroy log and remove file
                                                              // at end of scope

    std::string firstDef = File::test_data("libs/client/test/data/first.def", "libs/client");

    // Load the FIRST file with a set of unresolved extrens
    defs_ptr firstDefs = Defs::create();
    {
        std::string errorMsg, warningMsg;
        bool parse = firstDefs->restore(firstDef, errorMsg, warningMsg);
        BOOST_CHECK_MESSAGE(parse, "Parse failed. " << errorMsg);
        firstDefs->clear_externs(); // server defs should not have externs.
    }
    size_t noOfSuites = firstDefs->suiteVec().size();

    // load the SECOND file, which should resolve the externs
    std::string secondDef = File::test_data("libs/client/test/data/second.def", "libs/client");
    Defs secondDefs;
    {
        std::string errorMsg, warningMsg;
        bool parse = secondDefs.restore(secondDef, errorMsg, warningMsg);
        BOOST_CHECK_MESSAGE(parse, "Parse failed. " << errorMsg);
    }
    noOfSuites += secondDefs.suiteVec().size();

    // Create a LoadDefsCmd. This capable of merging defs files
    // Externs are NOT loaded into the server.
    LoadDefsCmd cmd(firstDefs);
    cmd.setup_user_authentification();

    // Calling handelRequest will absorb the first defs into second & server user variables
    // AND resolve any references to node paths in the trigger expressions
    //
    // Test that the merge was OK as well
    // Note: The firstDefs is recreated in the SERVER (via LoadDefsCmd), hence out of sync with this firstDefs
    MockServer mockServer(&secondDefs);
    STC_Cmd_ptr requestStatus = cmd.handleRequest(&mockServer);
    BOOST_CHECK_MESSAGE(requestStatus, "Handle Request " << cmd << " returned NULL\n");
    BOOST_CHECK_MESSAGE(requestStatus->error().empty(), requestStatus->error());
    BOOST_CHECK_MESSAGE(secondDefs.suiteVec().size() == noOfSuites, "Merge failed to add suites");

    // Modify the Defs file to add a task/trigger that references the undefined
    // extern path defined in file 'first.def' This should fail.
    task_ptr task = Task::create("AMadeUpName");
    task->add_trigger("/a/b/c/d/e/f/g/h/j == complete");
    secondDefs.suiteVec().back()->familyVec().back()->addTask(task);

    // we just added an expression, re-parse to create AST
    // This should also attempt to resolve the extern node path /a/b/c/d/e/f/g/h/j
    // The suite 'a' should exist. But the full path is non existent
    // hence we expect the PARSE to fail.
    std::string errormsg, warningMsg;
    BOOST_CHECK_MESSAGE(!secondDefs.check(errormsg, warningMsg), errormsg);
}

BOOST_AUTO_TEST_CASE(test_load_defs_check_only) {
    ECF_NAME_THIS_TEST();

    /// Test that when check only is called the definition is NOT loaded
    InvokeServer invokeServer("Client:: ...test_load_defs_check_only", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    // Do not load the defs do a check only
    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    {
        BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path, false, true /* check only*/) == 0,
                              "Expected load to succeed\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Expected sync to succeed\n" << theClient.errorMsg());

        // Note: when running with ECF_HOST=localhost the defs may exist, but the number of suites should be empty
        BOOST_REQUIRE_MESSAGE(!theClient.defs() || theClient.defs()->suiteVec().empty(),
                              "Expected no defs, since nothing should have been loaded\n"
                                  << theClient.errorMsg());
    }
    {
        theClient.set_auto_sync(true);
        BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path, false, true /* check only*/) == 0,
                              "Expected load to succeed\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(!theClient.defs() || theClient.defs()->suiteVec().empty(),
                              "Expected no defs, since nothing should have been loaded\n"
                                  << theClient.errorMsg());
    }
    // provide path to definition that should fail to parse
    std::string path_bad_def = File::test_data("libs/client/test/data/bad.def", "libs/client");
    BOOST_REQUIRE_THROW(theClient.loadDefs(path_bad_def, false, true /* check only*/), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_load_defs) {
    ECF_NAME_THIS_TEST();

    /// Test that loading a defs a second time, with the same suite, throws a errors
    /// unless the -force option is used.
    InvokeServer invokeServer("Client:: ...test_load_defs", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    // create a defs with a single suite 's1'
    defs_ptr theDefs = Defs::create();
    {
        suite_ptr suite = Suite::create("s1");
        theDefs->addSuite(suite);
    }

    // Load the defs into the server
    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_throw_on_error(false);
    BOOST_REQUIRE_MESSAGE(theClient.load(theDefs) == 0, "Expected load to succeed\n" << theClient.errorMsg());

    // load the defs again. This should fail. as it stops accidental overwrites
    BOOST_REQUIRE_MESSAGE(theClient.load(theDefs) == 1, "Expected load to fail\n" << theClient.errorMsg());

    // Try again but with force
    BOOST_REQUIRE_MESSAGE(theClient.load(theDefs, true /*force*/) == 0,
                          "Expected load to succeed\n"
                              << theClient.errorMsg());

    /// Destroy singleton's to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

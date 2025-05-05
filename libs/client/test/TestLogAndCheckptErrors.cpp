/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_LogAndCheckptErrors)

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************
BOOST_AUTO_TEST_CASE(test_log_and_checkpt_write_errors) {
    ECF_NAME_THIS_TEST();

    // This test needs to change directory *BEFORE* before the server starts.
    // Hence if the server is already running ignore this test.
    if (!ClientEnvironment::hostSpecified().empty()) {
        cout << "Ignoring test when server is already running" << endl;
        return;
    }

    // When this test is run in Bamboo/Docker, the users is root, hence the chmod below will not work and test will fail
    if (get_login_name() == "root") {
        cout << "Ignoring test when user is root." << endl;
        return;
    }

    bool debug_me = false;
    if (debug_me)
        cout << "->Create a directory from where we will start the server\n";
    std::string ecf_home = "test_log_and_checkpt_write_errors" + Pid::getpid();
    fs::create_directory(ecf_home);

    if (debug_me)
        cout << "->change directory, before server start\n";
    BOOST_CHECK_MESSAGE(chdir(ecf_home.c_str()) == 0,
                        "Can't change directory to " << ecf_home << "  error: " << strerror(errno));
    if (debug_me)
        cout << "->current path = " << fs::current_path() << "\n";

    {
        if (debug_me)
            cout << "->start the server\n";
        InvokeServer invokeServer("Client:: ...test_log_and_checkpt_write_errors", SCPort::next());
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                              "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());
        if (debug_me)
            invokeServer.keep_log_file_after_server_exit();

        ClientInvoker theClient(invokeServer.host(), invokeServer.port());
        theClient.set_throw_on_error(false);
        BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                              CtsApi::restartServer() << " should return 0 server not started, or connection refused\n"
                                                      << theClient.errorMsg());

        if (debug_me)
            cout << "->load a defs file to the server\n";
        std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");
        BOOST_CHECK_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
        if (debug_me) {
            BOOST_CHECK_MESSAGE(theClient.sync_local() == 0, "sync_local failed \n" << theClient.errorMsg());
            PrintStyle style(PrintStyle::STATE);
            cout << theClient.defs() << "\n";
        }

        if (debug_me)
            cout << "->flush the log file, this will close the log file in the server\n";
        BOOST_CHECK_MESSAGE(theClient.flushLog() == 0, "flushLog failed \n" << theClient.errorMsg());

        if (debug_me)
            cout << "->remove write permission for " << ecf_home << "\n";
        BOOST_CHECK_MESSAGE(chdir("..") == 0, "Can't change directory to parent  error: " << strerror(errno));
        BOOST_CHECK_MESSAGE(chmod(ecf_home.c_str(), strtol("0444", 0, 8)) == 0, "Can't chmod : " << strerror(errno));

        if (debug_me)
            cout << "->write the checkpoint file, should fail, this should also try to re-open the log file and fail\n";
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 1,
                              CtsApi::checkPtDefs() << " Checkpoint expected to fail\n"
                                                    << theClient.errorMsg());

        if (debug_me)
            cout << "->get the defs from server, Check flags, check server variables\n";
        BOOST_CHECK_MESSAGE(theClient.sync_local() == 0, "sync_local failed \n" << theClient.errorMsg());
        BOOST_CHECK_MESSAGE(theClient.defs()->get_flag().is_set(Flag::CHECKPT_ERROR),
                            "Expected Flag::CHECKPT_ERROR to be set");
        BOOST_CHECK_MESSAGE(theClient.defs()->get_flag().is_set(Flag::LOG_ERROR), "Expected Flag::LOG_ERROR to be set");
        BOOST_CHECK_MESSAGE(theClient.defs()->server().variable_exists("ECF_CHECKPT_ERROR"),
                            "Expected to find ECF_CHECKPT_ERROR as a server variable");
        BOOST_CHECK_MESSAGE(theClient.defs()->server().variable_exists("ECF_LOG_ERROR"),
                            "Expected to find ECF_LOG_ERROR as a server variable");

        const Variable& var  = theClient.defs()->server().findVariable("ECF_LOG_ERROR");
        const Variable& var1 = theClient.defs()->server().findVariable("ECF_CHECKPT_ERROR");
        BOOST_CHECK_MESSAGE(!var1.theValue().empty(), "Expected ECF_CHECKPT_ERROR to have a value");
        BOOST_CHECK_MESSAGE(!var.theValue().empty(), "Expected ECF_LOG_ERROR to have a value");
        if (debug_me)
            cout << "->ECF_CHECKPT_ERROR=" << var1.theValue() << "\n";
        if (debug_me)
            cout << "->ECF_LOG_ERROR =" << var.theValue() << "\n";

        if (debug_me)
            cout << "->add write permission to " << ecf_home << "\n";
        BOOST_CHECK_MESSAGE(chmod(ecf_home.c_str(), strtol("0755", 0, 8)) == 0, "Can't chmod : " << strerror(errno));

        if (debug_me)
            cout << "->flush log file again\n";
        BOOST_CHECK_MESSAGE(theClient.flushLog() == 0, "flushLog failed \n" << theClient.errorMsg());

        if (debug_me)
            cout << "->clear the flags -> this should *ALSO* delete the server user variables\n";
        BOOST_CHECK_MESSAGE(theClient.alter("/", "clear_flag", "log_error") == 0,
                            "alter / clear_flag log_error : failed \n"
                                << theClient.errorMsg());
        BOOST_CHECK_MESSAGE(theClient.alter("/", "clear_flag", "checkpt_error") == 0,
                            "alter / clear_flag checkpt_error : failed \n"
                                << theClient.errorMsg());

        if (debug_me)
            cout << "->checkpoint again\n";
        BOOST_CHECK_MESSAGE(theClient.checkPtDefs() == 0,
                            CtsApi::checkPtDefs() << " Checkpoint expected to pass\n"
                                                  << theClient.errorMsg());

        if (debug_me)
            cout << "->Get the defs, Check flags are cleared and server variables are deleted\n";
        BOOST_CHECK_MESSAGE(theClient.sync_local() == 0, "sync_local failed \n" << theClient.errorMsg());
        BOOST_CHECK_MESSAGE(!theClient.defs()->get_flag().is_set(Flag::CHECKPT_ERROR),
                            "Expected Flag::CHECKPT_ERROR to be cleared");
        BOOST_CHECK_MESSAGE(!theClient.defs()->get_flag().is_set(Flag::LOG_ERROR),
                            "Expected Flag::LOG_ERROR to be cleared");
        BOOST_CHECK_MESSAGE(!theClient.defs()->server().variable_exists("ECF_CHECKPT_ERROR"),
                            "Expected to NOT find ECF_CHECKPT_ERROR as a server variable");
        BOOST_CHECK_MESSAGE(!theClient.defs()->server().variable_exists("ECF_LOG_ERROR"),
                            "Expected to NOT find ECF_LOG_ERROR as a server variable");
    }

    if (debug_me)
        cout << "->remove created directory " << ecf_home << "\n";
    if (debug_me)
        cout << "->current path = " << fs::current_path() << "\n";
    if (!debug_me) {
        BOOST_CHECK_MESSAGE(File::removeDir(ecf_home),
                            "Failed to remove dir " << ecf_home << "  error: " << strerror(errno));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

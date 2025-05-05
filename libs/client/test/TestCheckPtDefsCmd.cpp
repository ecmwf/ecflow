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

#include "InvokeServer.hpp"
#include "MyDefsFixture.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_CheckPtDefsCmd)

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************
BOOST_AUTO_TEST_CASE(test_check_pt_defs_cmd) {
    ECF_NAME_THIS_TEST();

    // This will remove check pt and backup file before server start, to avoid the server from loading previous test
    // data
    InvokeServer invokeServer("Client:: ...test_check_pt_defs_cmd", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0 server not started, or connection refused\n"
                                                  << theClient.errorMsg());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());

    // First time. Should create a ecf_checkpt_file, but _no_ backup file
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                          CtsApi::checkPtDefs() << " failed should return 0\n"
                                                << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_checkpt_file()),
                          CtsApi::checkPtDefs() << " failed file(" << invokeServer.ecf_checkpt_file() << ") not saved");
    BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_checkpt_file()) != 0,
                          "Expected check point file(" << invokeServer.ecf_checkpt_file()
                                                       << "), to have file size > 0");
    if (ClientEnvironment::hostSpecified().empty()) {
        // This check only valid if server was invoked locally. Ignore for remote servers
        BOOST_REQUIRE_MESSAGE(!fs::exists(invokeServer.ecf_backup_checkpt_file()),
                              "Backup check point file(" << invokeServer.ecf_backup_checkpt_file()
                                                         << ")should not exist,for very first time.");
    }

    // Save checkpoint file again. This should create a backup file, we should have ecf_checkpt_file and
    // ecf_backup_checkpt_file
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                          CtsApi::checkPtDefs() << " failed should return 0\n"
                                                << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_checkpt_file()),
                          CtsApi::checkPtDefs()
                              << " failed No check pt file(" << invokeServer.ecf_checkpt_file() << ") saved");
    BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_checkpt_file()) != 0,
                          "Expected check point file(" << invokeServer.ecf_checkpt_file()
                                                       << ") to have file size > 0  ");
    BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_backup_checkpt_file()),
                          "Backup check point file(" << invokeServer.ecf_backup_checkpt_file() << ") not created");
    BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_backup_checkpt_file()) != 0,
                          "Expected backup check point file(" << invokeServer.ecf_backup_checkpt_file()
                                                              << "), to have file size > 0");

    // Check the defaults for mode , interval and alarm time before making any changes
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected default check pt mode to be ON_TIME");
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_interval_ == CheckPt::default_interval(),
                          " Expected default check pt interval of "
                              << CheckPt::default_interval() << " but found "
                              << theClient.server_reply().stats().checkpt_interval_);
    if (ClientEnvironment::hostSpecified().empty()) {
        // This check only valid if server was invoked locally. Ignore for remote servers
        // From ecflow 4.7.0 checkpt_save_time_alarm went from 30 -> 20 seconds, hence will fail for old servers
        BOOST_REQUIRE_MESSAGE(
            theClient.server_reply().stats().checkpt_save_time_alarm_ == CheckPt::default_save_time_alarm(),
            " Expected default check pt alarm time of " << CheckPt::default_save_time_alarm() << " but found "
                                                        << theClient.server_reply().stats().checkpt_save_time_alarm_);
    }

    // Test change of check_pt interval and mode and alarm
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::NEVER) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::NEVER) << " failed should return 0\n"
                                                                   << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::NEVER,
                          " Expected check pt mode of NEVER");

    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::ON_TIME) << " failed should return 0\n"
                                                                     << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected check pt mode of ON_TIME");

    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ALWAYS) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::ALWAYS) << " failed should return 0\n"
                                                                    << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ALWAYS,
                          " Expected check pt mode of ALWAYS");

    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME, 30) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::ON_TIME, 30) << " failed should return 0\n"
                                                                         << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected check pt mode of ON_TIME");
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_interval_ == 30,
                          " Expected check pt interval of 30 but found "
                              << theClient.server_reply().stats().checkpt_interval_);

    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::UNDEFINED, 56) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::UNDEFINED, 56) << " failed should return 0\n"
                                                                           << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected check pt mode of ON_TIME");
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_interval_ == 56,
                          " Expected check pt interval of 56 but found "
                              << theClient.server_reply().stats().checkpt_interval_);

    // Mode and interval should remain unchanged only the alarm time should be changed
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::UNDEFINED, 0, 10) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::UNDEFINED, 0, 10) << " failed should return 0\n"
                                                                              << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected check pt mode of ON_TIME");
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_interval_ == 56,
                          " Expected check pt interval of 56 but found "
                              << theClient.server_reply().stats().checkpt_interval_);
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_save_time_alarm_ == 10,
                          " Expected check pt alarm time of 10 but found "
                              << theClient.server_reply().stats().checkpt_save_time_alarm_);

    // restore default check pointing for test that follow.
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME, CheckPt::default_interval()) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::ON_TIME) << " failed should return 0\n"
                                                                     << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::UNDEFINED, 0, CheckPt::default_save_time_alarm()) == 0,
                          CtsApi::checkPtDefs(ecf::CheckPt::ON_TIME) << " failed should return 0\n"
                                                                     << theClient.errorMsg());

    // check defaults were set
    BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                          CtsApi::stats_server() << " failed should return 0\n"
                                                 << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_mode_ == ecf::CheckPt::ON_TIME,
                          " Expected default check pt mode to be ON_TIME");
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().stats().checkpt_interval_ == CheckPt::default_interval(),
                          " Expected default check pt interval of "
                              << CheckPt::default_interval() << " but found "
                              << theClient.server_reply().stats().checkpt_interval_);
    BOOST_REQUIRE_MESSAGE(
        theClient.server_reply().stats().checkpt_save_time_alarm_ == CheckPt::default_save_time_alarm(),
        " Expected default check pt alarm time of " << CheckPt::default_save_time_alarm() << " but found "
                                                    << theClient.server_reply().stats().checkpt_save_time_alarm_);
}

BOOST_AUTO_TEST_CASE(test_restore_from_check_pt) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...test_restore_from_check_pt", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    defs_ptr the_defs = Defs::create();
    the_defs->add_suite("s0");
    the_defs->add_suite("s1");
    the_defs->add_suite("s2");
    the_defs->add_suite("s3");
    the_defs->add_suite("s4");

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.load(the_defs) == 0, "load defs failed \n" << theClient.errorMsg());

    size_t expected_no_of_suites = 5;
    for (int i = 0; i < 5; i++) {
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                              CtsApi::checkPtDefs() << " failed should return 0\n"
                                                    << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                              "Expected delete all nodes to succeed\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0, "Expected halt server to succeed\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.restoreDefsFromCheckPt() == 0, "Expected restoreDefsFromCheckPt succeed\n");
        BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0, "Expected getDefs() to succeed, i.e expected empty defs\n");
        BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() == expected_no_of_suites,
                              "Expected " << expected_no_of_suites << " suites, after restoreDefsFromCheckPt but found "
                                          << theClient.defs()->suiteVec().size() << "\n");

        std::string suite = "/s" + ecf::convert_to<std::string>(i);
        BOOST_REQUIRE_MESSAGE(theClient.delete_node(suite) == 0,
                              "Expected delete single suite to succeed\n"
                                  << theClient.errorMsg());
        expected_no_of_suites--;
    }
}

BOOST_AUTO_TEST_CASE(test_restore_from_check_pt_using_new_server) {
    ECF_NAME_THIS_TEST();

    // This test relies on a NEW server invocation. Hence if ECF_HOST/remote server is used
    // the test will will invalid. hence ignore.
    if (!ClientEnvironment::hostSpecified().empty()) {
        cout << "Ignoring test when ECF_HOST specified\n";
        return;
    }

    PrintStyle style(PrintStyle::STATE);
    std::string port = SCPort::next();

    MyDefsFixture theDefsFixture; // make sure generated server variable use this port.
    defs_ptr defs_to_be_check_pointed = theDefsFixture.create_defs(port);

    BOOST_REQUIRE_MESSAGE(defs_to_be_check_pointed->suiteVec().size() >= 2, "expected at least 2 suites");

    {
        // Start a new server. However make sure that on server exit, we not delete check pt files
        InvokeServer invokeServer("Client:: ...test_restore_from_check_pt_using_new_server",
                                  port,
                                  false, /* bool disable_job_generation = false */
                                  true,  /* bool remove_checkpt_file_before_server_start = true */
                                  false  /* bool remove_checkpt_file_after_server_exit = true */
        );
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                              "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

        ClientInvoker theClient(invokeServer.host(), invokeServer.port());
        BOOST_REQUIRE_MESSAGE(theClient.load(defs_to_be_check_pointed) == 0,
                              "load defs failed \n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                              CtsApi::checkPtDefs() << " failed should\n"
                                                    << theClient.errorMsg());
    }

    // start a new server, using same port. Make sure on start, we do not delete any checkpt files on start up
    // server should *LOAD* check pt file on start up
    InvokeServer invokeServer("", /* for debug use -new server- as msg */
                              port,
                              false, /* bool disable_job_generation = false */
                              false, /* bool remove_checkpt_file_before_server_start = true */
                              true   /* bool remove_checkpt_file_after_server_exit = true */
    );
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "Expected sync_local() to succeed \n");
    BOOST_REQUIRE_MESSAGE(theClient.defs(), "Expected sync_local() to succeed defs is empty\n");

    // To compare the defs, we need to massage theDefsFixture
    // update server state to match server, and update flag, caused by restoreDefsFromCheckPt
    defs_to_be_check_pointed->set_server().set_state(SState::HALTED);
    defs_to_be_check_pointed->flag().set(ecf::Flag::MESSAGE);

    // Specifically ignore server variables, as the port numbers are different( and therefore checkpt,log, etc will not
    // match)
    DebugEquality debug_equality; // only as affect in DEBUG build
    DebugEquality::set_ignore_server_variables(true);

    BOOST_CHECK_MESSAGE(*theClient.defs() == *defs_to_be_check_pointed,
                        "expected defs to be the same.\nServer defs:\n"
                            << *theClient.defs() << "\nExpected defs:\n"
                            << *defs_to_be_check_pointed);
}

BOOST_AUTO_TEST_CASE(test_check_pt_edit_history) {
    ECF_NAME_THIS_TEST();

    // This test relies on a NEW server invocation. Hence if ECF_HOST/remote server is used
    // the test will will invalid. hence ignore.
    if (!ClientEnvironment::hostSpecified().empty()) {
        cout << "Ignoring test when ECF_HOST specified\n";
        return;
    }

    // This will remove check pt and backup file before server start, to avoid the server from loading previous test
    // data
    InvokeServer invokeServer("Client:: ...test_check_pt_edit_history", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.edit_history(Str::ROOT_PATH()) == 0,
                          CtsApi::to_string(CtsApi::edit_history(Str::ROOT_PATH())) << " should return 0\n"
                                                                                    << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().get_string_vec().size() == 0,
                          "Expected edit history of size 0 after server start, but found "
                              << theClient.server_reply().get_string_vec().size());

    // make 5 edits
    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0 server not started, or connection refused\n"
                                                  << theClient.errorMsg());
    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");
    BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                          CtsApi::shutdownServer() << " should return 0\n"
                                                   << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                          CtsApi::haltServer() << " should return 0\n"
                                               << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0\n"
                                                  << theClient.errorMsg());

    // make sure edit history updated
    BOOST_REQUIRE_MESSAGE(theClient.edit_history(Str::ROOT_PATH()) == 0,
                          CtsApi::to_string(CtsApi::edit_history(Str::ROOT_PATH())) << " should return 0\n"
                                                                                    << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.server_reply().get_string_vec().size() == 5,
                          "Expected edit history of size 5, but found "
                              << theClient.server_reply().get_string_vec().size());

    // make sure edit history was *NOT* serialized, It is only serialized when check pointing
    BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->get_edit_history(Str::ROOT_PATH()).size() == 0,
                          "Expected edit history of size 0, but found "
                              << theClient.defs()->get_edit_history(Str::ROOT_PATH()).size());

    // This should write the edit history
    BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                          CtsApi::checkPtDefs() << " failed should return 0\n"
                                                << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(fs::exists(invokeServer.ecf_checkpt_file()),
                          CtsApi::checkPtDefs()
                              << " failed No check pt file(" << invokeServer.ecf_checkpt_file() << ") saved");
    BOOST_REQUIRE_MESSAGE(fs::file_size(invokeServer.ecf_checkpt_file()) != 0,
                          "Expected check point file(" << invokeServer.ecf_checkpt_file()
                                                       << ") to have file size > 0  ");

    // Create a defs file from the check pt file & check edit history
    {
        Defs defs;
        defs.restore(invokeServer.ecf_checkpt_file()); // restore defs from checkpoint
        BOOST_REQUIRE_MESSAGE(defs.get_edit_history(Str::ROOT_PATH()).size() == 5,
                              "Expected edit history of size 5, but found "
                                  << defs.get_edit_history(Str::ROOT_PATH()).size());
    }

    {
        // clear edit history
        BOOST_REQUIRE_MESSAGE(theClient.edit_history("clear") == 0,
                              CtsApi::to_string(CtsApi::edit_history("clear")) << " should return 0\n"
                                                                               << theClient.errorMsg());

        // write checkpoint file, NOTE: theClient.checkPtDefs(ecf::CheckPt::ALWAYS) *ONLY* changes the mode.
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                              CtsApi::checkPtDefs() << " failed should return 0\n"
                                                    << theClient.errorMsg());

        // restore defs from checkpoint file
        Defs defs;
        defs.restore(invokeServer.ecf_checkpt_file()); // restore defs from checkpoint
        BOOST_REQUIRE_MESSAGE(defs.get_edit_history().size() == 0,
                              "Expected edit history of size 0, but found " << defs.get_edit_history().size());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

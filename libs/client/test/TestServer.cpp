/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/date_time/posix_time/time_formatters.hpp> // requires boost date and time lib, for to_simple_string
#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/core/perf_timer.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_Server)

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// Note: To test HPUX->Linux, invoke serve on (Linux/HPUX) and the client cmds on other system
//       On the client side set ECF_HOST to machine name. To allow further testing if ECF_HOST
//       is specified then *don't* shutdown the server
// ************************************************************************************

BOOST_AUTO_TEST_CASE(test_server_version) {
    ECF_NAME_THIS_TEST();

    /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
    InvokeServer invokeServer("Client:: ...test_server_version:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.server_version() == 0, "server version\n" << theClient.errorMsg());
    if (ClientEnvironment::hostSpecified().empty()) {
        // This check only valid if server was invoked locally. Ignore for remote servers
        BOOST_REQUIRE_MESSAGE(theClient.get_string() == Version::raw(),
                              "Expected client version(" << Version::raw() << ") to match server version("
                                                         << theClient.get_string() << ")");
    }
    else {
        // remote server, version may be different
        BOOST_WARN_MESSAGE(theClient.get_string() == Version::raw(),
                           "Client version(" << Version::raw() << ") does not match server version("
                                             << theClient.get_string() << ")");
    }
}

BOOST_AUTO_TEST_CASE(test_server_state_changes) {
    ECF_NAME_THIS_TEST();

    /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
    InvokeServer invokeServer("Client:: ...test_server_state_changes:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << theClient.errorMsg());
    if (ClientEnvironment::hostSpecified().empty()) {
        // server started locally
        BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::HALTED,
                              "Expected INITIAL server state HALTED but found "
                                  << SState::to_string(theClient.defs()->server().get_state()));
    }

    BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                          CtsApi::shutdownServer() << " should return 0\n"
                                                   << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::SHUTDOWN,
                          "Expected server state SHUTDOWN but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                          CtsApi::haltServer() << " should return 0\n"
                                               << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::HALTED,
                          "Expected server state HALTED but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0\n"
                                                  << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                          CtsApi::get() << " failed should return 0\n"
                                        << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::RUNNING,
                          "Expected server state RUNNING but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    /// Repeat test using sync_local() to test incremental changes of server
    BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                          CtsApi::shutdownServer() << " should return 0\n"
                                                   << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, " failed should return 0\n" << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::SHUTDOWN,
                          "Expected server state SHUTDOWN but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                          CtsApi::haltServer() << " should return 0\n"
                                               << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, " failed should return 0\n" << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::HALTED,
                          "Expected server state HALTED but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0\n"
                                                  << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, " failed should return 0\n" << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::RUNNING,
                          "Expected server state RUNNING but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    if (ClientEnvironment::hostSpecified().empty()) {
        // This check only valid if server was invoked locally. Ignore for remote servers

        // make sure edit history updated
        BOOST_REQUIRE_MESSAGE(theClient.edit_history(Str::ROOT_PATH()) == 0,
                              CtsApi::to_string(CtsApi::edit_history(Str::ROOT_PATH())) << " should return 0\n"
                                                                                        << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.server_reply().get_string_vec().size() == 7,
                              "Expected edit history of size 7, but found "
                                  << theClient.server_reply().get_string_vec().size());

        // make sure edit history was *NOT* serialized, It is only serialized when check pointing
        BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                              CtsApi::get() << " failed should return 0\n"
                                            << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.defs()->get_edit_history(Str::ROOT_PATH()).size() == 0,
                              "Expected edit history of size 0, but found "
                                  << theClient.defs()->get_edit_history(Str::ROOT_PATH()).size());

        // clear edit history
        BOOST_REQUIRE_MESSAGE(theClient.edit_history("clear") == 0,
                              CtsApi::to_string(CtsApi::edit_history("clear")) << " should return 0\n"
                                                                               << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.edit_history(Str::ROOT_PATH()) == 0,
                              CtsApi::to_string(CtsApi::edit_history(Str::ROOT_PATH())) << " should return 0\n"
                                                                                        << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.server_reply().get_string_vec().size() == 0,
                              "Expected no edit history " << theClient.server_reply().get_string_vec().size());
    }
}

BOOST_AUTO_TEST_CASE(test_server_state_changes_with_auto_sync) {
    ECF_NAME_THIS_TEST();

    /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
    InvokeServer invokeServer("Client:: ...test_server_state_changes_with_auto_sync:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_auto_sync(true);

    BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
    if (ClientEnvironment::hostSpecified().empty()) {
        // server started locally
        BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::HALTED,
                              "Expected INITIAL server state HALTED but found "
                                  << SState::to_string(theClient.defs()->server().get_state()));
    }

    BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                          CtsApi::shutdownServer() << " should return 0\n"
                                                   << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::SHUTDOWN,
                          "Expected server state SHUTDOWN but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                          CtsApi::haltServer() << " should return 0\n"
                                               << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::HALTED,
                          "Expected server state HALTED but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                          CtsApi::restartServer() << " should return 0\n"
                                                  << theClient.errorMsg());
    BOOST_REQUIRE_MESSAGE(theClient.defs()->server().get_state() == SState::RUNNING,
                          "Expected server state RUNNING but found "
                              << SState::to_string(theClient.defs()->server().get_state()));

    if (ClientEnvironment::hostSpecified().empty()) {
        // This check only valid if server was invoked locally. Ignore for remote servers

        // make sure edit history updated
        BOOST_REQUIRE_MESSAGE(theClient.edit_history(Str::ROOT_PATH()) == 0,
                              CtsApi::to_string(CtsApi::edit_history(Str::ROOT_PATH())) << " should return 0\n"
                                                                                        << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.server_reply().get_string_vec().size() == 8 &&
                                  theClient.server_reply().get_string_vec().size() <=
                                      Defs::max_edit_history_size_per_node(),
                              "Expected edit history of size 8, but found "
                                  << theClient.server_reply().get_string_vec().size() << "\n"
                                  << Str::dump_string_vec(theClient.server_reply().get_string_vec()));

        // make sure edit history was *NOT* serialized, It is only serialized when check pointing
        BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                              CtsApi::get() << " failed should return 0\n"
                                            << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.defs()->get_edit_history(Str::ROOT_PATH()).size() == 0,
                              "Expected edit history of size 0, but found "
                                  << theClient.defs()->get_edit_history(Str::ROOT_PATH()).size());
    }
}

BOOST_AUTO_TEST_CASE(test_server_stress_test) {
    ECF_NAME_THIS_TEST();

    /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
    InvokeServer invokeServer("Client:: ...test_server_stress_test:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    int load = 125;
#ifdef ECF_OPENSSL
    if (ecf::environment::has(ecf::environment::ECF_SSL)) {
        load = 30;
    }
#endif

    {
        boost::timer::cpu_timer boost_timer;
        DurationTimer duration_timer;
        Timer<std::chrono::milliseconds> chrono_timer;
        for (int i = 0; i < load; i++) {

            BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                                  CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n"
                                                                           << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.suspend("/suite1") == 0, "should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.resume("/suite1") == 0, "should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                                  CtsApi::shutdownServer() << " should return 0\n"
                                                           << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                                  CtsApi::haltServer() << " should return 0\n"
                                                       << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                                  CtsApi::restartServer() << " should return 0\n"
                                                          << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "unknown", true) == 0,
                                  "check should return 0\n"
                                      << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());

            BOOST_REQUIRE_MESSAGE(theClient.defs().get(), "Server returned a NULL defs");
            BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() >= 1, "  no suite ?");
        }
        cout << " Server handled " << load * 16 << " requests in boost_timer("
             << boost_timer.format(3, Str::cpu_timer_format()) << ")" << " DurationTimer("
             << to_simple_string(duration_timer.elapsed()) << ")" << " Chrono_timer("
             << std::chrono::duration<double, std::milli>(chrono_timer.elapsed()).count() << " milli)" << endl;
    }
    {
        theClient.set_auto_sync(true);
        boost::timer::cpu_timer
            boost_timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
        DurationTimer duration_timer;
        Timer<std::chrono::milliseconds> chrono_timer;
        for (int i = 0; i < load; i++) {

            BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                                  CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n"
                                                                           << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.loadDefs(path) == 0, "load defs failed \n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.suspend("/suite1") == 0, "should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.resume("/suite1") == 0, "should return 0\n" << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,
                                  CtsApi::shutdownServer() << " should return 0\n"
                                                           << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,
                                  CtsApi::haltServer() << " should return 0\n"
                                                       << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,
                                  CtsApi::restartServer() << " should return 0\n"
                                                          << theClient.errorMsg());
            BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "unknown", true) == 0,
                                  "check should return 0\n"
                                      << theClient.errorMsg());

            BOOST_REQUIRE_MESSAGE(theClient.defs().get(), "Server returned a NULL defs");
            BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() >= 1, "  no suite ?");
        }
        cout << " Server handled " << load * 8 << " requests in boost_timer("
             << boost_timer.format(3, Str::cpu_timer_format()) << ")" << " DurationTimer("
             << to_simple_string(duration_timer.elapsed()) << ")" << " Chrono_timer("
             << std::chrono::duration<double, std::milli>(chrono_timer.elapsed()).count() << " milli)"
             << " *with* AUTO SYNC" << endl;
    }
}

BOOST_AUTO_TEST_CASE(test_server_group_stress_test) {
    ECF_NAME_THIS_TEST();

    /// This is exactly the same test as above, but uses the group command
    /// This should be faster as the network traffic should be a lot less
    InvokeServer invokeServer("Client:: ...test_server_group_stress_test:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    boost::timer::cpu_timer
        boost_timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
    DurationTimer duration_timer;
    ClientInvoker theClient(invokeServer.host(), invokeServer.port());

    std::string groupRequest = CtsApi::to_string(CtsApi::delete_node());
    groupRequest += ";";
    groupRequest += CtsApi::to_string(CtsApi::loadDefs(path, true /*force*/, false /*check_only*/, false /*print*/));
    groupRequest += ";";
    groupRequest += CtsApi::shutdownServer();
    groupRequest += ";";
    groupRequest += CtsApi::haltServer();
    groupRequest += ";";
    groupRequest += CtsApi::restartServer();
    groupRequest += ";";
    groupRequest += CtsApi::restartServer();
    groupRequest += ";";
    groupRequest += CtsApi::checkPtDefs();
    groupRequest += ";";
    groupRequest += CtsApi::get();

    // cout << "groupRequest = " << groupRequest << "\n";

    int load = 125;
#ifdef ECF_OPENSSL
    if (ecf::environment::has(ecf::environment::ECF_SSL)) {
        load = 30;
    }
#endif

    for (int i = 0; i < load; i++) {
        BOOST_REQUIRE_MESSAGE(theClient.group(groupRequest) == 0,
                              "Group request " << CtsApi::group(groupRequest) << " failed should return 0\n"
                                               << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.defs().get(), "Server returned a NULL defs");
        BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() >= 1, "  no suite ?");
    }
    cout << " Server handled " << load * 8 << " commands using " << load << " group requests in boost_timer("
         << boost_timer.format(3, Str::cpu_timer_format()) << ") DurationTimer("
         << to_simple_string(duration_timer.elapsed()) << ")" << endl;
}

BOOST_AUTO_TEST_CASE(test_server_stress_test_2) {
    ECF_NAME_THIS_TEST();

    /// More extensive stress test, using as many user based command as possible.
    ///
    /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
    InvokeServer invokeServer("Client:: ...test_server_stress_test_2:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    Zombie z(Child::USER,
             ecf::Child::INIT,
             ZombieAttr::get_default_attr(Child::USER),
             "path_to_task",
             "DUMMY_JOBS_PASSWORD",
             "DUMMY_PROCESS_OR_REMOTE_ID",
             1,
             "host");
    std::vector<std::string> suites;
    suites.emplace_back("suite1");
    suites.emplace_back("made_up_suite");

    std::vector<std::string> nodes_to_delete;
    nodes_to_delete.emplace_back("/suite1/family2/aa"); // these should exist in lifecycle.txt
    nodes_to_delete.emplace_back("/suite1/family2/bb");

#if defined(HPUX)
    int load = 10; // On non linux systems use different load otherwise it takes to long
#elif defined(_AIX)
    int load = 65; // On non linux systems use different load otherwise it takes to long
#else
    int load = 130;
#endif

#ifdef ECF_OPENSSL
    if (ecf::environment::has(ecf::environment::ECF_SSL)) {
        load = 10;
    }
#endif

    boost::timer::cpu_timer
        boost_timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
    DurationTimer duration_timer;
    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_throw_on_error(false);

    for (int i = 0; i < load; i++) {

        BOOST_REQUIRE_MESSAGE(theClient.pingServer() == 0, " ping should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                              CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n"
                                                                       << theClient.errorMsg());
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
        BOOST_REQUIRE_MESSAGE(theClient.news_local() == 0,
                              " new local failed should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.sync_local() == 0, "failed should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.stats() == 0, CtsApi::stats() << " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.stats_server() == 0,
                              CtsApi::stats_server() << " should return 0\n"
                                                     << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.suites() == 0,
                              CtsApi::suites() << " should return 0\n"
                                               << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.server_version() == 0,
                              CtsApi::server_version() << " should return 0\n"
                                                       << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.debug_server_off() == 0,
                              CtsApi::debug_server_off() << " should return 0\n"
                                                         << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.check("/suite1") == 0, "check should return 0\n" << theClient.errorMsg()); // 13

        BOOST_REQUIRE_MESSAGE(theClient.logMsg("start") == 0, "log msg should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.get_log_path() == 0, "get_log_path should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.getLog(1) == 0, "get_log last line should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.flushLog() == 0, "flushLog should return 0\n" << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "unknown", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "complete", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "submitted", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "active", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "aborted", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1", "queued", true) == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1/family1/a:myEvent", "set") == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.force("/suite1/family1/a:myEvent", "clear") == 0,
                              "check should return 0\n"
                                  << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.zombieGet() == 0,
                              CtsApi::zombieGet() << " should return 0\n"
                                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieFob(z) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieFail(z) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieAdopt(z) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieBlock(z) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieRemove(z) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.zombieKill(z) == 0, " should return 0\n" << theClient.errorMsg()); // 19

        BOOST_REQUIRE_MESSAGE(theClient.suspend("/suite1") == 0,
                              CtsApi::to_string(CtsApi::suspend("/suite1")) << " should return 0\n"
                                                                            << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.resume("/suite1") == 0,
                              CtsApi::to_string(CtsApi::resume("/suite1")) << " should return 0\n"
                                                                           << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.edit_history("/suite1") == 0,
                              CtsApi::to_string(CtsApi::edit_history("/suite1")) << " should return 0\n"
                                                                                 << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.replace("/suite1", path) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.replace("/suite1", path, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.replace("/suite1", path, true, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.replace("/suite1", path, false, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg()); // 26

        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs() == 0,
                              CtsApi::checkPtDefs() << " should return 0\n"
                                                    << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::NEVER) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ALWAYS) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME, 180) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::UNDEFINED) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg()); // 32
        BOOST_REQUIRE_MESSAGE(theClient.checkPtDefs(ecf::CheckPt::ON_TIME, CheckPt::default_interval()) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1") == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1", true, true, true, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1", true) == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1", false, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1", false, false, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.freeDep("/suite1", false, false, false, true) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg()); // 38

        BOOST_REQUIRE_MESSAGE(theClient.ch_register(true, suites) == 0, "--ch_register \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_suites() == 0, "--ch_suites should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_add(1, suites) == 0, "--ch_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_remove(1, suites) == 0, "--ch_remove \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_auto_add(1, true) == 0, "--ch_auto_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_auto_add(1, false) == 0, "--ch_auto_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch_drop(1) == 0, "--ch_drop should return 0\n" << theClient.errorMsg()); // 45

        BOOST_REQUIRE_MESSAGE(theClient.ch_register(true, suites) == 0, "--ch_register \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch1_add(suites) == 0, "--ch1_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch1_auto_add(true) == 0, "--ch1_auto_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch1_auto_add(false) == 0, "--ch1_auto_add \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch1_remove(suites) == 0, "--ch1_remove \n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.ch1_drop() == 0, "--ch1_drop \n" << theClient.errorMsg()); // 51

        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", "top") == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", "bottom") == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", "alpha") == 0, " should return 0\n" << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", NOrder::ORDER) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", NOrder::UP) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.order("/suite1", NOrder::DOWN) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg()); // 57

        BOOST_REQUIRE_MESSAGE(theClient.delete_node("/suite1/family1/a") == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());
        BOOST_REQUIRE_MESSAGE(theClient.delete_nodes(nodes_to_delete) == 0,
                              " should return 0\n"
                                  << theClient.errorMsg());

        BOOST_REQUIRE_MESSAGE(theClient.getDefs() == 0,
                              CtsApi::get() << " failed should return 0\n"
                                            << theClient.errorMsg()); // 60
        BOOST_REQUIRE_MESSAGE(theClient.defs().get(), "Server returned a NULL defs");
        BOOST_REQUIRE_MESSAGE(theClient.defs()->suiteVec().size() >= 1, "  no suite ?");
    }

    int no_of_client_calls = 74;

    cout << " Server handled " << load * no_of_client_calls << " requests in boost_timer("
         << boost_timer.format(3, Str::cpu_timer_format()) << ") DurationTimer("
         << to_simple_string(duration_timer.elapsed()) << ")" << endl;
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

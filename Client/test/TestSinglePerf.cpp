/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib> // getenv
#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "TestHelper.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/client/Rtt.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(ClientTestSuite)

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************

// These thresholds are based on the largest definition in the tests.
static int load_threshold_ms          = 4500;
static int begin_threshold_ms         = 550;
static double sync_full_threshold_s   = 2.6;
static double full_threshold_s        = 2.9;
static double suspend_threshold_s     = 3.5;
static double resume_threshold_s      = 3.5;
static double force_threshold_s       = 8.5;
static double check_pt_threshold_s    = 1.0;
static double client_cmds_threshold_s = 8.5;

static void sync_and_news_local(ClientInvoker& theClient) {
    {
        cout << "   news_local() : ";
        DurationTimer duration_timer;
        theClient.news_local();
        cout << duration_timer.elapsed_seconds();
    }
    {
        cout << "   sync_local() : ";
        DurationTimer duration_timer;
        theClient.sync_local();
        cout << duration_timer.elapsed_seconds() << endl;
    }
}

void time_load_and_downloads(ClientInvoker& theClient,
                             const std::string& host,
                             const std::string& port,
                             const std::string& directory) {
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(directory));

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));

    int count = 10;
    std::vector<std::string> paths;

    // std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; ++dir_itr) {
        try {
            fs::path relPath(directory + "/" + dir_itr->path().filename().string());

            // recurse down directories
            if (is_directory(dir_itr->status())) {
                time_load_and_downloads(theClient, host, port, relPath.string());
                continue;
            }

            cout << "\n" << relPath.string() << "  : file size " << fs::file_size(relPath) << endl;
            {
                DurationTimer duration_timer;
                BOOST_REQUIRE_MESSAGE(theClient.loadDefs(relPath.string()) == 0,
                                      "load defs failed \n"
                                          << theClient.errorMsg());
                cout << " Load:                " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
                BOOST_CHECK_MESSAGE(duration_timer.elapsed().total_milliseconds() < load_threshold_ms,
                                    "regression load(" << duration_timer.elapsed().total_milliseconds()
                                                       << "), exceeded threshold of " << load_threshold_ms);
            }
            {
                DurationTimer duration_timer;
                BOOST_REQUIRE_MESSAGE(theClient.begin_all_suites() == 0, "begin failed \n" << theClient.errorMsg());
                cout << " Begin:               " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
                BOOST_CHECK_MESSAGE(duration_timer.elapsed().total_milliseconds() < begin_threshold_ms,
                                    "regression begin(" << duration_timer.elapsed().total_milliseconds()
                                                        << "), exceeded threshold of " << begin_threshold_ms);
            }

            {
                cout << " Download(news_local):";
                cout.flush();
                ClientInvoker client_news(host, port);
                DurationTimer duration_timer;
                for (int i = 0; i < count; i++) {
                    if (i == 0 || i != 1) {
                        client_news.news_local();
                        switch (client_news.server_reply().get_news()) {
                            case ServerReply::DO_FULL_SYNC:
                                cout << "FULL ";
                                break;
                            case ServerReply::NEWS:
                                cout << "NEWS ";
                                break;
                            case ServerReply::NO_NEWS:
                                cout << "NO_NEWS ";
                                break;
                        }
                    }
                    else if (i == 1)
                        client_news.sync_local();
                }
                cout << ": 1:news_local(),2:sync_local(),n:news_local with the new Client: "
                     << duration_timer.elapsed().total_milliseconds() << "(ms)" << endl;
            }
            {
                cout << " Download(Sync):      ";
                cout.flush();
                for (int i = 0; i < count; i++) {
                    DurationTimer duration_timer;
                    theClient.sync_local();
                    int seconds = duration_timer.elapsed().total_milliseconds();
                    cout << seconds << " ";
                }
                cout << ":(milli-seconds) sync_local() with the same Client. First call updates cache." << endl;
            }
            {
                // On construction of Defs, hence should be slightly faster
                cout << " Download(Sync-FULL): ";
                cout.flush();
                double total = 0;
                for (int i = 0; i < count; i++) {
                    ClientInvoker client(host, port);
                    DurationTimer duration_timer;
                    client.sync_local();
                    int mil_secs = duration_timer.elapsed().total_milliseconds();
                    cout << mil_secs << " ";
                    total += mil_secs;
                }
                double average = (double)(total) / ((double)count * 1000);
                cout << ": Avg:" << average << "(sec)  : sync_local() with *different* clients.uses cache!" << endl;
                BOOST_CHECK_MESSAGE(average < sync_full_threshold_s,
                                    "regression Sync-FULL(" << average << "), exceeded threshold of "
                                                            << sync_full_threshold_s);
            }
            {
                cout << " Download(FULL):      ";
                cout.flush();
                double total = 0;
                for (int i = 0; i < count; i++) {
                    ClientInvoker client(host, port);
                    DurationTimer duration_timer;
                    theClient.getDefs();
                    int seconds = duration_timer.elapsed().total_milliseconds();
                    cout << seconds << " ";
                    total += seconds;
                }
                double average = (double)(total) / ((double)count * 1000);
                cout << ": Avg:" << average << "(sec)  : get_defs() from different client" << endl;
                BOOST_CHECK_MESSAGE(average < full_threshold_s,
                                    "regression FULL(sync)(" << average << ") exceeded threshold of "
                                                             << full_threshold_s);
            }
            {
                std::vector<task_ptr> all_tasks;
                if (!theClient.defs())
                    theClient.sync_local();
                theClient.defs()->get_all_tasks(all_tasks);
                paths.clear();
                paths.reserve(all_tasks.size());
                for (size_t i = 0; i < all_tasks.size(); i++)
                    paths.push_back(all_tasks[i]->absNodePath());

                {
                    cout << " Suspend " << paths.size() << " tasks : ";
                    cout.flush();
                    DurationTimer duration_timer;
                    theClient.suspend(paths);
                    cout << duration_timer.elapsed_seconds();
                    sync_and_news_local(theClient);
                }
                {
                    cout << " Resume " << paths.size() << " tasks  : ";
                    cout.flush();
                    DurationTimer duration_timer;
                    theClient.resume(paths);
                    cout << duration_timer.elapsed_seconds();
                    sync_and_news_local(theClient);
                }
                {
                    cout << " Suspend " << paths.size() << " tasks : ";
                    cout.flush();
                    theClient.set_auto_sync(true);
                    DurationTimer duration_timer;
                    theClient.suspend(paths);
                    cout << duration_timer.elapsed_seconds() << " : auto-sync" << endl;
                    BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < suspend_threshold_s,
                                        "regression suspend(" << duration_timer.elapsed_seconds()
                                                              << ") exceeded threshold of " << suspend_threshold_s);
                    theClient.set_auto_sync(false);
                }
                {
                    cout << " Resume " << paths.size() << " tasks  : ";
                    cout.flush();
                    theClient.set_auto_sync(true);
                    DurationTimer duration_timer;
                    theClient.resume(paths);
                    cout << duration_timer.elapsed_seconds() << " : auto-sync" << endl;
                    BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < resume_threshold_s,
                                        "regression resume(" << duration_timer.elapsed_seconds()
                                                             << ") exceeded threshold of " << resume_threshold_s);
                    theClient.set_auto_sync(false);
                }
                {
                    cout << " check  " << paths.size() << " tasks  : ";
                    cout.flush();
                    DurationTimer duration_timer;
                    theClient.check(paths);
                    cout << duration_timer.elapsed_seconds();
                    sync_and_news_local(theClient);
                }
                {
                    cout << " kill   " << paths.size() << " tasks  : ";
                    cout.flush();
                    DurationTimer duration_timer;
                    theClient.kill(paths);
                    cout << duration_timer.elapsed_seconds();
                    sync_and_news_local(theClient);
                }
                {
                    // force complete on all tasks, on some suites(3199.def), can cause infinite recursion, i.e cause
                    // repeats to loop Hence we will call force aborted instead. Also AVOID active/submitted otherwise
                    // we spend a huge time adding zombies, for the subsequent force cmd below. Node: state change
                    // memento, will also persist duration time of state change.
                    cout << " force  " << paths.size() << " tasks  : ";
                    cout.flush();
                    DurationTimer duration_timer;
                    theClient.force(paths, "aborted");
                    cout << duration_timer.elapsed_seconds() << " force(aborted)";
                    sync_and_news_local(theClient);
                }
                {
                    cout << " force  " << paths.size() << " tasks  : ";
                    cout.flush();
                    theClient.set_auto_sync(true);
                    DurationTimer duration_timer;
                    theClient.force(
                        paths, "queued"); // can't use aborted again, as it already aborted, and hence will be ignored
                    cout << duration_timer.elapsed_seconds() << " : auto-sync :  force(queued)" << endl;
                    BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < force_threshold_s,
                                        "regression force(" << duration_timer.elapsed_seconds()
                                                            << ") exceeded threshold of " << force_threshold_s);
                    theClient.set_auto_sync(false);
                }
            }
            {
                cout << " Check pt:            ";
                cout.flush();
                double total = 0;
                for (int i = 0; i < count; i++) {
                    DurationTimer duration_timer;
                    theClient.checkPtDefs();
                    int seconds = duration_timer.elapsed().total_milliseconds();
                    cout << seconds << " ";
                    total += seconds;
                }
                double average = (double)(total) / ((double)count * 1000);
                cout << ": Avg:" << average << "(s)" << endl;
                BOOST_CHECK_MESSAGE(average < check_pt_threshold_s,
                                    "regression in check_pt(" << average << ") exceeded threshold of "
                                                              << check_pt_threshold_s);
            }
            {
                cout << " client cmds:         ";
                cout.flush();
                DurationTimer duration_timer;
                size_t i;
                for (i = 0; i < paths.size() && i < 2000; i++) {
                    theClient.suspend(paths[i]);
                    theClient.sync_local();
                    theClient.resume(paths[i]);
                    theClient.sync_local();
                    theClient.force(paths[i], "aborted");
                    theClient.sync_local();
                    theClient.alter(paths[i], "add", "variable", "XXXX", "XXXX");
                    theClient.sync_local();
                    theClient.requeue(paths[i]);
                    theClient.sync_local();
                }
                cout << i << " times " << duration_timer.elapsed_seconds()
                     << "(s) (sync_local) with same client (suspend,resume,force,alter,requeue)" << endl;
                BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < client_cmds_threshold_s,
                                    "regression, exceeded threshold of " << client_cmds_threshold_s);
            }
            {
                cout << " client cmds:         ";
                cout.flush();
                theClient.set_auto_sync(true);
                DurationTimer duration_timer;
                size_t i;
                for (i = 0; i < paths.size() && i < 2000; i++) {
                    theClient.suspend(paths[i]);
                    theClient.resume(paths[i]);
                    theClient.force(paths[i], "aborted");
                    theClient.alter(paths[i], "add", "variable", "ZZZZ", "XXXX");
                    theClient.requeue(paths[i]);
                }
                cout << i << " times " << duration_timer.elapsed_seconds()
                     << "(s) (auto_sync ) with same client (suspend,resume,force,alter,requeue)" << endl;
                BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < client_cmds_threshold_s,
                                    "regression, exceeded threshold of " << client_cmds_threshold_s);
                theClient.set_auto_sync(false);
            }

            {
                DurationTimer duration_timer;
                BOOST_REQUIRE_MESSAGE(theClient.delete_all(true) == 0,
                                      "delete all defs failed \n"
                                          << theClient.errorMsg());
                cout << " Delete:              " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
            }
        }
        catch (const std::exception& ex) {
            std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_perf_for_large_defs) {
    if (const char* ecf_ssl = getenv("ECF_SSL"); ecf_ssl) {
        load_threshold_ms       = 8000; // 4500;
        begin_threshold_ms      = 800;  // 400;
        sync_full_threshold_s   = 4.5;  // 2.6;
        full_threshold_s        = 4.5;  // 2.8;
        suspend_threshold_s     = 5.8;  // 3.5;
        resume_threshold_s      = 6.5;  // 3.5;
        force_threshold_s       = 15;   // 8.5;
        check_pt_threshold_s    = 1.5;  // 1.0;
        client_cmds_threshold_s = 950;  // 8.5;
    }

    if (const char* ecf_test_defs_dir = getenv("ECF_TEST_DEFS_DIR"); !ecf_test_defs_dir) {
        std::cout << "Ignoring test! Environment variable ECF_TEST_DEFS_DIR is not defined\n";
    }
    else if (!fs::exists(ecf_test_defs_dir)) {
        std::cout << "Ignoring test! Test definitions directory " << ecf_test_defs_dir << " does not exist\n";
    }
    else {
        /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
        InvokeServer invokeServer("Client:: ...test_perf_for_large_defs:", SCPort::next());
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                              "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

        ClientInvoker theClient(invokeServer.host(), invokeServer.port());
        time_load_and_downloads(theClient, invokeServer.host(), invokeServer.port(), ecf_test_defs_dir);
    }
}

BOOST_AUTO_TEST_SUITE_END()

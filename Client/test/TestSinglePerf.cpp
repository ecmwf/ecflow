#define BOOST_TEST_MODULE TestSingle
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <fstream>
#include <cstdlib>  // getenv

#ifdef ECF_SHARED_BOOST_LIBS
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "File.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"
#include "Host.hpp"
#include "Rtt.hpp"
#include "DurationTimer.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************

// These thresholds are based on the largest definition in the tests.
static int load_threshold_ms  = 4500;
static int begin_threshold_ms = 400;
static double sync_full_threshold_s = 2.6;
static double full_threshold_s = 2.8;
static double suspend_threshold_s = 3.5;
static double resume_threshold_s = 3.5;
static double force_threshold_s = 8.5;
static double check_pt_threshold_s = 1.0;
static double client_cmds_threshold_s = 8.5;


static void sync_and_news_local(ClientInvoker& theClient)
{
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

void time_load_and_downloads(
         ClientInvoker& theClient,
         const std::string& host,
         const std::string& port,
         const std::string& directory
)
{
   fs::path full_path( fs::initial_path<fs::path>() );
   full_path = fs::system_complete( fs::path( directory ) );

   BOOST_CHECK(fs::exists( full_path ));
   BOOST_CHECK(fs::is_directory( full_path ));

   int count = 10;
   std::vector<std::string> paths;

   //std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
   fs::directory_iterator end_iter;
   for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr )
   {
      try
      {
         fs::path relPath(directory + "/" + dir_itr->path().filename().string());

         // recurse down directories
         if ( is_directory(dir_itr->status()) )  {
            time_load_and_downloads(theClient,host,port,relPath.string());
            continue;
         }

         cout << "\n" << relPath.string() << "  : file size " << fs::file_size(relPath) << endl;
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.loadDefs(relPath.string()) == 0,"load defs failed \n" << theClient.errorMsg());
            cout << " Load:                " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
            BOOST_CHECK_MESSAGE(duration_timer.elapsed().total_milliseconds() < load_threshold_ms,
            		"regression load(" << duration_timer.elapsed().total_milliseconds() << "), exceeded threshold of " << load_threshold_ms);
         }
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.begin_all_suites() == 0,"begin failed \n" << theClient.errorMsg());
            cout << " Begin:               " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
            BOOST_CHECK_MESSAGE(duration_timer.elapsed().total_milliseconds() < begin_threshold_ms,
            		"regression begin(" << duration_timer.elapsed().total_milliseconds() << "), exceeded threshold of " << begin_threshold_ms );
         }

         {
            cout << " Download(news_local):"; cout.flush();
            ClientInvoker client_news(host,port);
            DurationTimer duration_timer;
            for(int i = 0; i < count; i++) {
               if (i == 0 || i != 1) {
                  client_news.news_local();
                  switch (client_news.server_reply().get_news()) {
                     case ServerReply::DO_FULL_SYNC: cout << "FULL ";break;
                     case ServerReply::NEWS: cout << "NEWS ";break;
                     case ServerReply::NO_NEWS: cout << "NO_NEWS ";break;
                  }
               }
               else if (i == 1) client_news.sync_local();
            }
            cout << ": 1:news_local(),2:sync_local(),n:news_local with the new Client: "
                 <<  duration_timer.elapsed().total_milliseconds() << "(ms)" << endl;
         }
         {
            cout << " Download(Sync):      "; cout.flush();
            for(int i = 0; i < count; i++) {
               DurationTimer duration_timer;
               theClient.sync_local();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
            }
            cout << ":(milli-seconds) sync_local() with the same Client. First call updates cache." << endl;
         }
         {
            // On construction of Defs, hence should be slightly faster
            cout << " Download(Sync-FULL): "; cout.flush();
            double total = 0;
            for(int i = 0; i < count; i++) {
               ClientInvoker client(host,port);
               DurationTimer duration_timer;
               client.sync_local();
               int mil_secs = duration_timer.elapsed().total_milliseconds();
               cout << mil_secs  << " ";
               total += mil_secs;
            }
            double average = (double)(total)/((double)count*1000);
            cout << ": Avg:" << average << "(sec)  : sync_local() with *different* clients.uses cache!" << endl;
            BOOST_CHECK_MESSAGE(average < sync_full_threshold_s,
            		"regression Sync-FULL(" << average << "), exceeded threshold of " << sync_full_threshold_s );
         }
         {
            cout << " Download(FULL):      "; cout.flush();
            double total = 0;
            for(int i = 0; i < count; i++) {
               ClientInvoker client(host,port);
               DurationTimer duration_timer;
               theClient.getDefs();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
               total += seconds;
            }
            double average = (double)(total)/((double)count*1000);
            cout << ": Avg:" <<  average << "(sec)  : get_defs() from different client" << endl;
            BOOST_CHECK_MESSAGE(average < full_threshold_s,
            		"regression FULL(sync)(" << average << ") exceeded threshold of " << full_threshold_s );
         }
         {
            std::vector<task_ptr> all_tasks;
            if (!theClient.defs()) theClient.sync_local();
            theClient.defs()->get_all_tasks(all_tasks);
            paths.clear();
            paths.reserve(all_tasks.size());
            for(size_t i = 0; i < all_tasks.size(); i++)  paths.push_back(all_tasks[i]->absNodePath());

            {
               cout << " Suspend " << paths.size() << " tasks : "; cout.flush();
               DurationTimer duration_timer;
               theClient.suspend(paths);
               cout << duration_timer.elapsed_seconds();
               sync_and_news_local(theClient);
            }
            {
               cout << " Resume " << paths.size() << " tasks  : "; cout.flush();
               DurationTimer duration_timer;
               theClient.resume(paths);
               cout << duration_timer.elapsed_seconds();
               sync_and_news_local(theClient);
            }
            {
               cout << " Suspend " << paths.size() << " tasks : "; cout.flush();
               theClient.set_auto_sync(true);
               DurationTimer duration_timer;
               theClient.suspend(paths);
               cout << duration_timer.elapsed_seconds() << " : auto-sync" << endl;
               BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < suspend_threshold_s,
            		   "regression suspend(" << duration_timer.elapsed_seconds() << ") exceeded threshold of " << suspend_threshold_s);
               theClient.set_auto_sync(false);
            }
            {
               cout << " Resume " << paths.size() << " tasks  : "; cout.flush();
               theClient.set_auto_sync(true);
               DurationTimer duration_timer;
               theClient.resume(paths);
               cout << duration_timer.elapsed_seconds() << " : auto-sync" << endl;
               BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < resume_threshold_s,
            		   "regression resume(" << duration_timer.elapsed_seconds() << ") exceeded threshold of " << resume_threshold_s);
               theClient.set_auto_sync(false);
            }
            {
               cout << " check  " << paths.size() << " tasks  : "; cout.flush();
               DurationTimer duration_timer;
               theClient.check(paths);
               cout << duration_timer.elapsed_seconds();
               sync_and_news_local(theClient);
            }
            {
               cout << " kill   " << paths.size() << " tasks  : "; cout.flush();
               DurationTimer duration_timer;
               theClient.kill(paths);
               cout << duration_timer.elapsed_seconds();
               sync_and_news_local(theClient);
            }
            {
               // force complete on all tasks, on some suites(3199.def), can cause infinite recursion, i.e cause repeats to loop
               // Hence we will call force aborted instead.
               // Also AVOID active/submitted otherwise we spend a huge time adding zombies, for the subsequent force cmd below.
               // Node: state change memento, will also persist duration time of state change.
               cout << " force  " << paths.size() << " tasks  : "; cout.flush();
               DurationTimer duration_timer;
               theClient.force(paths,"aborted");
               cout << duration_timer.elapsed_seconds() << " force(aborted)";
               sync_and_news_local(theClient);
            }
            {
               cout << " force  " << paths.size() << " tasks  : "; cout.flush();
               theClient.set_auto_sync(true);
               DurationTimer duration_timer;
               theClient.force(paths,"queued");  // can't use aborted again, as it already aborted, and hence will be ignored
               cout << duration_timer.elapsed_seconds() << " : auto-sync :  force(queued)" << endl;
               BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < force_threshold_s,
            		   "regression force(" << duration_timer.elapsed_seconds() << ") exceeded threshold of " << force_threshold_s);
               theClient.set_auto_sync(false);
            }
         }
         {
            cout << " Check pt:            "; cout.flush();
            double total = 0;
            for(int i = 0; i < count; i++) {
               DurationTimer duration_timer;
               theClient.checkPtDefs();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
               total += seconds;
            }
            double average = (double)(total)/((double)count*1000);
            cout << ": Avg:" << average << "(s)" << endl;
            BOOST_CHECK_MESSAGE(average < check_pt_threshold_s,
            		"regression in check_pt(" << average << ") exceeded threshold of " << check_pt_threshold_s );
         }
         {
            cout << " client cmds:         " ;  cout.flush();
            DurationTimer duration_timer;
            size_t i;
            for(i = 0; i < paths.size() && i < 2000; i++) {
               theClient.suspend(paths[i]);         theClient.sync_local();
               theClient.resume(paths[i]);          theClient.sync_local();
               theClient.force(paths[i],"aborted"); theClient.sync_local();
               theClient.alter(paths[i],"add","variable","XXXX","XXXX"); theClient.sync_local();
               theClient.requeue(paths[i]);         theClient.sync_local();
            }
            cout << i << " times " << duration_timer.elapsed_seconds()  << "(s) (sync_local) with same client (suspend,resume,force,alter,requeue)" << endl;
            BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < client_cmds_threshold_s,"regression, exceeded threshold of " <<  client_cmds_threshold_s );
         }
         {
            cout << " client cmds:         " ;  cout.flush();
            theClient.set_auto_sync(true);
            DurationTimer duration_timer;
            size_t i;
            for(i = 0; i < paths.size() && i < 2000; i++) {
               theClient.suspend(paths[i]);
               theClient.resume(paths[i]);
               theClient.force(paths[i],"aborted");
               theClient.alter(paths[i],"add","variable","ZZZZ","XXXX");
               theClient.requeue(paths[i]);
            }
            cout << i << " times " << duration_timer.elapsed_seconds() << "(s) (auto_sync ) with same client (suspend,resume,force,alter,requeue)" << endl;
            BOOST_CHECK_MESSAGE(duration_timer.elapsed_seconds() < client_cmds_threshold_s,"regression, exceeded threshold of " <<  client_cmds_threshold_s );
            theClient.set_auto_sync(false);
         }

         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.delete_all(true) == 0,"delete all defs failed \n" << theClient.errorMsg());
            cout << " Delete:              " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
         }
      }
      catch ( const std::exception & ex ) {
         std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
      }
   }
}

BOOST_AUTO_TEST_CASE( test_perf_for_large_defs )
{
   if (getenv("ECF_SSL")) {
      load_threshold_ms  = 5000;     // 4500;
      begin_threshold_ms = 800;      // 400;
      sync_full_threshold_s = 3.6;   // 2.6;
      full_threshold_s = 3.8;        // 2.8;
      suspend_threshold_s = 4.5;     // 3.5;
      resume_threshold_s = 4.5;      // 3.5;
      force_threshold_s = 10.5;      // 8.5;
      check_pt_threshold_s = 1.5;    // 1.0;
      client_cmds_threshold_s = 12;  // 8.5;
   }


   if (fs::exists("/var/tmp/ma0/BIG_DEFS")) {
      /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
      InvokeServer invokeServer("Client:: ...test_perf_for_large_defs:",SCPort::next());
      BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );

      ClientInvoker theClient(invokeServer.host(), invokeServer.port());
      time_load_and_downloads(theClient,invokeServer.host(), invokeServer.port(),"/var/tmp/ma0/BIG_DEFS");
   }
   else {
      std::cout << "Ingoring test, since directory /var/tmp/ma0/BIG_DEFS does not exist";
   }
}

BOOST_AUTO_TEST_SUITE_END()

// Sept 2015, desktop

//^Ceurydice{/var/tmp/ma0/workspace/ecflow}:191 --> Client/bin/gcc-4.8/release/perf_test_large_defs
//Running 1 test case...
//Client:: ...test_perf_for_large_defs:   port(3146)
//
///var/tmp/ma0/BIG_DEFS/od.def  : file size 11081032
//Warning: TASK /lbc/perle/local/perle has a inlimit ../process:excl :The referenced FAMILY '/lbc/perle/process' does not define the limit excl
//
// Load:                1364ms
// Begin:               239ms
// Download(Sync):      791 4 0 0 0 0 0 0 0 0 :(milli-seconds) sync_local() with the same Client. First call updates cache.
// Download(Sync-FULL): 451 453 456 455 457 456 456 457 456 455 : Avg:0.4552(sec)  : sync_local() with *different* clients. Uses Cache
// Download(FULL):      802 813 835 855 880 891 900 910 923 934 : Avg:0.8743(sec)  : get_defs() from same client
// Suspend 6001 tasks : 0.027   news_local() : 0   sync_local() : 0.075
// Resume 6001 tasks  : 0.027   news_local() : 0   sync_local() : 0.053
// force  6001 tasks  : 0.073   news_local() : 0   sync_local() : 0.108
// Check pt:            334 334 339 338 338 338 339 338 338 338 : Avg:0.3374ms
// Delete:              98ms
//
///var/tmp/ma0/BIG_DEFS/vsms2.31415.def  : file size 153539843
// Load:                8720ms
// Begin:               591ms
// Download(Sync):      5323 0 0 0 0 0 0 0 0 0 :(milli-seconds) sync_local() with the same Client. First call updates cache.
// Download(Sync-FULL): 3208 3181 3181 3193 3199 3197 3200 3209 3199 3202 : Avg:3.1969(sec)  : sync_local() with *different* clients. Uses Cache
// Download(FULL):      5456 5911 5916 5995 6315 6458 6696 6658 6811 6903 : Avg:6.3119(sec)  : get_defs() from same client
// Suspend 6001 tasks : 0.045   news_local() : 0   sync_local() : 0.113
// Resume 6001 tasks  : 0.044   news_local() : 0   sync_local() : 0.081
// force  6001 tasks  : 0.076   news_local() : 0   sync_local() : 0.12
// Check pt:            2327 2342 2341 2343 2351 2340 2344 2350 2346 2376 : Avg:2.346ms
// Delete:              1486ms
//
///var/tmp/ma0/BIG_DEFS/3199.def  : file size 59631577
// Load:                5290ms
// Begin:               1577ms
// Download(Sync):      3797 23 0 0 0 0 0 0 0 0 :(milli-seconds) sync_local() with the same Client. First call updates cache.
// Download(Sync-FULL): 2157 2170 2153 2337 2446 2156 2565 2223 2165 2163 : Avg:2.2535(sec)  : sync_local() with *different* clients. Uses Cache
// Download(FULL):      3841 4143 4018 4473 4336 4406 4813 4856 5029 4508 : Avg:4.4423(sec)  : get_defs() from same client
// Suspend 6001 tasks : 0.061   news_local() : 0   sync_local() : 0.095
// Resume 6001 tasks  : 0.046   news_local() : 0   sync_local() : 0.066
// force  6001 tasks  : 0.126   news_local() : 0   sync_local() : 0.239
// Check pt:            1616 1614 1606 1603 1604 1606 1602 1602 1603 1600 : Avg:1.6056ms
// Delete:              991ms
//
///var/tmp/ma0/BIG_DEFS/mega.def  : file size 6723372
// Load:                835ms
// Begin:               117ms
// Download(Sync):      552 1 0 0 0 0 0 0 0 0 :(milli-seconds) sync_local() with the same Client. First call updates cache.
// Download(Sync-FULL): 280 281 277 283 280 284 279 283 279 283 : Avg:0.2809(sec)  : sync_local() with *different* clients. Uses Cache
// Download(FULL):      547 556 564 577 585 588 599 602 609 610 : Avg:0.5837(sec)  : get_defs() from same client
// Suspend 6001 tasks : 0.035   news_local() : 0   sync_local() : 0.085
// Resume 6001 tasks  : 0.031   news_local() : 0   sync_local() : 0.06
// force  6001 tasks  : 0.104   news_local() : 0   sync_local() : 0.553
// Check pt:            255 250 242 242 243 242 242 244 244 241 : Avg:0.2445ms
// Delete:              105ms
//
//*** No errors detected
//eurydice{/var/tmp/ma0/workspace/ecflow}:192 -->


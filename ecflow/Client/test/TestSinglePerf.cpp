#define BOOST_TEST_MODULE TestSingle
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <fstream>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/timer.hpp>

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

         cout << relPath.string() << "  : file size " << fs::file_size(relPath) << "\n";
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.loadDefs(relPath.string()) == 0,"load defs failed \n" << theClient.errorMsg());
            cout << " Load:                " << duration_timer.elapsed().seconds() << "\n";
         }
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.begin_all_suites() == 0,"begin failed \n" << theClient.errorMsg());
            cout << " Begin:               " << duration_timer.elapsed().seconds() << "\n";
         }
         {
            cout << " Download(Sync):      ";
            for(int i = 0; i < 5; i++) {
               DurationTimer duration_timer;
               theClient.sync_local();
               int seconds = duration_timer.elapsed().seconds();
               cout << seconds;
               if (i != 4) cout << ", ";
            }
            cout << "        : sync_local() with the same Client. First call updates cache.\n";
         }
         {
            // On construction of Defs, hence should be slightly faster
            cout << " Download(Sync-FULL): ";
            double time1 = 0.0, time2 = 0.0, time3 = 0.0, time4 = 0.0, time5 = 0.0;
            for(int i = 0; i < 5; i++) {
               ClientInvoker client(host,port);
               DurationTimer duration_timer;
               client.sync_local();
               int seconds = duration_timer.elapsed().seconds();
               cout << seconds;
               if (i != 4) cout << ", ";
               if (i == 0) time1 = seconds;
               if (i == 1) time2 = seconds;
               if (i == 2) time3 = seconds;
               if (i == 3) time4 = seconds;
               if (i == 4) time5 = seconds;
            }
            cout << " Avg:" << (double)( time1 + time2 + time3 +time4 +time5)/5
                 << "  : sync_local() with *different* clients. Uses Cache\n";
         }
         {
            // This should more expensive on second call, due to destruction of
            // defs(on theClient) from previous calls
            cout << " Download(FULL):      ";
            double time1 = 0.0, time2 = 0.0, time3 = 0.0, time4 = 0.0, time5 = 0.0;
            for(int i = 0; i < 5; i++) {
               DurationTimer duration_timer;
               theClient.getDefs();
               int seconds = duration_timer.elapsed().seconds();
               cout << seconds;
               if (i != 4) cout << ", ";
               if (i == 0) time1 = seconds;
               if (i == 1) time2 = seconds;
               if (i == 2) time3 = seconds;
               if (i == 3) time4 = seconds;
               if (i == 4) time5 = seconds;
            }
            cout << " Avg:" << (double)( time1 + time2 + time3 + time4 + time5 )/5
                 << "  : get_defs() from same client.\n";
         }
         {
            std::vector<task_ptr> all_tasks;
            theClient.defs()->get_all_tasks(all_tasks);
            std::vector<std::string> paths;paths.reserve(all_tasks.size());
            for(size_t i = 0; i < all_tasks.size(); i++) {
               paths.push_back(all_tasks[i]->absNodePath());
               if (i == 6000) break;  //  > 10000 really slows down, could be logging ??
            }
            {
               cout << " Suspend " << paths.size() << " tasks : ";
               DurationTimer duration_timer;
               theClient.suspend(paths);
               cout << duration_timer.elapsed().seconds();
            }
            {
               cout << "   sync_local() : ";
               DurationTimer duration_timer;
               theClient.sync_local();
               cout << duration_timer.elapsed().seconds() << "\n";
            }
            {
               cout << " Resume " << paths.size() << " tasks  : ";
               DurationTimer duration_timer;
               theClient.resume(paths);
               cout << duration_timer.elapsed().seconds();
            }
            {
               cout << "   sync_local() : ";
               DurationTimer duration_timer;
               theClient.sync_local();
               cout << duration_timer.elapsed().seconds() << "\n";
            }
         }
         {
            // This should more expensive on second call, due to destruction of
            // defs(on theClient) from previous calls
            cout << " Check pt:            ";
            double time1 = 0.0, time2 = 0.0, time3 = 0.0, time4 = 0.0, time5 = 0.0;
            for(int i = 0; i < 5; i++) {
               DurationTimer duration_timer;
               theClient.checkPtDefs();
               int seconds = duration_timer.elapsed().seconds();
               cout << seconds;
               if (i != 4) cout << ", ";
               if (i == 0) time1 = seconds;
               if (i == 1) time2 = seconds;
               if (i == 2) time3 = seconds;
               if (i == 3) time4 = seconds;
               if (i == 4) time5 = seconds;
            }
            cout << " Avg:" << (double)( time1 + time2 + time3 + time4 + time5 )/5 << "\n";
         }
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.delete_all(true)  == 0,"delete all defs failed \n" << theClient.errorMsg());
            cout << " Delete:              " << duration_timer.elapsed().seconds() << "\n";
         }
      }
      catch ( const std::exception & ex ) {
         std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
      }
   }
}

BOOST_AUTO_TEST_CASE( test_perf_for_large_defs )
{
   /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
   InvokeServer invokeServer("Client:: ...test_perf_for_large_defs:",SCPort::next());

   ClientInvoker theClient(invokeServer.host(), invokeServer.port());
   time_load_and_downloads(theClient,invokeServer.host(), invokeServer.port(),"/var/tmp/ma0/BIG_DEFS");
}

BOOST_AUTO_TEST_SUITE_END()

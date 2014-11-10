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

static void sync_and_news_local(ClientInvoker& theClient)
{
   {
      cout << "   news_local() : ";
      DurationTimer duration_timer;
      theClient.news_local();
      cout << (double)duration_timer.elapsed().total_milliseconds()/(double)1000;
   }
   {
      cout << "   sync_local() : ";
      DurationTimer duration_timer;
      theClient.sync_local();
      cout << (double)duration_timer.elapsed().total_milliseconds()/(double)1000 << endl;
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
         }
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.begin_all_suites() == 0,"begin failed \n" << theClient.errorMsg());
            cout << " Begin:               " << duration_timer.elapsed().total_milliseconds() << "ms" << endl;
         }
         {
            cout << " Download(Sync):      ";
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
            cout << " Download(Sync-FULL): ";
            double total = 0;
            for(int i = 0; i < count; i++) {
               ClientInvoker client(host,port);
               DurationTimer duration_timer;
               client.sync_local();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
               total += seconds;
            }
            cout << ": Avg:" << (double)(total)/((double)count*1000) << "(sec)  : sync_local() with *different* clients. Uses Cache" << endl;
         }
         {
            // This should more expensive on second call, due to destruction of
            // defs(on theClient) from previous calls
            cout << " Download(FULL):      ";
            double total = 0;
            for(int i = 0; i < count; i++) {
               DurationTimer duration_timer;
               theClient.getDefs();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
               total += seconds;
            }
            cout << ": Avg:" << (double)(total)/((double)count*1000) << "(sec)  : get_defs() from same client" << endl;
         }
         {
            std::vector<task_ptr> all_tasks;
            theClient.defs()->get_all_tasks(all_tasks);
            std::vector<std::string> paths;paths.reserve(all_tasks.size());
            for(size_t i = 0; i < all_tasks.size(); i++) {
               paths.push_back(all_tasks[i]->absNodePath());
               if (i == 6000) break;  //  > 9000 really slows down, could be logging ??
            }
            {
               cout << " Suspend " << paths.size() << " tasks : ";
               DurationTimer duration_timer;
               theClient.suspend(paths);
               cout << (double)duration_timer.elapsed().total_milliseconds()/(double)1000;
            }
            sync_and_news_local(theClient);
            {
               cout << " Resume " << paths.size() << " tasks  : ";
               DurationTimer duration_timer;
               theClient.resume(paths);
               cout << (double)duration_timer.elapsed().total_milliseconds()/(double)1000;
            }
            sync_and_news_local(theClient);
            {
               cout << " force  " << paths.size() << " tasks  : ";
               DurationTimer duration_timer;
               theClient.force(paths,"complete");
               cout << (double)duration_timer.elapsed().total_milliseconds()/(double)1000;
            }
            sync_and_news_local(theClient);
         }
         {
            // This should more expensive on second call, due to destruction of
            // defs(on theClient) from previous calls
            cout << " Check pt:            ";
            double total = 0;
            for(int i = 0; i < count; i++) {
               DurationTimer duration_timer;
               theClient.checkPtDefs();
               int seconds = duration_timer.elapsed().total_milliseconds();
               cout << seconds << " ";
               total += seconds;
            }
            cout << ": Avg:" << (double)(total)/((double)count*1000) << "ms" << endl;
         }
         {
            DurationTimer duration_timer;
            BOOST_REQUIRE_MESSAGE(theClient.delete_all(true)  == 0,"delete all defs failed \n" << theClient.errorMsg());
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
   if (fs::exists("/var/tmp/ma0/BIG_DEFS")) {
      /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
      InvokeServer invokeServer("Client:: ...test_perf_for_large_defs:",SCPort::next());

      ClientInvoker theClient(invokeServer.host(), invokeServer.port());
      time_load_and_downloads(theClient,invokeServer.host(), invokeServer.port(),"/var/tmp/ma0/BIG_DEFS");
   }
   else {
      std::cout << "Ingoring test, since directory /var/tmp/ma0/BIG_DEFS does not exist";
   }
}

BOOST_AUTO_TEST_SUITE_END()

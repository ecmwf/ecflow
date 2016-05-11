#define BOOST_TEST_MODULE TestMigration
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2016 ECMWF.
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

/**
* Make available program's arguments to all tests, recieving
* this fixture.
*/
struct ArgsFixture {
   ArgsFixture(): argc(boost::unit_test::framework::master_test_suite().argc),
                  argv(boost::unit_test::framework::master_test_suite().argv){}
   int argc;
   char **argv;
};

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
// ************************************************************************************

void do_test_migration(
         ClientInvoker& theClient,
         const std::string& host,
         const std::string& port,
         const std::string& directory,
         int & error_cnt
)
{
   fs::path full_path( fs::initial_path<fs::path>() );
   full_path = fs::system_complete( fs::path( directory ) );

   BOOST_CHECK(fs::exists( full_path ));
   BOOST_CHECK(fs::is_directory( full_path ));

   //std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
   fs::directory_iterator end_iter;
   for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr ) {
      try  {
         fs::path relPath(directory + "/" + dir_itr->path().filename().string());

         // recurse down directories
         if ( is_directory(dir_itr->status()) )  {
            do_test_migration(theClient,host,port,relPath.string(),error_cnt);
            continue;
         }

         cout << relPath.string() << "  : file size " << fs::file_size(relPath) << "\n\n";
         if ( fs::file_size(relPath) > 0) {
            try {
               theClient.loadDefs(relPath.string());
               theClient.delete_all(true);
            }
            catch ( std::exception& e) {
               error_cnt++;
               BOOST_CHECK_MESSAGE(false,theClient.errorMsg() << " : " << e.what());
            }
         }
      }
      catch ( const std::exception & ex ) {
         std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
      }
   }
}


BOOST_FIXTURE_TEST_CASE( test_migration,ArgsFixture )
{
   if (argc == 2 && fs::exists(argv[1])) {
      /// This will remove checkpt and backup , to avoid server from loading it. (i.e from previous test)
      InvokeServer invokeServer("Client:: ...test_migration:",SCPort::next());

      ClientInvoker theClient(invokeServer.host(), invokeServer.port());
      int error_cnt = 0;
      do_test_migration(theClient,invokeServer.host(), invokeServer.port(),argv[1],error_cnt);
      BOOST_REQUIRE_MESSAGE( error_cnt == 0, "Migration test failed " << error_cnt << " times " );
   }
   else {
      std::cout << "Ignoring test, since directory '/var/tmp/ma0/ECFLOW_TEST/migration' not found\n";
   }
}

BOOST_AUTO_TEST_SUITE_END()

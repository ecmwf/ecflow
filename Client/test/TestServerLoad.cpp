//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #29 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <string>
#include <fstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

BOOST_AUTO_TEST_CASE( test_server_load )
{
   // Check if gnuplot is found on the path, on the RPM machines gnuplot not always installed
   std::string path_to_gnuplot = File::which("gnuplot");
   if ( path_to_gnuplot.empty()) {
	   cout << "Client:: ...test_server_load -----> gnuplot not found on $PATH *IGNORING* test\n";
	   return;
   }

   // This will remove check pt and backup file before server start, to avoid the server from loading previous test data
   InvokeServer invokeServer("Client:: ...test_server_load",SCPort::next());
   BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());


   // add some text in log file, need at least 4 different time-stamps for plotting
   int load = 2;
   for(int i = 0; i < load; i++) {
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());
   }
   sleep(1); // need a time gap
   for(int i = 0; i < load; i++) {
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());
   }
   sleep(1); // need a time gap
   for(int i = 0; i < load; i++) {
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());
   }
   sleep(1); // need a time gap
   for(int i = 0; i < load; i++) {
      BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE(theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());
   }

   // Now generate gnuplot files.
   BOOST_REQUIRE_MESSAGE( theClient.server_load() == 0,CtsApi::server_load_arg() << " should return 0 server not started, or connection refused\n" << theClient.errorMsg());

   // Check files were created
   Host the_host(invokeServer.host());
   std::string gnuplot_dat_file    = the_host.prefix_host_and_port(invokeServer.port(),"gnuplot.dat");
   std::string gnuplot_script_file = the_host.prefix_host_and_port(invokeServer.port(),"gnuplot.script");
   std::string gnuplot_png_file = the_host.prefix_host_and_port(invokeServer.port(),"png");


   BOOST_REQUIRE_MESSAGE(fs::exists(gnuplot_dat_file),CtsApi::server_load_arg() << " failed file(" << gnuplot_dat_file << ") not generated");
   BOOST_REQUIRE_MESSAGE(fs::exists(gnuplot_script_file),CtsApi::server_load_arg() << " failed file(" << gnuplot_script_file << ") not generated");
   BOOST_REQUIRE_MESSAGE(fs::exists(gnuplot_png_file),CtsApi::server_load_arg() << " failed file(" << gnuplot_png_file << ") not generated");

   BOOST_REQUIRE_MESSAGE(fs::file_size(gnuplot_dat_file) !=0,"Expected file(" << gnuplot_dat_file << ") to have file size > 0  ");
   BOOST_REQUIRE_MESSAGE(fs::file_size(gnuplot_script_file) !=0,"Expected file(" << gnuplot_script_file << ") to have file size > 0  ");
   BOOST_REQUIRE_MESSAGE(fs::file_size(gnuplot_png_file) !=0,"Expected file(" << gnuplot_png_file << ") to have file size > 0  ");

   // remove generated gnuplot files.
   fs::remove( gnuplot_dat_file );
   fs::remove( gnuplot_script_file );
   fs::remove( gnuplot_png_file );
}

BOOST_AUTO_TEST_SUITE_END()


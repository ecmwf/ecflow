#define BOOST_TEST_MODULE TestSimulator
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

#include <string>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"

#include "System.hpp"
#include "Simulator.hpp"
#include "File.hpp"
#include "LogVerification.hpp"

namespace fs = boost::filesystem;

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

void simulate(const std::string& directory, bool pass)
{
	fs::path full_path( fs::initial_path<fs::path>() );
	full_path = fs::system_complete( fs::path( directory ) );

	BOOST_CHECK(fs::exists( full_path ));
	BOOST_CHECK(fs::is_directory( full_path ));

	//std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr ) {

	   try {
	      fs::path relPath(directory + "/" + dir_itr->path().filename().string());

	      // recurse down directories
	      if ( is_directory(dir_itr->status()) )  {
	         simulate(relPath.string(),pass);
	         continue;
	      }

	      // Only simulate file with .def file extension, i.e. ignore log files.
	      // cout << "path = " << relPath << "\n";
	      if (File::getExt(relPath.filename().string()) != "def" && File::getExt(relPath.filename().string()) != "got") continue;

	      //std::cout << "...............Simulating file " << relPath.string() << "\n";
	      Simulator simulator;
	      std::string errorMsg;
	      bool simPass = simulator.run(relPath.string(), errorMsg);
	      if (pass) {
	         // Test expected to pass
	         BOOST_CHECK_MESSAGE(simPass,"Simulator expected to pass for " << relPath << "\n" << errorMsg);
	      }
	      else {
	         // test expected to fail
	         BOOST_CHECK_MESSAGE(!simPass,"Simulator expected to fail for " << relPath << "\n" << errorMsg);
	      }

	      // remove generated log file. Comment out to debug
         std::string logFileName = relPath.string() + ".log";
         fs::remove(logFileName);
	   }
	   catch ( const std::exception & ex )
	   {
	      std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
	   }
	}
}

BOOST_AUTO_TEST_CASE( test_simulate_good_defs )
{
	cout << "Simulator:: ...test_simulate_good_defs\n";

   std::string path = File::test_data("CSim/test/data/good_defs","CSim");

	// All the defs in this directory are expected to pass
	simulate(path, true);
}

BOOST_AUTO_TEST_CASE( test_simulate_bad_defs )
{
	cout << "Simulator:: ...test_simulate_bad_defs\n";

   std::string path = File::test_data("CSim/test/data/bad_defs","CSim");

	// All the defs in this directory are expected to fail
	simulate(path, false);

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()


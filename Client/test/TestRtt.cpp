//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
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
#include <iostream>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/timer.hpp>

#include "Rtt.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

BOOST_AUTO_TEST_CASE( test_client_invoker_round_trip_times )
{
   cout << "Client:: ...test_client_invoker_round_trip_times" << endl;

   std::string root_path = File::test_data("Client/test/data/","Client");

   /// Open file rtt.dat and compute average round trip times
   std::string result = Rtt::analyis( root_path + "rtt.dat");
   //cout << result << "\n";

   /// generated a file with results
   std::string errorMsg;
   string generated_file = root_path + "rtt_analysis.dat";
   BOOST_CHECK_MESSAGE(File::create(generated_file, result,errorMsg),errorMsg);

   /// Compare with a reference file
   std::vector<std::string> ignoreVec; errorMsg.clear();
   std::string diffs = File::diff(generated_file, root_path + "ref_analysis.dat", ignoreVec, errorMsg );
   BOOST_CHECK_MESSAGE(diffs.empty(),diffs << "\n" <<  errorMsg);

   if (diffs.empty())  boost::filesystem::remove(generated_file);
}

BOOST_AUTO_TEST_SUITE_END()

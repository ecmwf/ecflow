/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <boost/test/unit_test.hpp>

#include "System.hpp"
#include "Signal.hpp"
#include "boost/filesystem/operations.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_system )
{
   cout << "ANode:: ...test_system \n";

   std::string file = "test_system.log";
   std::string cmd = "echo sat siri akal dunyia > " + file;

   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD,cmd,"", errorMsg) ,"System::instance()->spawn() failed: " << errorMsg);
   while (System::instance()->process() !=0 ) {
      // Capture child process termination. Child sends SIGNAl SIGCHLD, caught by parent
      Signal unblock_on_desctruction_then_reblock;
      //sleep(1); // Need to wait for child termination()
      System::instance()->processTerminatedChildren();
   }

   BOOST_CHECK_MESSAGE(fs::exists(file),"Expected cmd(" << cmd << ") to produce a file " << file);

   fs::remove(file);  // Remove the file. Comment out for debugging
}


BOOST_AUTO_TEST_SUITE_END()

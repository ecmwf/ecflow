 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
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
#include <boost/test/unit_test.hpp>
#include "ClientToServerCmd.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_log_cmd )
{
   cout << "Base:: ...test_log_cmd\n";

   {
      LogCmd log_cmd;
      BOOST_REQUIRE_MESSAGE(log_cmd.api() ==  LogCmd::GET," expected GET at the default");
      BOOST_REQUIRE_MESSAGE(log_cmd.new_path() == "","expected empty path but found " << log_cmd.new_path());
   }

   {
      LogCmd log_cmd("/a/path");
      BOOST_REQUIRE_MESSAGE(log_cmd.api() ==  LogCmd::NEW," expected GET at the default");
      BOOST_REQUIRE_MESSAGE(log_cmd.new_path() == "/a/path","expected 'a/path' but found " << log_cmd.new_path());
   }

   // ECFLOW-377
   {
      LogCmd log_cmd("/a/path ");
      BOOST_CHECK_MESSAGE(log_cmd.new_path() == "/a/path","expected '/a/path' but found '" << log_cmd.new_path() << "'");
   }
   {
      LogCmd log_cmd(" /a/path");
      BOOST_CHECK_MESSAGE(log_cmd.new_path() == "/a/path","expected '/a/path' but found '" << log_cmd.new_path() << "'");
   }
   {
      LogCmd log_cmd(" /a/path ");
      BOOST_CHECK_MESSAGE(log_cmd.new_path() == "/a/path","expected '/a/path' but found '" << log_cmd.new_path() << "'");
   }
}

BOOST_AUTO_TEST_SUITE_END()


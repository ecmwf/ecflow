 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "ClientToServerCmd.hpp"
#include "TestHelper.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "File.hpp"
#include "Str.hpp"

using namespace boost;
namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_log_cmd )
{
   cout << "Base:: ...test_log_cmd\n";

   {
      LogCmd log_cmd;
      BOOST_REQUIRE_MESSAGE(log_cmd.api() ==  LogCmd::GET," expected GET as the default");
      BOOST_REQUIRE_MESSAGE(log_cmd.new_path() == "","expected empty path but found " << log_cmd.new_path());
   }

   {
      LogCmd log_cmd("/a/path");
      BOOST_REQUIRE_MESSAGE(log_cmd.api() ==  LogCmd::NEW," expected NEW as the default api when path provided");
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

   // Create a new log file, equivalent --log=new .../Base/test/new_logfile.txt
   // LogCmd::doHandleRequest needs log file to have been created first
   std::string old_log_file = File::test_data("Base/test/old_logfile.txt","Base");
   Log::instance()->create(old_log_file);

   std::string new_log_file = File::test_data("Base/test/new_logfile.txt ","Base");          // ECFLOW-377 note extra space at the end
   std::string expected_new_log_file = File::test_data("Base/test/new_logfile.txt","Base");  // space removed

   // ECFLOW-376 --log=new <path> should be treated same as changing ECF_LOG from the gui. i.e added as a user variable. hence visible in GUI
   Defs defs;
   TestHelper::invokeRequest(&defs,Cmd_ptr( new LogCmd(new_log_file)), false /* check_change_numbers */);
   BOOST_CHECK_MESSAGE( defs.server().find_variable("ECF_LOG") == expected_new_log_file , "expected to find ECF_LOG with value '" << expected_new_log_file << "' but found '" << defs.server().find_variable("ECF_LOG") << "'");
   std::string value;
   BOOST_CHECK_MESSAGE( defs.server().find_user_variable("ECF_LOG",value)  && (value == expected_new_log_file) , "expected to find ECF_LOG in the *USER* variables '" << expected_new_log_file << "' but found '" << value << "'");
   BOOST_CHECK_MESSAGE( Log::instance()->path() == expected_new_log_file , "expected to find ECF_LOG with value '" << expected_new_log_file << "' but found '" << defs.server().find_variable("ECF_LOG") << "'");


   // Update ECF_LOG to have a *SPACE* at the end.  ECFLOW-377
   defs.set_server().add_or_update_user_variables(Str::ECF_LOG(),new_log_file);
   BOOST_CHECK_MESSAGE( defs.server().find_variable("ECF_LOG") == new_log_file , "expected to find ECF_LOG with value '" << new_log_file << "' but found '" << defs.server().find_variable("ECF_LOG") << "'");


   // INVOKE log command where we update log file from ECF_LOG, equivalent --log=new
   TestHelper::invokeRequest(&defs,Cmd_ptr( new LogCmd(LogCmd::NEW)), false /* check_change_numbers */);
   BOOST_CHECK_MESSAGE( Log::instance()->path() == expected_new_log_file , "expected to find Log::instance()->path() with value '" << expected_new_log_file << "' but found '" << defs.server().find_variable("ECF_LOG") << "'");

   // tidy up
   Log::instance()->destroy();
   fs::remove(old_log_file);  // remove generated log file
   fs::remove(expected_new_log_file);  // remove generated log file
}


BOOST_AUTO_TEST_SUITE_END()


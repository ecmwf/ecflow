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
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/tmpdir.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/foreach.hpp>
#include "boost/progress.hpp"

#include <iostream>
#include <fstream>

#include "PasswdFile.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

//#define DEBUG_ME 1

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

void test_passwd_files(const std::string& directory, bool pass)
{
   fs::path full_path( fs::initial_path<fs::path>() );
   full_path = fs::system_complete( fs::path( directory ) );

   BOOST_CHECK(fs::exists( full_path ));
   BOOST_CHECK(fs::is_directory( full_path ));

#if DEBUG_ME
   std::cout << "...In directory: " << full_path.relative_path() << "\n";
#endif

   fs::directory_iterator end_iter;
   for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr )
   {
      try
      {
         fs::path relPath(directory + "/" + dir_itr->path().filename().string());

         // recurse down directories
         if ( fs::is_directory(dir_itr->status()) )  {
            test_passwd_files(relPath.string(),pass);
            continue;
         }
#if DEBUG_ME
         std::cout << "......Parsing file " << relPath.string() << "\n";
#endif
         PasswdFile theFile;
         std::string errorMsg;
         bool parsedOk = theFile.load(relPath.string(),false/*debug*/, errorMsg);
         if (pass) {
            // Test expected to pass
            BOOST_CHECK_MESSAGE(parsedOk,"Failed to parse file " << relPath << "\n" << errorMsg << "\n" << theFile.dump());
         }
         else {
            // test expected to fail
            BOOST_CHECK_MESSAGE(!parsedOk,"Parse expected to fail for " << relPath << "\n" << errorMsg << "\n" << theFile.dump());
#if DEBUG_ME
            cout << "\n" << errorMsg << "\n";
#endif
         }
      }
      catch ( const std::exception & ex )
      {
         std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
      }
   }
}

BOOST_AUTO_TEST_CASE( test_parsing_for_good_passwd_files )
{
   cout << "ACore:: ...test_parsing_for_good_passwd_files\n";

   std::string path = File::test_data("ACore/test/data/goodPasswdFiles","ACore");

   // All the files in this directory are expected to pass
   test_passwd_files(path, true);
}

BOOST_AUTO_TEST_CASE( test_parsing_for_bad_passwd_files )
{
   cout << "ACore:: ...test_parsing_for_bad_passwd_files\n";

   std::string path = File::test_data("ACore/test/data/badPasswdFiles","ACore");

   // All the files in this directory are expected to fail
   test_passwd_files(path, false);
}

BOOST_AUTO_TEST_CASE( test_passwd_empty_file )
{
   cout << "ACore:: ...test_passwd_empty_file\n";

   std::string path = File::test_data("ACore/test/data/goodPasswdFiles/empty.passwd","ACore");

   PasswdFile theFile;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

   BOOST_REQUIRE_MESSAGE(theFile.passwds().empty(), "expected empty file " );
   BOOST_REQUIRE_MESSAGE(theFile.get_passwd("fred","host","port") == string(), "expected empty string" );
   BOOST_REQUIRE_MESSAGE(theFile.authenticate("fred",""), "expected to authenticate. TEST CASE with empty password file");
   BOOST_REQUIRE_MESSAGE(!theFile.authenticate("fred","passwd"), "expected not to authenticate" );
   BOOST_REQUIRE_MESSAGE(!theFile.authenticate("","passwd"), "expected not to authenticate" );
   BOOST_REQUIRE_MESSAGE(!theFile.authenticate("",""), "expected not to authenticate" );
}

BOOST_AUTO_TEST_CASE( test_passwd )
{
   cout << "ACore:: ...test_passwd\n";

   std::string path = File::test_data("ACore/test/data/goodPasswdFiles/ecf.passwd","ACore");

   PasswdFile theFile;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

   BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host","3141"),"expected to pass");
   BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host3","3143"),"expected to pass");
   BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host4","3145"),"expected to pass");
   BOOST_CHECK_MESSAGE(!theFile.check_at_least_one_user_with_host_and_port("xxxx","3141"),"expected fail ");
   BOOST_CHECK_MESSAGE(!theFile.check_at_least_one_user_with_host_and_port("host","13141"),"expected fail ");

   // make sure we find all the users and the access right are correct
//   4.4.0  # comment
//
//   # comment
//   fred host 3141  x12ggg # comment
//   fred host3 3143 passwd
//   fred host4 3145 x12ggg
//   # comment
//
//   jake host 3141  x12ggg
//   tom host3 3143    x12ggg # sdsdsd

   std::vector<Pass_wd> expected_passwds;
   expected_passwds.push_back(Pass_wd("fred", "host", "3141",  "x12ggg"));
   expected_passwds.push_back(Pass_wd("fred", "host3", "3143",  "passwd"));
   expected_passwds.push_back(Pass_wd("fred", "host4", "3145",  "x12ggg"));
   expected_passwds.push_back(Pass_wd("jake", "host", "3141",  "x12ggg"));
   expected_passwds.push_back(Pass_wd("tom", "host3", "3143",  "x12ggg"));

   BOOST_REQUIRE_MESSAGE(expected_passwds == theFile.passwds() ,"expected passwords to match");
}

BOOST_AUTO_TEST_SUITE_END()

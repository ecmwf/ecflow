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

#include "WhiteListFile.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

//#define DEBUG_ME 1

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

void test_white_list_files(const std::string& directory, bool pass)
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
		    if ( fs::is_directory(dir_itr->status()) )  {
		    	test_white_list_files(relPath.string(),pass);
		    	continue;
		    }

#if DEBUG_ME
			std::cout << "......Parsing file " << relPath.string() << "\n";
#endif

		   WhiteListFile theFile;
			std::string errorMsg;
			bool parsedOk = theFile.load(relPath.string(),false/*debug*/, errorMsg);
			if (pass) {
				// Test expected to pass
				BOOST_CHECK_MESSAGE(parsedOk,"Failed to parse file " << relPath << "\n" << errorMsg);
 			}
			else {
				// test expected to fail
				BOOST_CHECK_MESSAGE(!parsedOk,"Parse expected to fail for " << relPath << "\n" << errorMsg);
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

BOOST_AUTO_TEST_CASE( test_parsing_for_good_white_list_files )
{
	cout << "ACore:: ...test_parsing_for_good_white_list_files\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles","ACore");

	// All the files in this directory are expected to pass
	test_white_list_files(path, true);
}

BOOST_AUTO_TEST_CASE( test_parsing_for_bad_white_list_files )
{
	cout << "ACore:: ...test_parsing_for_bad_white_list_files\n";

   std::string path = File::test_data("ACore/test/data/badWhiteListFiles","ACore");

	// All the files in this directory are expected to fail
	test_white_list_files(path, false);
}

BOOST_AUTO_TEST_CASE( test_white_list_empty_file )
{
   cout << "ACore:: ...test_white_list_empty_file\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles/empty.lists","ACore");

   WhiteListFile theFile;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

   BOOST_REQUIRE_MESSAGE(0 == theFile.read_access_size(), "expected 0 users with read access but found " << theFile.read_access_size() );
   BOOST_REQUIRE_MESSAGE(0 == theFile.write_access_size(),"expected 0 users with write access but found " << theFile.write_access_size() );

   // test random user
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("xxxx"),"Expected user xxxx to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("xxtt"),"Expected user xxtt to have read access ");

   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("yyyy"),"Expected user yyyy to have write access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("zzzz"),"Expected user zzzz to have write access ");
}

BOOST_AUTO_TEST_CASE( test_white_list )
{
	cout << "ACore:: ...test_white_list\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles/good1.lists","ACore");

   WhiteListFile theFile;
	std::string errorMsg;
 	BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

 	// make sure we find all the users and the access right are correct
// 	uid1    # a comment
// 	uid2  # a comment
// 	cog   # a comment
//
// 	#
// 	# Read only uisers
// 	#
// 	-fred  # a comment
// 	-bill # a comment
// 	-jake # a comment
	std::vector<std::string> expected_users_with_read_access;
	expected_users_with_read_access.push_back( std::string("fred") );
	expected_users_with_read_access.push_back( std::string("bill") );
   expected_users_with_read_access.push_back( std::string("jake") );

   expected_users_with_read_access.push_back( std::string("uid1") );  // users with write access also have read access
   expected_users_with_read_access.push_back( std::string("uid2") );  // users with write access also have read access
   expected_users_with_read_access.push_back( std::string("cog") );   // users with write access also have read access

   std::vector<std::string> expected_users_with_read_write_access;
   expected_users_with_read_write_access.push_back( std::string("uid1"));
   expected_users_with_read_write_access.push_back( std::string("uid2"));
   expected_users_with_read_write_access.push_back( std::string("cog"));

 	BOOST_REQUIRE_MESSAGE(expected_users_with_read_access.size() == theFile.read_access_size(),
 	      " expected " << expected_users_with_read_access.size() << " users with read access but found " << theFile.read_access_size() );
   BOOST_REQUIRE_MESSAGE(expected_users_with_read_write_access.size() == theFile.write_access_size(),
         " expected " << expected_users_with_read_write_access.size() << " users with write access but found " << theFile.write_access_size() );


 	std::vector< std::string >::const_iterator i;
 	for(i=expected_users_with_read_access.begin(); i!= expected_users_with_read_access.end(); ++i) {
 	  BOOST_REQUIRE_MESSAGE( theFile.allow_read_access(*i),"Expected user " << *i << " to have read access ");
	}
   for(i=expected_users_with_read_write_access.begin(); i!= expected_users_with_read_write_access.end(); ++i) {
     BOOST_REQUIRE_MESSAGE( theFile.allow_write_access(*i),"Expected user " << *i << " to have write access ");
   }

   // test random user
   BOOST_REQUIRE_MESSAGE( !theFile.allow_read_access("xxxx"),"Expected user xxxx to NOT have read access ");
   BOOST_REQUIRE_MESSAGE( !theFile.allow_read_access("*"),"Expected user *  to NOT have read access ");

   BOOST_REQUIRE_MESSAGE( !theFile.allow_write_access("yyyy"),"Expected user yyyy  to NOT have write access ");
   BOOST_REQUIRE_MESSAGE( !theFile.allow_write_access("zzzz"),"Expected user zzzz  to NOT have write access ");
}

BOOST_AUTO_TEST_CASE( test_white_list_all_users_have_read_access )
{
   cout << "ACore:: ...test_white_list_all_users_have_read_access\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles/all_read_access.lists","ACore");

   WhiteListFile theFile;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

   // make sure we find all the users and the access right are correct
//# These user have read and write access to the server
//uid1  # a comment
//uid2  # a comment
//cog   # a comment
//
//
//# Read only users
//-*
//-fred  # a comment
//-bill # a comment
//-jake # a comment

   std::vector<std::string> expected_users_with_read_write_access;
   expected_users_with_read_write_access.push_back( std::string("uid1"));
   expected_users_with_read_write_access.push_back( std::string("uid2"));
   expected_users_with_read_write_access.push_back( std::string("cog"));

   // When all users have read access, the read access size should be empty
   BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0, " expected 0  but found " << theFile.read_access_size() );

   BOOST_REQUIRE_MESSAGE(expected_users_with_read_write_access.size() == theFile.write_access_size(),
         " expected " << expected_users_with_read_write_access.size() << " users with write access but found " << theFile.write_access_size() );

   // Any user should have read write access
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("fred"),"Expected user fred  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("bill"),"Expected user bill  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("xxxx"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("uid1"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("uid2"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("cog"),"Expected user xxxx  to have read access ");

   std::vector< std::string >::const_iterator i;
   for(i=expected_users_with_read_write_access.begin(); i!= expected_users_with_read_write_access.end(); ++i) {
     BOOST_REQUIRE_MESSAGE( theFile.allow_write_access(*i),"Expected user " << *i << " to have write access ");
   }

   // test random user for write access
   BOOST_REQUIRE_MESSAGE( !theFile.allow_write_access("yyyy"),"Expected user yyyy  to NOT have write access ");
   BOOST_REQUIRE_MESSAGE( !theFile.allow_write_access("zzzz"),"Expected user zzzz  to NOT have write access ");
}


BOOST_AUTO_TEST_CASE( test_white_list_all_users_have_write_access )
{
   cout << "ACore:: ...test_white_list_all_users_have_write_access\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles/all_write_access.lists","ACore");

   WhiteListFile theFile;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(theFile.load(path,false, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

   // make sure we find all the users and the access right are correct
//   uid1  # a comment
//   uid2  # a comment
//   cog   # a comment
//   *
//
//   # Read only users
//   -*
//   -fred  # a comment
//   -bill # a comment
//   -jake # a comment

   // When all users have read access, the read access size should be empty
   BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0,  " expected 0  but found " << theFile.read_access_size() );
   BOOST_REQUIRE_MESSAGE(theFile.write_access_size() == 0, " expected 0  but found " << theFile.read_access_size() );

   // Any user should have read access
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("fred"),"Expected user fred  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("bill"),"Expected user bill  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("xxxx"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("uid1"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("uid2"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_read_access("cog"),"Expected user xxxx  to have read access ");

   // Any user should have read write access
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("fred"),"Expected user fred  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("bill"),"Expected user bill  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("xxxx"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("uid1"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("uid2"),"Expected user xxxx  to have read access ");
   BOOST_REQUIRE_MESSAGE( theFile.allow_write_access("cog"),"Expected user xxxx  to have read access ");
}

BOOST_AUTO_TEST_SUITE_END()

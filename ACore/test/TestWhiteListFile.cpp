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
#if BOOST_FILESYSTEM_VERSION == 3
         fs::path relPath(directory + "/" + dir_itr->path().filename().string());
#else
 			fs::path relPath(directory + "/" + dir_itr->path().filename());
#endif

 			// recurse down directories
		    if ( fs::is_directory(dir_itr->status()) )  {
		    	test_white_list_files(relPath.string(),pass);
		    	continue;
		    }

#if DEBUG_ME
			std::cout << "......Parsing file " << relPath.string() << "\n";
#endif

		    WhiteListFile theFile(relPath.string());

		    std::map<std::string,bool> validUsers;
			std::string errorMsg;

			bool parsedOk = theFile.parse(validUsers, errorMsg);
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


BOOST_AUTO_TEST_CASE( test_white_list )
{
	cout << "ACore:: ...test_white_list\n";

   std::string path = File::test_data("ACore/test/data/goodWhiteListFiles/good1.lists","ACore");

   WhiteListFile theFile(path);
   std::map<std::string,bool> validUsers;
	std::string errorMsg;
 	BOOST_CHECK_MESSAGE(theFile.parse(validUsers, errorMsg),"Failed to parse file " << path << "\n" << errorMsg);

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
	std::vector< std::pair<std::string,bool> > expected;
	expected.push_back(std::make_pair(std::string("uid1"), true));
	expected.push_back(std::make_pair(std::string("uid2"), true));
	expected.push_back(std::make_pair(std::string("cog"),  true));
	expected.push_back(std::make_pair(std::string("fred"), false));
	expected.push_back(std::make_pair(std::string("bill"), false));
	expected.push_back(std::make_pair(std::string("jake"), false));

 	BOOST_REQUIRE_MESSAGE(expected.size() == validUsers.size(), " expected " << expected.size() << " users but found " << validUsers.size() );
 	std::vector< std::pair<std::string,bool> >::const_iterator i;
 	for(i=expected.begin(); i!= expected.end(); ++i) {
 		std::map<std::string,bool>::const_iterator it = validUsers.find( (*i).first );
	 	BOOST_REQUIRE_MESSAGE(it != validUsers.end(),"Failed to find user " << (*i).first);
	 	BOOST_REQUIRE_MESSAGE((*it).second == (*i).second ,"For user " << (*i).first << " expected " <<  (*i).second << " but found " << (*it).second);
	}
}

BOOST_AUTO_TEST_SUITE_END()

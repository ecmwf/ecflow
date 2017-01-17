#define BOOST_TEST_MODULE TestParser
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Request
// Author      : Avi
// Revision    : $Revision$ 
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
#include <string>
#include <iostream>
#include <fstream>

#include <boost/archive/tmpdir.hpp>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "PrintStyle.hpp"
#include "PersistHelper.hpp"
#include "File.hpp"
#include "Ecf.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ParserTestSuite )

void test_defs(const std::string& directory, bool pass)
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
		    	test_defs(relPath.string(),pass);
		    	continue;
		    }

			//std::cout << "......Parsing file " << relPath.string() << "\n";

			Defs defs;
			DefsStructureParser parser( &defs , relPath.string() );

 			std::string errorMsg,warningMsg;
			bool parsedOk = parser.doParse(errorMsg,warningMsg);
			if (pass) {
				// Test expected to pass
				BOOST_CHECK_MESSAGE(parsedOk,"Failed to parse file " << relPath << "\n" << errorMsg);

				if (parsedOk) {
 					// Write parsed file to a temporary file on disk, and reload, then compare defs, should be the same
 					PersistHelper helper;
					BOOST_CHECK_MESSAGE( helper.test_persist_and_reload(defs,parser.get_file_type()), relPath.string() << " " << helper.errorMsg());
					BOOST_CHECK_MESSAGE( helper.test_checkpt_and_reload(defs), relPath.string() << " " << helper.errorMsg());

					// test copy constructor
				   Ecf::set_debug_equality(true);
					Defs copy_of_defs = Defs(defs);
					BOOST_CHECK_MESSAGE(copy_of_defs == defs,"Error copy constructor failed " << relPath);
				   Ecf::set_debug_equality(false);
				}
 			}
			else {
				// test expected to fail
			   //std::cout << errorMsg << "\n";
				BOOST_CHECK_MESSAGE(!parsedOk,"Parse expected to fail for " << relPath << "\n" << errorMsg);
 			}
		}
		catch ( const std::exception & ex )
		{
 			std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
		}
	}
}

BOOST_AUTO_TEST_CASE( test_parsing_for_good_defs )
{
	cout << "AParser:: ...test_parsing_for_good_defs\n";

   std::string path = File::test_data("AParser/test/data/good_defs","AParser");

	// All the defs in this directory are expected to pass
	test_defs(path, true);
}

BOOST_AUTO_TEST_CASE( test_parsing_for_bad_defs )
{
	cout << "AParser:: ...test_parsing_for_bad_defs\n";

   std::string path = File::test_data("AParser/test/data/bad_defs","AParser");

	// All the defs in this directory are expected to fail
	test_defs(path, false);
}

BOOST_AUTO_TEST_CASE( test_parsing_for_good_defs_state )
{
   cout << "AParser:: ...test_parsing_for_good_defs_state\n";

   std::string path = File::test_data("AParser/test/data/good_defs_state","AParser");

   // All the defs in this directory are expected to pass
   test_defs(path, true);
}
BOOST_AUTO_TEST_SUITE_END()


//============================================================================
// Name        :
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

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/timer.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include <boost/test/unit_test.hpp>

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "NodeContainer.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "Log.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( ParserTestSuite )

// Test that automatic add of externs
BOOST_AUTO_TEST_CASE( test_auto_add_externs )
{
   std::string path = File::test_data("ANode/parser/test/data/single_defs/test_auto_add_extern.def","parser");

	size_t mega_file_size = fs::file_size(path);
	cout << "AParser:: ...test_auto_add_externs " << path << " file_size(" << mega_file_size << ")\n";

	Defs defs;
	DefsStructureParser checkPtParser( &defs, path);
	std::string errorMsg,warningMsg;
  	BOOST_REQUIRE_MESSAGE(checkPtParser.doParse(errorMsg,warningMsg),errorMsg);
   BOOST_REQUIRE_MESSAGE(warningMsg.empty(),"Expected no warnings but found:\n" << warningMsg);

   // Check number of extrens read in: Duplicate should be ignore
  	BOOST_REQUIRE_MESSAGE(defs.externs().size() ==  9 ,"Expected 9 externs as starting point but found " << defs.externs().size() << "\n");

  	// Test auto extern generation. Don't remove existing extern's
   defs.auto_add_externs(false/* remove_existing_externs_first*/);
  	BOOST_REQUIRE_MESSAGE(defs.externs().size() ==  9 ,"Expected 9, auto_add_extern(false) gave: " << defs.externs().size() << "\n" << defs << "\n");

  	// By removing the externs read, in we can determine the real number of extern;s from
  	// parsing all the trigger expressions, and inlimit references
   defs.auto_add_externs(true/* remove_existing_externs_first*/);
  	BOOST_REQUIRE_MESSAGE(defs.externs().size() ==  8 ,"Expected 8 externs, since redundant externs removed, auto_add_extern(true) gave: " << defs.externs().size() << "\n"<< defs << "\n");
}

BOOST_AUTO_TEST_SUITE_END()


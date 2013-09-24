 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
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
#include <string>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ParserTestSuite )

BOOST_AUTO_TEST_CASE( test_single_defs ) {

	cout << "AParser:: ...test_variable  \n";

   std::string path = File::test_data("AParser/test/data/good_defs/edit/edit.def","AParser");

	Defs defs;
	DefsStructureParser checkPtParser( &defs, path );
	std::string errorMsg,warningMsg;
  	BOOST_REQUIRE_MESSAGE(checkPtParser.doParse(errorMsg,warningMsg),errorMsg);

//  	suite edit
//  		edit ECF_INCLUDE /home/ma/map/sms/example/x                  # comment line
//  		edit ECF_FILES   /home/ma/map/sms/example/x                  #comment line
//  	    edit EXPVER 'f8na'                                          #
//  	    edit USER 'ecgems'                                          #comment
//  	    edit USER2 "ecgems"                                         # comment
//  	    edit INT1 "10"                                             # comment
//  	    edit INT2 '11'                                             # comment
//  	    edit YMD  '20091012'                                        # comment
//  	 	family family
//  			edit ECF_FETCH  "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"  # comment line
//  			edit ECF_FETCH2 'smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%'  #comment line
//  			task t2
//  		endfamily
//  	endsuite
	suite_ptr editSuite = defs.findSuite("edit");
  	BOOST_REQUIRE_MESSAGE(editSuite,"Could not find the edit suite");

  	const Variable& int1 = editSuite->findVariable("INT1");
  	BOOST_REQUIRE_MESSAGE(!int1.empty(),"Could not find variable INT1");
  	BOOST_REQUIRE_MESSAGE(int1.value() == 10,"Expected INT1 to have a value of 10, but found " << int1.value());

  	const Variable& int2 = editSuite->findVariable("INT2");
  	BOOST_REQUIRE_MESSAGE(!int2.empty(),"Could not find variable INT2");
  	BOOST_REQUIRE_MESSAGE(int2.value() == 11,"Expected INT2 to have a value of 11, but found " << int2.value());

  	const Variable& ymd = editSuite->findVariable("YMD");
  	BOOST_REQUIRE_MESSAGE(!ymd.empty(),"Could not find variable YMD");
  	BOOST_REQUIRE_MESSAGE(ymd.value() == 20091012,"Expected YMD to have a value of 20091012, but found " << ymd.value());

  	const Variable& user = editSuite->findVariable("USER");
  	BOOST_REQUIRE_MESSAGE(!user.empty(),"Could not find variable USER");
  	BOOST_REQUIRE_MESSAGE(user.value() == 0,"Expected user to have a value of 0, but found " << user.value());
}

BOOST_AUTO_TEST_SUITE_END()


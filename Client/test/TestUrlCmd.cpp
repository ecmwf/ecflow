//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include <boost/test/unit_test.hpp>

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "UrlCmd.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

//=============================================================================
// This will test the UrlCmd.
BOOST_AUTO_TEST_CASE( test_url_cmd )
{
	cout << "Client:: ...test_url_cmd" << endl;

   std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");

	defs_ptr defs = Defs::create();

 	DefsStructureParser checkPtParser( defs.get(), path );
	std::string errorMsg,warningMsg;
	bool parse = checkPtParser.doParse(errorMsg,warningMsg);
 	BOOST_CHECK_MESSAGE(parse,errorMsg);

 	// Check error conditions
 	BOOST_REQUIRE_THROW(UrlCmd(defs,"a made up name"), std::runtime_error );
 	BOOST_REQUIRE_THROW(UrlCmd(defs_ptr(),"/suite1/family1/a"), std::runtime_error );

 	// The Url command relies on variable substitution, hence we must ensure that
 	// generated variables are created.
 	defs->beginAll();

	UrlCmd urlCmd(defs,"/suite1/family1/a");
	std::string expected = "${BROWSER:=firefox} -remote 'openURL(http://www.ecmwf.int/publications/manuals/sms)'";
	std::string actual = urlCmd.getUrl();
	BOOST_CHECK_MESSAGE( expected == actual,"Expected '" << expected << "' but found " << actual);
}

BOOST_AUTO_TEST_SUITE_END()



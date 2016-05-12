/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_environment_substitution )
{
    std::cout <<  "ANode:: ...test_environment_substitution\n";

    Defs defs;
    Suite* s =  NULL;
    {
		suite_ptr suite = defs.add_suite( "suite" );  s =  suite.get();
		suite->addVariable(Variable("AVI","avi"));

  		std::vector<std::pair<std::string,std::string> > env;
  		env.push_back( std::make_pair(Str::ECF_HOME(), string("/home/smshome")) );
  		env.push_back( std::make_pair(string("FRED"),    string("/home/fred")) );
  		env.push_back( std::make_pair(string("BILL"),    string("/home/bill")) );
  		env.push_back( std::make_pair(string("JANE"),    string("/home/jane")) );
  		defs.set_server().add_or_update_user_variables( env );
	}

 	// See page 31, section 5.1 variable inheritance, of SMS users guide
	string expected = "/home/smshome";
 	std::string cmd = "$ECF_HOME";
 	BOOST_REQUIRE_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "$ECF_HOME/include"; expected = "/home/smshome/include";
 	BOOST_REQUIRE_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "$ECF_HOME$FRED$BILL$JANE"; expected = "/home/smshome/home/fred/home/bill/home/jane";
 	BOOST_REQUIRE_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");


 	cmd = "$ECF_HOME/$FRED/$BILL/$JANE";  expected = "/home/smshome//home/fred//home/bill//home/jane";
  	BOOST_CHECK_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%PATH"; expected = "%PATH";
 	BOOST_CHECK_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "$$"; expected = "$$";
 	BOOST_CHECK_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "$ERROR$"; expected = "$ERROR$";
 	BOOST_CHECK_MESSAGE(!s->variable_dollar_subsitution(cmd)," substitution expected to fail since ERROR does not exist");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = ""; expected = "";
 	BOOST_CHECK_MESSAGE(s->variable_dollar_subsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_SUITE_END()


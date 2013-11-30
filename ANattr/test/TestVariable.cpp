/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <boost/test/unit_test.hpp>

#include "Variable.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_multi_line_variable_values )
{
   cout << "ANattr:: ...test_multi_line_variable_values\n";

   {
      Variable var("name","value");
      BOOST_CHECK_MESSAGE(var.name() == "name","name not as expected");
      BOOST_CHECK_MESSAGE(var.theValue() == "value","value not as expected");

      std::string expected = "edit name 'value'";
      BOOST_CHECK_MESSAGE(var.toString() == expected,"expected " << expected << " but found " << var.toString());
   }
   {
      Variable var("name","");
      std::string expected = "edit name ''";
      BOOST_CHECK_MESSAGE(var.toString() == expected,"expected " << expected << " but found " << var.toString());
   }
   {
      Variable var("name","value\n");
      std::string expected = "edit name 'value\\n'";
      BOOST_CHECK_MESSAGE(var.toString() == expected,"expected " << expected << " but found " << var.toString());
   }
   {
      Variable var("name","val1\nxxx\nval2");
      std::string expected = "edit name 'val1\\nxxx\\nval2'";
      BOOST_CHECK_MESSAGE(var.toString() == expected,"expected " << expected << " but found " << var.toString());
   }
}

BOOST_AUTO_TEST_SUITE_END()


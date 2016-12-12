/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <stdio.h>
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

BOOST_AUTO_TEST_CASE( test_variable_value )
{
   cout << "ANattr:: ...test_variable_value\n";

   std::vector<std::string> values;
   values.push_back("sdsd");
   values.push_back("0fred0");
   values.push_back("fted");
   values.push_back("%value%");
   values.push_back("a");
   values.push_back("");
   values.push_back("0");
   values.push_back("00");
   values.push_back("000");
   values.push_back("0000");
   values.push_back("0000000000000");
   for(size_t i=0; i < values.size(); i++) {
      Variable var("name","");
      var.set_value(values[i]);
      BOOST_CHECK_MESSAGE(var.value() == 0,"expected 0 but found " << var.value() << " for " << values[i]);
   }

   {
      Variable var("name","0100");
      BOOST_CHECK_MESSAGE(var.value() == 100,"expected 100 but found " << var.value());
   }
   {
      Variable var("name","0001");
      BOOST_CHECK_MESSAGE(var.value() == 1,"expected 1 but found " << var.value());
   }
   {
      Variable var("name","2359");
      BOOST_CHECK_MESSAGE(var.value() == 2359,"expected 2359 but found " << var.value());
   }

   // make sure time is convertible to an integer
   char smstime[255];
   for(int h = 0; h < 24; h++) {
      for(int m=1; m < 60; m++ ) {
         int output_written = sprintf(smstime,"%02d%02d", h,m);
         BOOST_CHECK_MESSAGE(output_written == 4," expected size 4 but found " << output_written);
         Variable var("name","");
         var.set_value( smstime);
         int value = atoi(smstime);
         BOOST_CHECK_MESSAGE(var.value() == value,"expected " << value << " but found " << var.value());
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()


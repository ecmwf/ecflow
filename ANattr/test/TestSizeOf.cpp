//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description
//============================================================================
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include "TimeSeries.hpp"
#include "DayAttr.hpp"

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_size_of )
{
   cout << "ACore:: ...test_size_of\n" ;
   cout << "   sizeof(std::ofstream)      " << sizeof(std::ofstream) << "\n";
   cout << "   sizeof(std::string)        " << sizeof(std::string) << "\n";
   cout << "   sizeof(vector<int>)        " << sizeof(std::vector<int>) << "\n";
   cout << "   sizeof(vector<string>)     " << sizeof(vector<string>) << "\n";
   cout << "   sizeof(std::weak_ptr<int>) " << sizeof(std::weak_ptr<int>) << "\n";
   cout << "   sizeof(nullptr)            " << sizeof(nullptr) << "\n";
   cout << "   sizeof(unique_ptr)         " << sizeof(unique_ptr<int>) << "\n";
   cout << "   sizeof(double)             " << sizeof(double) << "\n";
   cout << "   sizeof(long)               " << sizeof(long) << "\n";
   cout << "   sizeof(int)                " << sizeof(int) << "\n";
   cout << "   sizeof(unsigned int)       " << sizeof(unsigned int) << "\n";
   cout << "   sizeof(bool)               " << sizeof(bool) << "\n";
   cout << "   sizeof(TimeSeries)         " << sizeof(ecf::TimeSeries) << "\n";
   cout << "   sizeof(DayAttr)            " << sizeof(DayAttr) << "\n";

   BOOST_CHECK_MESSAGE(true,"Dummy");
}

BOOST_AUTO_TEST_SUITE_END()

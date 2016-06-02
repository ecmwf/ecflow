//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #29 $
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
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/date_time/posix_time/time_formatters.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include "TimeSeries.hpp"
#include "Calendar.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

using namespace boost;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_time_slot )
{
   cout << "ACore:: ...test_time_slot\n";

   // test timeslot operator
   {
      TimeSlot x;
      TimeSlot y;
      BOOST_CHECK_MESSAGE( x.isNULL(),"Expected NULL");
      BOOST_CHECK_MESSAGE( y.isNULL(),"Expected NULL");
      BOOST_CHECK_MESSAGE( x == y,"Equality operator expected to succeed");
      BOOST_CHECK_MESSAGE( !(x < y),"Less than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(x > y),"Greater than operator expected to fail");
      BOOST_CHECK_MESSAGE( x <= y,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( x >= y,">= operator expected to succeed");
      BOOST_CHECK_MESSAGE( y <= x,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( y >= x,">= operator expected to succeed");

      TimeSlot a(10,12);
      TimeSlot b(10,12);
      BOOST_CHECK_MESSAGE( !a.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( !b.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( a == b,"Equality operator expected to succeed");
      BOOST_CHECK_MESSAGE( !(a < b),"Less than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(a > b),"Greater than operator expected to fail");
      BOOST_CHECK_MESSAGE( a <= b,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( a >= b,">= operator expected to succeed");
      BOOST_CHECK_MESSAGE( b <= a,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( b >= a,">= operator expected to succeed");

      TimeSlot c(0,0);
      TimeSlot d(0,0);
      BOOST_CHECK_MESSAGE( !c.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( !d.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( c == d,"Equality operator expected to succeed");
      BOOST_CHECK_MESSAGE( !(c < d),"Less than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(c > d),"Greater than operator expected to fail");
      BOOST_CHECK_MESSAGE( c <= d,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( c >= d,">= operator expected to succeed");
      BOOST_CHECK_MESSAGE( d <= c,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( d >= c,">= operator expected to succeed");

      TimeSlot a1(10,1);
      TimeSlot b1(10,12);
      BOOST_CHECK_MESSAGE( !a1.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( !b1.isNULL(),"Expected NOT NULL");
      BOOST_CHECK_MESSAGE( a1 != b1,"Equality operator expected to fail");
      BOOST_CHECK_MESSAGE( a1 < b1,"Less than operator expected to succeed");
      BOOST_CHECK_MESSAGE( !(a1 > b1),"Greater than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(a1 >= b1),">= than operator expected to fail");
      BOOST_CHECK_MESSAGE( b1 > a1,"Greater than operator expected to succeed");
      BOOST_CHECK_MESSAGE( b1 >= a1,">= expected to succeed");

      TimeSlot xx(10,1);
      TimeSlot yy(23,12);
      BOOST_CHECK_MESSAGE( xx != yy,"Equality operator expected to fail");
      BOOST_CHECK_MESSAGE( xx < yy,"Less than operator expected to succeed");
      BOOST_CHECK_MESSAGE( xx <= yy,"<= operator expected to succeed");
      BOOST_CHECK_MESSAGE( yy > xx,"Greater than operator expected to succeed");
      BOOST_CHECK_MESSAGE( yy >= xx,">= operator expected to succeed");
      BOOST_CHECK_MESSAGE( !(xx > yy),"Greater than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(xx >= yy),">= operator expected to fail");

      TimeSlot x1(11,0);
      TimeSlot y1(10,0);
      BOOST_CHECK_MESSAGE( x1 != y1,"Equality operator expected to fail");
      BOOST_CHECK_MESSAGE( !(x1 < y1),"Less than operator expected to fail");
      BOOST_CHECK_MESSAGE( !(x1 <= y1),"<= operator expected to fail");
      BOOST_CHECK_MESSAGE( x1 > y1,"Greater than operator expected to succced");
      BOOST_CHECK_MESSAGE( x1 >= y1,">= operator expected to succced");
   }
}

BOOST_AUTO_TEST_SUITE_END()

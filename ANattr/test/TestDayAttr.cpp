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

#include <string>
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

#include "DayAttr.hpp"
#include "Calendar.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_day_attr)
{
   cout << "ANattr:: ...test_day_attr\n";

   // See TimeAttr.hpp for rules concerning isFree() and checkForReque()
   // test time attr isFree(), and checkForRequeue
   Calendar calendar;
   calendar.init(ptime(date(2013,7,9), minutes(0)), Calendar::REAL); // tuesday

   // Represent a day within a week (range 0==Sun to 6==Sat)
   BOOST_CHECK_MESSAGE(calendar.day_of_week() == 2 ," Expected tuesday(2) but found " <<  calendar.day_of_week() );

   DayAttr day(DayAttr::WEDNESDAY);

   int day_changed = 0; // after midnight make sure we keep day_changed
   // day_changed = 0;  tuesday
   // day_changed = 1;  wednesday
   // day_changed = 3;  thursday
   for(int m=1; m < 96; m++) {
      calendar.update( time_duration( hours(1) ) );
      if (calendar.dayChanged()) day_changed++;

      // cout << " day_changed(" << day_changed << ") calendar.day_of_week() = " <<  calendar.day_of_week() << "\n";

      day.calendarChanged( calendar );

      if ( calendar.day_of_week() < day.day() ) {
         BOOST_CHECK_MESSAGE(!day.isFree(calendar),day.toString() << " is free should fail at day " << calendar.day_of_week() );
         BOOST_CHECK_MESSAGE(day.checkForRequeue(calendar),day.toString() << " checkForRequeue should pass at " << calendar.day_of_week() );
      }
      else if (calendar.day_of_week() == day.day()  ) {
         BOOST_CHECK_MESSAGE(day.isFree(calendar),day.toString() << " is free should pass at day " << calendar.day_of_week() );
         BOOST_CHECK_MESSAGE(!day.checkForRequeue(calendar),day.toString() << " checkForRequeue should fail at " << calendar.day_of_week() );
      }
      else {
         BOOST_CHECK_MESSAGE(calendar.day_of_week() > day.day(),"");
         BOOST_CHECK_MESSAGE(!day.isFree(calendar),day.toString() << " is free should fail at day " << calendar.day_of_week() );
         BOOST_CHECK_MESSAGE(!day.checkForRequeue(calendar),day.toString() << " checkForRequeue should fail at " << calendar.day_of_week() );
      }
   }
}


BOOST_AUTO_TEST_SUITE_END()


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

#include "LateAttr.hpp"
#include "Calendar.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_late_attr_submitted )
{
   cout << "ANattr:: ...test_late_attr_submitted\n";

   //REF: ECFLOW-322
   Calendar calendar;
   calendar.init(ptime(date(2013,7,9), minutes(0)), Calendar::REAL); // tuesday

   /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
   ///               relative, so + is simple ignored, if present. If the node stays submitted
   ///               longer than the time specified, the late flag is set
   /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
   ///               is still queued or submitted, the late flag is set
   /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
   ///               taken from the time the node became active, otherwise node must be complete by
   ///               the time given.
   ecf::LateAttr lateAttr;
   lateAttr.addSubmitted( ecf::TimeSlot(0,4) );

   calendar.update( time_duration( minutes(1) ) );
   calendar.update( time_duration( minutes(1) ) );
   calendar.update( time_duration( minutes(1) ) );
   calendar.update( time_duration( minutes(1) ) );
   calendar.update( time_duration( minutes(1) ) );

   // set submitted state at 00:05:00
   //cout << "start:" << to_simple_string(calendar.suiteTime()) << "\n";
   std::pair<NState,boost::posix_time::time_duration> state = std::make_pair(NState::SUBMITTED, calendar.duration() );

   // after four minutes in submitted state, we should be late
   for(int m=1; m < 10; m++) {
      calendar.update( time_duration( minutes(1) ) );
      //cout << "m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";

      lateAttr.checkForLateness(state, calendar );
      //if (lateAttr.isLate()) {
      //   cout << "late at m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";
      //}

      if ( m >= 4 ) {
         BOOST_CHECK_MESSAGE( lateAttr.isLate()," expected to be late at " << to_simple_string(calendar.suiteTime()));
      }
   }
}

BOOST_AUTO_TEST_CASE( test_late_attr_active )
{
   cout << "ANattr:: ...test_late_attr_active\n";

   Calendar calendar;
   calendar.init(ptime(date(2013,7,9), minutes(0)), Calendar::REAL); // tuesday

   /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
   ///               relative, so + is simple ignored, if present. If the node stays submitted
   ///               longer than the time specified, the late flag is set
   /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
   ///               is still queued or submitted, the late flag is set
   /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
   ///               taken from the time the node became active, otherwise node must be complete by
   ///               the time given.

   ecf::LateAttr lateAttr;
   lateAttr.addActive( ecf::TimeSlot(10,0) );

   // set submitted state at 00:00:00
   //cout << "start:" << to_simple_string(calendar.suiteTime()) << "\n";
   std::pair<NState,boost::posix_time::time_duration> state = std::make_pair(NState::SUBMITTED, calendar.duration() );

   // after 10 hours we, if we are not active, we should be late
   for(int m=1; m < 23; m++) {
      calendar.update( time_duration( hours(1) ) );
      //cout << "m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";

      lateAttr.checkForLateness(state, calendar );
//      if (lateAttr.isLate()) {
//         cout << "late at m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";
//      }

      if ( m >= 10 ) {
         BOOST_CHECK_MESSAGE( lateAttr.isLate()," expected to be late at " << to_simple_string(calendar.suiteTime()));
      }
   }
}


BOOST_AUTO_TEST_CASE( test_late_attr_complete_relative )
{
   cout << "ANattr:: ...test_late_attr_complete_relative\n";

   Calendar calendar;
   calendar.init(ptime(date(2013,7,9), minutes(0)), Calendar::REAL); // tuesday

   /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
   ///               relative, so + is simple ignored, if present. If the node stays submitted
   ///               longer than the time specified, the late flag is set
   /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
   ///               is still queued or submitted, the late flag is set
   /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
   ///               taken from the time the node became active, otherwise node must be complete by
   ///               the time given.

   ecf::LateAttr lateAttr;
   lateAttr.addComplete( ecf::TimeSlot(0,15), true);

   // set active state at 00:00:00
//   cout << "start:" << to_simple_string(calendar.suiteTime()) << "\n";
   std::pair<NState,boost::posix_time::time_duration> state = std::make_pair(NState::ACTIVE, calendar.duration() );

   // after 15 minutes relative, if we are not complete, we should be late
   for(int m=1; m < 23; m++) {
      calendar.update( time_duration( minutes(1) ) );
//      cout << "m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";

      lateAttr.checkForLateness(state, calendar );
//      if (lateAttr.isLate()) {
//         cout << "late at m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";
//      }

      if ( m >= 15 ) {
         BOOST_CHECK_MESSAGE( lateAttr.isLate()," expected to be late at " << to_simple_string(calendar.suiteTime()));
      }
   }
}

BOOST_AUTO_TEST_CASE( test_late_attr_complete_real )
{
   cout << "ANattr:: ...test_late_attr_complete_real\n";

   Calendar calendar;
   calendar.init(ptime(date(2013,7,9), minutes(0)), Calendar::REAL); // tuesday

   /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
   ///               relative, so + is simple ignored, if present. If the node stays submitted
   ///               longer than the time specified, the late flag is set
   /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
   ///               is still queued or submitted, the late flag is set
   /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
   ///               taken from the time the node became active, otherwise node must be complete by
   ///               the time given.

   ecf::LateAttr lateAttr;
   lateAttr.addComplete( ecf::TimeSlot(3,0), false);

   // set active state at 00:00:00
//   cout << "start:" << to_simple_string(calendar.suiteTime()) << "\n";
   std::pair<NState,boost::posix_time::time_duration> state = std::make_pair(NState::ACTIVE, calendar.duration() );

   // after 3 hours we, if we are not complete, we should be late
   for(int m=1; m < 7; m++) {

      calendar.update( time_duration( hours(1) ) );
//      cout << "m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";

      lateAttr.checkForLateness(state, calendar );
//      if (lateAttr.isLate()) {
//         cout << "late at m=" << m << " " << to_simple_string(calendar.suiteTime()) << "\n";
//      }

      if ( m >= 3 ) {
         BOOST_CHECK_MESSAGE( lateAttr.isLate()," expected to be late at " << to_simple_string(calendar.suiteTime()));
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()

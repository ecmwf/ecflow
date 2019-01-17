/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
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

#include "TimeAttr.hpp"
#include "TimeSeries.hpp"
#include "Calendar.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_time_string_constrcutor)
{
   cout << "ANattr:: ...test_time_string_constrcutor\n";
   {
      TimeAttr time("+00:30");
      BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 0 &&
                          time.time_series().start().minute() == 30 &&
                          time.time_series().finish().hour() == 0 &&
                          time.time_series().finish().minute() == 0 &&
                          time.time_series().incr().hour() == 0 &&
                          time.time_series().incr().minute() == 0 &&
                          time.time_series().relative(),"Error in time constructor");
   }
   {
      TimeAttr time("12:30");
      BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 12 &&
                          time.time_series().start().minute() == 30 &&
                          time.time_series().finish().hour() == 0 &&
                          time.time_series().finish().minute() == 0 &&
                          time.time_series().incr().hour() == 0 &&
                          time.time_series().incr().minute() == 0 &&
                          !time.time_series().relative(),"Error in time constructor");
   }
   {
      TimeAttr time("+00:30 11:30 00:01");
      BOOST_CHECK_MESSAGE(time.time_series().start().hour() == 0 &&
                          time.time_series().start().minute() == 30 &&
                          time.time_series().finish().hour() == 11 &&
                          time.time_series().finish().minute() == 30 &&
                          time.time_series().incr().hour() == 0 &&
                          time.time_series().incr().minute() == 1 &&
                          time.time_series().relative(),"Error in time constructor");
   }
   {
      BOOST_REQUIRE_THROW(TimeAttr(""),std::runtime_error);
      BOOST_REQUIRE_THROW(TimeAttr(" "),std::runtime_error);
      BOOST_REQUIRE_THROW(TimeAttr("sdsdsdsd"),std::runtime_error);
   }
}

BOOST_AUTO_TEST_CASE( test_time_attr)
{
   cout << "ANattr:: ...test_time_attr\n";

   // See TimeAttr.hpp for rules concerning isFree() and checkForReque()
   // test time attr isFree(), and checkForRequeue
   Calendar calendar;
   calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);

   // Create a test when we can match a time series. Need to sync hour with suite time
   // at hour 1, suite time should also be 01:00, for test to work
   //
   // Create the time series: start  10:00
   //                         finish 20:00
   //                         incr    1:00
   TimeSeries timeSeriesX(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(1,0), false/* relative */);
   TimeSeries timeSeries2X(TimeSlot(11,0), TimeSlot(15,0), TimeSlot(1,0), false/* relative */);
   TimeSeries timeSeries3X(TimeSlot(15,0),  false/* relative */);
   TimeSeries timeSeries4X(TimeSlot(0,0),  false/* relative */);

   TimeSlot t1_min, t1_max,t2_min,t2_max,t3_min,t3_max,t4_min,t4_max;
   timeSeriesX.min_max_time_slots(t1_min, t1_max);
   timeSeries2X.min_max_time_slots(t2_min, t2_max);
   timeSeries3X.min_max_time_slots(t3_min, t3_max);
   timeSeries4X.min_max_time_slots(t4_min, t4_max);
   BOOST_CHECK_MESSAGE(t1_min == TimeSlot(10,0) && t1_max == TimeSlot(20,0),"Not as expected");
   BOOST_CHECK_MESSAGE(t2_min == TimeSlot(11,0) && t2_max == TimeSlot(15,0),"Not as expected");
   BOOST_CHECK_MESSAGE(t3_min == TimeSlot(15,0) && t3_max == TimeSlot(15,0),"Not as expected");
   BOOST_CHECK_MESSAGE(t4_min == TimeSlot(0,0)  && t4_max == TimeSlot(0,0),"Not as expected");

   TimeAttr timeSeries(timeSeriesX);
   TimeAttr timeSeries2(timeSeries2X);
   TimeAttr timeSeries3(timeSeries3X);
   TimeAttr timeSeries4(timeSeries4X);

   std::vector<boost::posix_time::time_duration> timeSeries_free_slots;
   std::vector<boost::posix_time::time_duration> timeSeries2_free_slots;
   timeSeries.time_series().free_slots(timeSeries_free_slots);
   timeSeries2.time_series().free_slots(timeSeries2_free_slots);
   BOOST_CHECK_MESSAGE(timeSeries_free_slots.size() == 11,"Expected 11 free slots for " << timeSeries.toString() << " but found "  << timeSeries_free_slots.size());
   BOOST_CHECK_MESSAGE(timeSeries2_free_slots.size() == 5,"Expected 5 free slots for " << timeSeries2.toString() << " but found "  << timeSeries_free_slots.size());
//   cout << "time " << timeSeries.toString() << " free slots:";
//   for(size_t i = 0; i < timeSeries_free_slots.size(); i++)  cout << timeSeries_free_slots[i] << " ";
//   cout << "\n";

   // follow normal process
   timeSeries.reset( calendar );
   timeSeries2.reset( calendar );
   timeSeries3.reset( calendar );
   timeSeries4.reset( calendar );

   bool day_changed = false; // after midnight make sure we keep day_changed
   for(int m=1; m < 96; m++) {
      calendar.update( time_duration( minutes(30) ) );
      if (!day_changed) day_changed = calendar.dayChanged();

      boost::posix_time::time_duration time = calendar.suiteTime().time_of_day();
      //cout << time << " day_changed(" << day_changed << ")\n";

      timeSeries.calendarChanged( calendar );
      timeSeries2.calendarChanged( calendar );
      timeSeries3.calendarChanged( calendar );
      timeSeries4.calendarChanged( calendar );

      //cout << to_simple_string(calendar.suiteTime()) << "\n";

      if (calendar.dayChanged()) {
         BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max)," expected " << timeSeries.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()));
      }
      else if (time < timeSeries.time_series().start().duration()) {
         BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),timeSeries.toString() << " should NOT be free at time " << time );
         BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << " checkForRequeue should pass at " << time );
      }
      else if (time >= timeSeries.time_series().start().duration() && time <=timeSeries.time_series().finish().duration()) {

         bool matches_free_slot = false;
         for(size_t i = 0; i < timeSeries_free_slots.size(); i++) {
            if (time == timeSeries_free_slots[i]) { matches_free_slot = true; break; }
         }
         if (matches_free_slot) BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),timeSeries.toString() << " should be free at time " << time );
         else                   BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),timeSeries.toString() << " should be fail at time " << time );

         /// At the last hour checkForRequeue should return false; This ensures that value will
         /// not get incremented and so, should leave node in the complete state.
         if ( time < timeSeries.time_series().finish().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << " checkForRequeue should be free at time " << time );
         }
         else {
            BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << "checkForRequeue should Not free at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),timeSeries.toString() << " should be holding at time " << time );
         BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << " should fail at " << time );
      }


      if (calendar.dayChanged()) {
         BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max)," expected " << timeSeries2.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()));
      }
      else if (time < timeSeries2.time_series().start().duration()) {
         BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),timeSeries2.toString() << " should NOT be free at time " << time );
         BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max),timeSeries2.toString() << " checkForRequeue should pass at " << time );
      }
      else if (time >= timeSeries2.time_series().start().duration() && time <=timeSeries2.time_series().finish().duration()) {

         bool matches_free_slot = false;
         for(size_t i = 0; i < timeSeries2_free_slots.size(); i++) {
            if (time == timeSeries2_free_slots[i]) { matches_free_slot = true; break;}
         }
         if (matches_free_slot) BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),timeSeries2.toString() << " should be free at time " << time );
         else                   BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),timeSeries2.toString() << " should be fail at time " << time );


         /// At the last time checkForRequeue should return false; This ensures that value will
         /// not get incremented and so, should leave node in the complete state.
         if ( time < timeSeries2.time_series().finish().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max),timeSeries2.toString() << " checkForRequeue should be free at time " << time );
         }
         else {
            BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar,t2_min,t2_max),timeSeries2.toString() << "checkForRequeue should Not free at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),timeSeries2.toString() << " should be holding at time " << time );
         BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar,t2_min,t2_max),timeSeries2.toString() << " should fail at " << time );
      }


      // Single slot, Once a single slot is Free it *stays* free until explicitly requeued, (i.e by parent repeat/cron)
      if (!day_changed) {
         if (time < timeSeries3.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(!timeSeries3.isFree(calendar),timeSeries3.toString() << " should be fail at time " << time );
         }
         else if (time == timeSeries3.time_series().start().duration()  ) {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " should be free at time " << time );
         }
         else if (time > timeSeries3.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " isFree should pass at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " should be free at time after day change" << time );
      }
      BOOST_CHECK_MESSAGE(!timeSeries3.checkForRequeue(calendar,t3_min,t3_max),timeSeries3.toString() << " checkForRequeue should fail at " << time );


      // single slot at midnight, Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent repeat/cron)
      if (!day_changed) {
         if (time == timeSeries4.time_series().start().duration()  ) {
            BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),timeSeries4.toString() << " should be free at time " << time );
         }
         else {
            BOOST_CHECK_MESSAGE(!timeSeries4.isFree(calendar),timeSeries4.toString() << " day_changed(" << day_changed << ")  isFree should fail at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),timeSeries4.toString() << " day_changed(" << day_changed << ")  isFree should pass at time " << time );
      }
      BOOST_CHECK_MESSAGE(!timeSeries4.checkForRequeue(calendar,t4_min,t4_max),timeSeries4.toString() << " checkForRequeue should fail at " << time );


       // Typically when a time is free, it stays free, until it is re-queued
       // However in order to test isFree for time with time intervals, we need to re-queue
       timeSeries.requeue( calendar );
       timeSeries2.requeue( calendar );

       // Do not requeue time 00, and time 15, so that we can check for free
   }
}

BOOST_AUTO_TEST_CASE( test_time_once_free_stays_free)
{
   cout << "ANattr:: ...test_time_once_free_stays_free\n";

   Calendar calendar;
   calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);

   TimeSeries timeSeriesX(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(1,0), false/* relative */);
   TimeSeries timeSeries2X(TimeSlot(11,0), TimeSlot(15,0), TimeSlot(1,0), false/* relative */);
   TimeSeries timeSeries3X(TimeSlot(15,0),  false/* relative */);
   TimeSeries timeSeries4X(TimeSlot(0,0),  false/* relative */);

   TimeAttr timeSeries(timeSeriesX );
   TimeAttr timeSeries2(timeSeries2X );
   TimeAttr timeSeries3(timeSeries3X );
   TimeAttr timeSeries4(timeSeries4X );

   bool day_changed = false; // after midnight make sure we keep day_changed
   for(int m=1; m < 96; m++) {
      calendar.update( time_duration( minutes(30) ) );
      if (!day_changed) {
         day_changed = calendar.dayChanged();
      }
      boost::posix_time::time_duration time = calendar.suiteTime().time_of_day();
      // cout << time << " day_changed(" << day_changed << ")\n";

      timeSeries.calendarChanged( calendar );
      timeSeries2.calendarChanged( calendar );
      timeSeries3.calendarChanged( calendar );
      timeSeries4.calendarChanged( calendar );

      // **********************************************************************************
      // When a time (regardless of whether its single slot or time series) is free, it stays free,
      // until explicitly re-queued,
      // ***********************************************************************************

      if (time < timeSeries.time_series().start().duration()) {
         if (!day_changed) BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),timeSeries.toString() << " should NOT be free at time " << time );
         else BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),timeSeries.toString() << " should be free at time " << time );
      }
      else if (time >= timeSeries.time_series().start().duration()) {
         BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),timeSeries.toString() << " should be free at time " << time );
      }


      if (time < timeSeries2.time_series().start().duration()) {
         if (!day_changed) BOOST_CHECK_MESSAGE(!timeSeries2.isFree(calendar),timeSeries2.toString() << " should NOT be free at time " << time );
         else BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),timeSeries.toString() << " should be free at time " << time );
      }
      else if (time >= timeSeries2.time_series().start().duration()) {
         BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),timeSeries2.toString() << " should be free at time " << time );
      }

      if (!day_changed) {
         if (time == timeSeries3.time_series().start().duration()  ) {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " should be free at time " << time );
         }
         else if (time > timeSeries3.time_series().start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " isFree, once free should stay free at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries3.toString() << " should be free at time after day change " << time );
      }


      // single slot at midnight, Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent repeat/cron)
      if (!day_changed) {
         if (time == timeSeries4.time_series().start().duration()  ) {
            BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),timeSeries4.toString() << " should be free at time " << time );
         }
         else {
            BOOST_CHECK_MESSAGE(!timeSeries4.isFree(calendar),timeSeries4.toString() << " day_changed(" << day_changed << ")  isFree should fail at time " << time );
         }
      }
      else {
         BOOST_CHECK_MESSAGE(timeSeries4.isFree(calendar),timeSeries4.toString() << " day_changed(" << day_changed << ")  isFree should pass at time " << time );
      }
   }
}


BOOST_AUTO_TEST_CASE( test_time_attr_multiples )
{
   cout << "ANattr:: ...test_time_attr_multiples\n";

   // See TimeAttr.hpp for rules concerning isFree() and checkForReque()
   // test time attr isFree(), and checkForRequeue
   Calendar calendar;
   calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);

   TimeSeries timeSeries1530(TimeSlot(15,30),  false/* relative */);
   TimeSeries timeSeries1630(TimeSlot(16,30),  false/* relative */);
   TimeSeries timeSeries2030(TimeSlot(20,30),  false/* relative */);

   TimeSlot t1_min, t1_max;
   timeSeries1530.min_max_time_slots(t1_min, t1_max);
   timeSeries1630.min_max_time_slots(t1_min, t1_max);
   timeSeries2030.min_max_time_slots(t1_min, t1_max);
   BOOST_CHECK_MESSAGE(t1_min == TimeSlot(15,30) && t1_max == TimeSlot(20,30),"Not as expected");

   TimeAttr timeSeries(timeSeries1530);
   TimeAttr timeSeries2(timeSeries1630);
   TimeAttr timeSeries3(timeSeries2030);

   bool day_changed = false; // after midnight make sure we keep day_changed
    for(int m=1; m < 96; m++) {
       calendar.update( time_duration( minutes(30) ) );
       if (!day_changed) {
          day_changed = calendar.dayChanged();
       }
       boost::posix_time::time_duration time = calendar.suiteTime().time_of_day();
 //      cout << time << " day_changed(" << day_changed << ")\n";

      timeSeries.calendarChanged( calendar );
      timeSeries2.calendarChanged( calendar );
      timeSeries3.calendarChanged( calendar );

      if (!day_changed) {
         if ( time < t1_max.duration()) {
            BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << " checkForRequeue should pass at " << time );
            BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t1_min,t1_max),timeSeries2.toString() << " checkForRequeue should pass at " << time );
            BOOST_CHECK_MESSAGE(timeSeries3.checkForRequeue(calendar,t1_min,t1_max),timeSeries3.toString() << " checkForRequeue should pass at " << time );
         }
         else {
            BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max),timeSeries.toString() << " checkForRequeue should fail at " << time );
            BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar,t1_min,t1_max),timeSeries2.toString() << " checkForRequeue should fail at " << time );
            BOOST_CHECK_MESSAGE(!timeSeries3.checkForRequeue(calendar,t1_min,t1_max),timeSeries3.toString() << " checkForRequeue should fail at " << time );
         }
      }
      else {
         // Once a single slot if Free it *stays* free until explicitly requeued, (i.e by parent repeat/cron)
         BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),timeSeries.toString() << " day_changed(" << day_changed << ")  isFree should pass at time " << time );
         BOOST_CHECK_MESSAGE(timeSeries2.isFree(calendar),timeSeries2.toString() << " day_changed(" << day_changed << ")  isFree should pass at time " << time );
         BOOST_CHECK_MESSAGE(timeSeries3.isFree(calendar),timeSeries2.toString() << " day_changed(" << day_changed << ")  isFree should pass at time " << time );
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()


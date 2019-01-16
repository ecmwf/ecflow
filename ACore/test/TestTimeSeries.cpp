//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #32 $ 
//
// Copyright 2009-2019 ECMWF.
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

BOOST_AUTO_TEST_CASE( test_time_series_min_max_time_slots )
{
   cout << "ACore:: ...test_time_series_min_max_time_slots\n";

   TimeSlot the_min;
   TimeSlot the_max;

   TimeSeries a(9,0,true);
   a.min_max_time_slots(the_min,the_max);
   BOOST_CHECK_MESSAGE( the_min == TimeSlot(9,0),"Max min time slot failed");
   BOOST_CHECK_MESSAGE( the_max == TimeSlot(9,0),"Max min time slot failed");

   TimeSeries b(TimeSlot(10,12),false);
   b.min_max_time_slots(the_min,the_max);
   BOOST_CHECK_MESSAGE( the_min == TimeSlot(9,0),"Max min time slot failed");
   BOOST_CHECK_MESSAGE( the_max == TimeSlot(10,12),"Max min time slot failed");

   TimeSeries c(TimeSlot(0,10), TimeSlot(10,13), TimeSlot(0,1));
   c.min_max_time_slots(the_min,the_max);
   BOOST_CHECK_MESSAGE( the_min == TimeSlot(0,10),"Max min time slot failed");
   BOOST_CHECK_MESSAGE( the_max == TimeSlot(10,13),"Max min time slot failed");

   TimeSeries x;
   x.min_max_time_slots(the_min,the_max);
   BOOST_CHECK_MESSAGE( the_min == TimeSlot(),"Max min time slot failed");
   BOOST_CHECK_MESSAGE( the_max == TimeSlot(10,13),"Max min time slot failed");

}


BOOST_AUTO_TEST_CASE( test_time_series_constrcution )
{
	cout << "ACore:: ...test_time_series_constrcution\n";

	{
		TimeSeries x;
		TimeSeries y;
		BOOST_CHECK_MESSAGE( x == y,"Equality operator expected to succeed");

		TimeSeries a(10,12,true);
		TimeSeries b(10,12,true);
		BOOST_CHECK_MESSAGE( a == b,"Equality operator expected to succeed");

		TimeSeries c(10,12,false);
		TimeSeries d(10,12,false);
		BOOST_CHECK_MESSAGE( c == d,"Equality operator expected to succeed");

		TimeSeries e(TimeSlot(14,59),false);
		TimeSeries f(TimeSlot(14,59),false);
 		BOOST_CHECK_MESSAGE( e == f,"Equality operator expected to succeed");

 		TimeSeries g(TimeSlot(0,10), TimeSlot(10,4), TimeSlot(0,1));
 		TimeSeries h(TimeSlot(0,10), TimeSlot(10,4), TimeSlot(0,1));
 		BOOST_CHECK_MESSAGE( g == h,"Equality operator expected to succeed");
	}
	{
		TimeSeries a(10,12,false);
		TimeSeries b(10,12,true);
 		BOOST_CHECK_MESSAGE( a != b,"Equality operator expected to fail");

		TimeSeries c(10,13,false);
		TimeSeries d(10,12,false);
		BOOST_CHECK_MESSAGE( c != d,"Equality operator expected to fail");

		TimeSeries e(TimeSlot(14,59),false);
		TimeSeries f(TimeSlot(14,10),false);
 		BOOST_CHECK_MESSAGE( e != f,"Equality operator expected to fail");

 		TimeSeries g(TimeSlot(0,10), TimeSlot(10,4), TimeSlot(0,1), true);
 		TimeSeries h(TimeSlot(0,10), TimeSlot(10,4), TimeSlot(0,1), false);
 		BOOST_CHECK_MESSAGE( g != h,"Equality operator expected to fail");
 	}

	/// Basic test for time series of getters and setters
	TimeSeries timeSeries(TimeSlot(0,1), TimeSlot(0,4), TimeSlot(0,1));
	BOOST_CHECK_MESSAGE(timeSeries.hasIncrement(),"expected increment");
	std::string errMsg; BOOST_CHECK_MESSAGE(timeSeries.checkInvariants(errMsg),errMsg);

	TimeSeries another = timeSeries;
	BOOST_CHECK_MESSAGE(another == timeSeries,"copy constructor failed for TimeSeries ");
	BOOST_CHECK_MESSAGE(another.checkInvariants(errMsg),errMsg);
	BOOST_CHECK_MESSAGE(timeSeries.checkInvariants(errMsg),errMsg);

	TimeSeries timeSeries2(TimeSlot(0,1), TimeSlot(0,4), TimeSlot(0,1));
	BOOST_CHECK_MESSAGE(timeSeries2.checkInvariants(errMsg),errMsg);
	another = timeSeries2;
	BOOST_CHECK_MESSAGE(another ==  timeSeries2,"assignment operator failed for TimeSeries ");
	BOOST_CHECK_MESSAGE(another.checkInvariants(errMsg),errMsg);

	TimeSeries timeSeries3(TimeSlot(0,1));
	BOOST_CHECK_MESSAGE(timeSeries3.checkInvariants(errMsg),errMsg);
}

BOOST_AUTO_TEST_CASE( test_time_series )
{
	cout << "ACore:: ...test_time_series\n";

	/// Basic test for time series of getters and setters
	TimeSeries timeSeries(TimeSlot(0,1), TimeSlot(0,4), TimeSlot(0,1));
	BOOST_CHECK_MESSAGE(timeSeries.hasIncrement(),"expected increment");

	std::string expected = "00:01:00";
	std::string actual = to_simple_string(timeSeries.start().duration());
	BOOST_CHECK_MESSAGE(expected == actual," expected " << expected << " but found " << actual );

	expected = "00:04:00";
	actual = to_simple_string(timeSeries.finish().duration());
	BOOST_CHECK_MESSAGE(expected == actual," expected " << expected << " but found " << actual );

	expected = "00:01:00";
	actual = to_simple_string(timeSeries.incr().duration());
	BOOST_CHECK_MESSAGE(expected == actual," expected " << expected << " but found " << actual );


	TimeSeries timeSeries2(TimeSlot(0,1));
	BOOST_CHECK_MESSAGE(!timeSeries2.hasIncrement(),"Not expecting time series");
}

BOOST_AUTO_TEST_CASE( test_time_series_increment_real )
{
	cout << "ACore:: ...test_time_series_increment_real\n";

	// Test time series with  a calendar, we update calendar then
	// test time series isFree(), and checkForRequeue
	Calendar c;
	c.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);


	// Create a test when we can match a time series. Need to sync hour with suite time
	// at hour 1, suite time should also be 01:00, for test to work
	//
	// Create the time series: start  10:00
	//                         finish 20:00
	//                         incr    1:00
	TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(1,0), true/* relative */);
	TimeSeries timeSeries2(TimeSlot(11,0), TimeSlot(15,0), TimeSlot(1,0), true/* relative */);
	TimeSeries timeSeries3(TimeSlot(15,0),  true/* relative */);

   TimeSlot t1_min, t1_max,t2_min,t2_max,t3_min,t3_max;
   timeSeries.min_max_time_slots(t1_min, t1_max);
   timeSeries2.min_max_time_slots(t2_min, t2_max);
   timeSeries3.min_max_time_slots(t3_min, t3_max);
   BOOST_CHECK_MESSAGE(t1_min == TimeSlot(10,0) && t1_max == TimeSlot(20,0),"Not as expected");
   BOOST_CHECK_MESSAGE(t2_min == TimeSlot(11,0) && t2_max == TimeSlot(15,0),"Not as expected");
   BOOST_CHECK_MESSAGE(t3_min == TimeSlot(15,0) && t3_max == TimeSlot(15,0),"Not as expected");

   // Follow normal process
   timeSeries.reset( c );
   timeSeries2.reset( c );
   timeSeries3.reset( c );

	for(int hour=1; hour < 24; hour++) {
		// Update calendar every hour, then see we can match time series, *RELATIVE* to suite start
		c.update( time_duration( hours(1) ) );
		timeSeries.calendarChanged( c );
		timeSeries2.calendarChanged( c );
		timeSeries3.calendarChanged( c );

//		cerr << "hour = " << hour << " calendar_duration " << to_simple_string(timeSeries.duration(c))
//		    << " timeSeries=" << timeSeries.toString() << " timeSeries2=" << timeSeries2.toString() << " timeSeries3=" << timeSeries3.toString() << "\n";
		if (hour < timeSeries.start().hour()) {
         BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(c,t1_min,t1_max)," Time series " << timeSeries.toString() << "checkForRequeue should pass at " << hour );
         BOOST_CHECK_MESSAGE(!timeSeries.isFree(c),"Time series " << timeSeries.toString() << " should NOT be free at hour " << hour );
		}
		else if (hour >= timeSeries.start().hour() && hour <= timeSeries.finish().hour()) {
			BOOST_CHECK_MESSAGE(timeSeries.isFree(c),"Time series " << timeSeries.toString() << " should be free at hour " << hour );

			/// At the last hour checkForRequeue should return false; This ensures that value will
			/// not get incremented and so, should leave node in the complete state.
			if ( hour < timeSeries.finish().hour()) {
				BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(c,t1_min,t1_max),"Time series " << timeSeries.toString() << " checkForRequeue should be free at hour " << hour );
			}
			else {
				BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(c,t1_min,t1_max),"Time series " << timeSeries.toString() << "checkForRequeue should Not free at hour " << hour );
			}
		}
		else {
			BOOST_CHECK_MESSAGE(!timeSeries.isFree(c),"Time series " << timeSeries.toString() << " should NOT be free at hour " << hour );
 			BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(c,t1_min,t1_max)," Time series " << timeSeries.toString() << " should fail at " << hour );
		}


      if (hour < timeSeries2.start().hour()) {
         BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(c,t1_min,t1_max)," Time series " << timeSeries2.toString() << "checkForRequeue should pass at " << hour );
         BOOST_CHECK_MESSAGE(!timeSeries2.isFree(c),"Time series " << timeSeries2.toString() << " should NOT be free at hour " << hour );
      }
      else if (hour >= timeSeries2.start().hour() && hour <=timeSeries2.finish().hour()) {
			BOOST_CHECK_MESSAGE(timeSeries2.isFree(c),"Time series " << timeSeries2.toString() << " should be free at hour " << hour );

			/// At the last hour checkForRequeue should return false;
			if ( hour < timeSeries2.finish().hour()) {
				BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(c,t2_min,t2_max),"Time series " << timeSeries2.toString() << " checkForRequeue should be free at hour " << hour );
			}
			else {
				BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(c,t2_min,t2_max),"Time series " << timeSeries2.toString() << "checkForRequeue should Not free at hour " << hour );
			}
  		}
		else {
			BOOST_CHECK_MESSAGE(!timeSeries2.isFree(c),"Time series " << timeSeries2.toString() << " not be free at hour " << hour );
			BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(c,t2_min,t2_max)," Time series " << timeSeries2.toString() << " should fail at " << hour );
 		}


		if (hour == timeSeries3.start().hour()  ) {
			BOOST_CHECK_MESSAGE(timeSeries3.isFree(c),"Time series " << timeSeries3.toString() << " should be free at hour " << hour );
 		}
		else if (hour < timeSeries3.start().hour()) {
			BOOST_CHECK_MESSAGE(!timeSeries3.isFree(c),"Time series " << timeSeries3.toString() << " isFree should fail at hour " << hour );
 		}
		else if (hour > timeSeries3.start().hour()) {
			BOOST_CHECK_MESSAGE(!timeSeries3.isFree(c),"Time series " << timeSeries3.toString() << " is Free should fail at hour " << hour );
	 	}
		BOOST_CHECK_MESSAGE(!timeSeries3.checkForRequeue(c,t3_min,t3_max)," Time series " << timeSeries3.toString() << " checkForRequeue should fail at " << hour );
	}
}

BOOST_AUTO_TEST_CASE( test_time_series_requeueable_and_compute_next_time_slot )
{
   cout << "ACore:: ...test_time_series_requeueable_and_compute_next_time_slot\n";

   // Test time series with  a calendar, we update calendar then
   // test time series requeueable(), and compute_next_time_slot
   // This are used with the WHY command
   Calendar c;
   c.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);


   // Create a test when we can match a time series. Need to sync hour with suite time
   // at hour 1, suite time should also be 01:00, for test to work
   //
   // Create the time series: start  10:00
   //                         finish 20:00
   //                         incr    1:00
   TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(1,0), true/* relative */);
   TimeSeries timeSeries2(TimeSlot(11,0), TimeSlot(15,0), TimeSlot(1,0), true/* relative */);
   TimeSeries timeSeries3(TimeSlot(15,0),  true/* relative */);

   for(int hour=1; hour < 24; hour++) {
      // Update calendar every hour, then see we can match time series,
      c.update( time_duration( hours(1) ) );
      timeSeries.calendarChanged( c );
      timeSeries2.calendarChanged( c );
      timeSeries3.calendarChanged( c );

//    cerr << "hour = " << hour << " calendar_duration " << to_simple_string(timeSeries.duration(c))
//        << " timeSeries=" << timeSeries.toString() << " timeSeries2=" << timeSeries2.toString() << " timeSeries3=" << timeSeries3.toString() << "\n";
      if (hour < timeSeries.start().hour()) {
         TimeSlot next_time_slot = timeSeries.compute_next_time_slot(c);
         TimeSlot expected(10,0);
         BOOST_CHECK_MESSAGE(next_time_slot ==expected," Time series " << timeSeries.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries.requeueable(c),"Time series " << timeSeries.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour == timeSeries.start().hour() ) {
         TimeSlot next_time_slot = timeSeries.compute_next_time_slot(c);
         TimeSlot expected(11,0);
         BOOST_CHECK_MESSAGE(next_time_slot == expected," Time series " << timeSeries.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries.requeueable(c),"Time series " << timeSeries.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour > timeSeries.start().hour() &&  hour < timeSeries.finish().hour()) {
         TimeSlot next_time_slot = timeSeries.compute_next_time_slot(c);
         TimeSlot expected(hour+1,0);
         BOOST_CHECK_MESSAGE(next_time_slot == expected," Time series " << timeSeries.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries.requeueable(c),"Time series " << timeSeries.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour == timeSeries.finish().hour()) {
         TimeSlot next_time_slot = timeSeries.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries.toString() << " at " << hour << " expected next time slot  to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries.requeueable(c),"Time series " << timeSeries.toString() << " should NOT be requeueable at hour " << hour );
      }
      else if (hour > timeSeries.finish().hour()) {
         TimeSlot next_time_slot = timeSeries.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries.toString() << " at " << hour << " expected next time slot to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries.requeueable(c),"Time series " << timeSeries.toString() << " should NOT be requeueable at hour " << hour );
      }


      if (hour < timeSeries2.start().hour()) {
         TimeSlot next_time_slot = timeSeries2.compute_next_time_slot(c);
         TimeSlot expected(11,0);
         BOOST_CHECK_MESSAGE(next_time_slot ==expected," Time series " << timeSeries2.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries2.requeueable(c),"Time series " << timeSeries2.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour == timeSeries2.start().hour() ) {
         TimeSlot next_time_slot = timeSeries2.compute_next_time_slot(c);
         TimeSlot expected(12,0);
         BOOST_CHECK_MESSAGE(next_time_slot == expected," Time series " << timeSeries2.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries2.requeueable(c),"Time series " << timeSeries2.toString() << " should NOT be requeueable at hour " << hour );
      }
      else if (hour > timeSeries2.start().hour() &&  hour < timeSeries2.finish().hour()) {
         TimeSlot next_time_slot = timeSeries2.compute_next_time_slot(c);
         TimeSlot expected(hour+1,0);
         BOOST_CHECK_MESSAGE(next_time_slot == expected," Time series " << timeSeries2.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries2.requeueable(c),"Time series " << timeSeries2.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour == timeSeries2.finish().hour()) {
         TimeSlot next_time_slot = timeSeries2.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries2.toString() << " at " << hour << " expected next time slot  to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries2.requeueable(c),"Time series " << timeSeries2.toString() << " should NOT be requeueable at hour " << hour );
      }
      else if (hour > timeSeries2.finish().hour()) {
         TimeSlot next_time_slot = timeSeries2.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries2.toString() << " at " << hour << " expected next time slot to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries2.requeueable(c),"Time series " << timeSeries2.toString() << " should NOT be requeueable at hour " << hour );
      }


      if (hour < timeSeries3.start().hour()) {
         TimeSlot next_time_slot = timeSeries3.compute_next_time_slot(c);
         TimeSlot expected(15,0);
         BOOST_CHECK_MESSAGE(next_time_slot ==expected," Time series " << timeSeries3.toString() << " at " << hour << " expected next time slot at " << expected.toString() << " but found " << next_time_slot.toString());
         BOOST_CHECK_MESSAGE(timeSeries3.requeueable(c),"Time series " << timeSeries3.toString() << " should be requeueable at hour " << hour );
      }
      else if (hour == timeSeries3.start().hour() ) {
         TimeSlot next_time_slot = timeSeries3.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries3.toString() << " at " << hour << " expected next time slot to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries3.requeueable(c),"Time series " << timeSeries3.toString() << " should NOT be requeueable at hour " << hour );
      }
      else if (hour > timeSeries3.start().hour()) {
         TimeSlot next_time_slot = timeSeries3.compute_next_time_slot(c);
         BOOST_CHECK_MESSAGE(next_time_slot.isNULL()," Time series " << timeSeries3.toString() << " at " << hour << " expected next time slot to be NULL");
         BOOST_CHECK_MESSAGE(!timeSeries3.requeueable(c),"Time series " << timeSeries3.toString() << " should NOT be requeueable at hour " << hour );
      }
   }
}


BOOST_AUTO_TEST_CASE( test_time_series_finish_not_divisble_by_increment )
{
	cout << "ACore:: ...test_time_series_finish_not_divisble_by_increment\n";

	// HANDLE CASE WHERE FINISH MINUTES IS NOT DIVISIBLE BY THE INCREMENT

	// Test time series with  a calendar, we update calendar then
	// test time series isFree(), and checkForRequeue
	Calendar calendar;
	calendar.init(ptime(date(2008,10,8), hours(0) ), Calendar::REAL);

	// Create a test when we can match a time series.
	// NOTE: Finish minute is not a multiple of INCREMENT hence last valid time slot is 23:50
	//
	// Create the time series: start  00:00
	//                         finish 23:59
	//                         incr   00:10
	TimeSeries timeSeries(TimeSlot(0,0), TimeSlot(23,59), TimeSlot(0,10), false/* relative */);
	TimeSeries timeSeries2(TimeSlot(0,30), TimeSlot(23,59), TimeSlot(4,0), false/* relative */);

   TimeSlot t1_min, t1_max,t2_min,t2_max;
   timeSeries.min_max_time_slots(t1_min, t1_max);
   timeSeries2.min_max_time_slots(t2_min, t2_max);
   BOOST_CHECK_MESSAGE(t1_min == TimeSlot(0,0) && t1_max == TimeSlot(23,59),"Not as expected");
   BOOST_CHECK_MESSAGE(t2_min == TimeSlot(0,30) && t2_max == TimeSlot(23,59),"Not as expected");

 	time_duration last = hours(23) + minutes(50);  // last valid time is 23:50
	time_duration last2 = hours(20) + minutes(30);  // last valid time is 20:30

	// follow normal process
   timeSeries.reset(calendar);
   timeSeries2.reset(calendar);

 	for(int hour=0; hour < 24; hour++) {
 		for( int minute=0; minute<60; minute++) {

 			calendar.update( minutes(1) );
         timeSeries.calendarChanged(calendar);
         timeSeries2.calendarChanged(calendar);

 			//cout << to_simple_string(calendar.suiteTime()) << "\n";

 			if (calendar.dayChanged()) {
            BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max)," expected " << timeSeries.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()));
 			}
 			else if ( calendar.suiteTime().time_of_day() < timeSeries.start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max)," expected " << timeSeries.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()) );
 			}
 			else if ( calendar.suiteTime().time_of_day() >= timeSeries.start().duration() && calendar.suiteTime().time_of_day() < last	) {
     			BOOST_CHECK_MESSAGE(timeSeries.checkForRequeue(calendar,t1_min,t1_max)," expected " << timeSeries.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime())  );
  			}
  			else {
     			BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max)," expected " << timeSeries.toString() << " checkForRequeue to fail at " << to_simple_string(calendar.suiteTime()) );
  			}

         if (calendar.dayChanged()) {
            BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max)," expected " << timeSeries2.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()));
         }
         else if ( calendar.suiteTime().time_of_day() < timeSeries2.start().duration()) {
            BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max)," expected " << timeSeries2.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()) );
         }
         else if ( calendar.suiteTime().time_of_day() >= timeSeries2.start().duration() && calendar.suiteTime().time_of_day() < last2	) {
     			BOOST_CHECK_MESSAGE(timeSeries2.checkForRequeue(calendar,t2_min,t2_max)," expected " << timeSeries2.toString() << " checkForRequeue to pass at " << to_simple_string(calendar.suiteTime()));
  			}
  			else {
     			BOOST_CHECK_MESSAGE(!timeSeries2.checkForRequeue(calendar,t2_min,t2_max)," expected " << timeSeries2.toString() << " checkForRequeue to fail at " << to_simple_string(calendar.suiteTime()));
  			}

//         if (calendar.dayChanged()) {
//            timeSeries.reset(calendar);
//            timeSeries2.reset(calendar);
//         }
 		}
 	}
}

BOOST_AUTO_TEST_CASE( test_time_series_miss_time_slot )
{
   cout << "ACore:: ...test_time_series_miss_time_slot\n";

   Calendar calendar;
   calendar.init(ptime(date(2008,10,8), hours(10) ), Calendar::REAL);

   // Create a test when we can match a time series.
   // NOTE: Finish minute is not a multiple of INCREMENT hence last valid time slot is 23:50
   //
   // Create the time series: start  10:00
   //                         finish 23:59
   //                         incr   00:10
   TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(23,59), TimeSlot(0,10), false/* relative */);
   timeSeries.miss_next_time_slot(); // will increment next_time_slot
   BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,10), "miss time slot not working ");

   timeSeries.miss_next_time_slot(); // will increment next_time_slot
   BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,20), "miss time slot not working ");

   // requeue time series,
   timeSeries.requeue(calendar); // calendar time is at midnight 10:00
   BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,10),"Expected requeue to reset time series");
}

BOOST_AUTO_TEST_CASE( test_time_series_miss_time_slot_1 )
{
   cout << "ACore:: ...test_time_series_miss_time_slot_1\n";

   // Create calendar before time series
   {
      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(9) ), Calendar::REAL);  // 09:00

      TimeSeries timeSeries(10,0, false/* relative */);                 // 10:00

      TimeSlot t1_min, t1_max;
      timeSeries.min_max_time_slots(t1_min, t1_max);
      BOOST_CHECK_MESSAGE(t1_min == TimeSlot(10,0) && t1_max == TimeSlot(10,0),"Not as expected");


      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time holding at 10:00 since calendar is 09:00");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false always");

      timeSeries.miss_next_time_slot();
      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time to hold");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false");

      timeSeries.requeue(calendar);
      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time holding at 10:00 since calendar is 09:00");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false after Requeue");
   }
   {
      // Create a time after the time series
      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(11) ), Calendar::REAL); // 11:00

      TimeSeries timeSeries(10,0, false/* relative */);                 // 10:00

      TimeSlot t1_min, t1_max;
      timeSeries.min_max_time_slots(t1_min, t1_max);
      BOOST_CHECK_MESSAGE(t1_min == TimeSlot(10,0) && t1_max == TimeSlot(10,0),"Not as expected");


      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time NOT to be free, since calendar time is after time slot");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false, since calendar time > slot time");

      timeSeries.miss_next_time_slot();
      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time to hold");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false");

      timeSeries.requeue(calendar);
      BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar), "Expected time to hold, since calendar time is after time slot");
      BOOST_CHECK_MESSAGE(!timeSeries.checkForRequeue(calendar,t1_min,t1_max), "Expected checkForRequeue to return false,since calendar time is after time slot");
   }
}

BOOST_AUTO_TEST_CASE( test_time_series_reset )
{
   cout << "ACore:: ...test_time_series_reset\n";


   // Create a test when we can match a time series.
   // NOTE: Finish minute is not a multiple of INCREMENT hence last valid time slot is 23:50
   //
   // Create the time series: start  10:00
   //                         finish 23:59
   //                         incr   00:10
   TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(23,59), TimeSlot(0,10), false/* relative */);
   BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,0), "defaults not correct");

   { // set calendar before time series start & then reset

      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(9) ), Calendar::REAL);
      timeSeries.reset(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,0), "Reset should set value(next_valid_time_slot) to start." << timeSeries.dump());

      timeSeries.miss_next_time_slot();

      timeSeries.requeue(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,0), "requeue should set value(next_valid_time_slot) to start."<< timeSeries.dump());
   }
   { // set calendar at time series start & then reset

      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(10) ), Calendar::REAL);
      timeSeries.reset(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,0), "Reset should set value(next_valid_time_slot) to start."<< timeSeries.dump());

      timeSeries.miss_next_time_slot();

      timeSeries.requeue(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(10,10), "requeue should set value(next_valid_time_slot) to start."<< timeSeries.dump());
   }
   { // set calendar after time series start & before time series end, then reset

      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(11) ), Calendar::REAL);
      timeSeries.reset(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(11,0), "Reset should update free slot."<< timeSeries.dump());

      timeSeries.miss_next_time_slot();

      timeSeries.requeue(calendar);
      BOOST_CHECK_MESSAGE(timeSeries.value() == TimeSlot(11,10), "requeue should update to first time slot."<< timeSeries.dump());
   }
}


BOOST_AUTO_TEST_CASE( test_time_series_parsing )
{
   cout << "ACore:: ...test_time_series_parsing\n";

   BOOST_CHECK_MESSAGE(TimeSeries::create("00:30") == ecf::TimeSeries(0,30), "Error ");
   BOOST_CHECK_MESSAGE(TimeSeries::create("+00:30") == ecf::TimeSeries(0,30,true), "Error ");

   ecf::TimeSlot start(0,30);
   ecf::TimeSlot finish(21,03);
   ecf::TimeSlot incr(1,30);
   BOOST_CHECK_MESSAGE(TimeSeries::create("00:30 21:03 01:30") == ecf::TimeSeries(start,finish,incr), "Error");
   BOOST_CHECK_MESSAGE(TimeSeries::create("+00:30 21:03 01:30") == ecf::TimeSeries(start,finish,incr,true), "Error");
}

BOOST_AUTO_TEST_CASE( test_time_series_state_parsing )
{
   cout << "ACore:: ...test_time_series_state_parsing\n";

   /// extract string like
   ///     time +00:00 20:00 00:10 # this is a comment which will be ignored. index = 1
   ///     time +20:00        // index = 1
   ///     today 20:00        // index = 1
   ///     +00:00 20:00 00:10 // index = 0
   ///     +20:00             // index = 0
   /// will throw std:runtime_error for errors
   /// will assert if index >= lineTokens.size()
   {
      size_t index = 0;
      std::string the_time = "+00:00 20:00 00:10";
      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      BOOST_CHECK_MESSAGE(TimeSeries::create(index,lineTokens) == ecf::TimeSeries(TimeSlot(0,0),TimeSlot(20,0),TimeSlot(0,10),true), "Error");
   }
   {
      size_t index = 0;
      std::string the_time = "+10:10";
      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      BOOST_CHECK_MESSAGE(TimeSeries::create(index,lineTokens) == ecf::TimeSeries(TimeSlot(10,10),true), "Error");
   }
   {
      size_t index = 0;
      std::string the_time = "+10:10 # free isValid:false";
      ecf::TimeSeries expected(TimeSlot(10,10),true);
      expected.set_isValid(false);

      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      ecf::TimeSeries parsed_ts = TimeSeries::create(index,lineTokens,true);
      BOOST_CHECK_MESSAGE(parsed_ts == expected,
                          "Expected \n'" << expected.toString() << expected.state_to_string(false) << "'" <<
                          " But found \n'" << parsed_ts.toString() << parsed_ts.state_to_string(false) << "'");
   }
   {
      size_t index = 0;
      std::string the_time = "+10:10 # free isValid:false nextTimeSlot/10:10 relativeDuration/00:00:00";
      ecf::TimeSeries expected(TimeSlot(10,10),true);
      expected.set_isValid(false);

      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      ecf::TimeSeries parsed_ts = TimeSeries::create(index,lineTokens,true);
      BOOST_CHECK_MESSAGE(parsed_ts == expected,
                          "Expected \n'" << expected.toString() << expected.state_to_string(false) << "'" <<
                          " But found\n'" << parsed_ts.toString() << parsed_ts.state_to_string(false) << "'");
   }
   {
      // Update relative duration
      size_t index = 0;
      std::string the_time = "+10:10 # free isValid:false nextTimeSlot/10:10 relativeDuration/01:00:00";
      ecf::TimeSeries expected(TimeSlot(10,10),true);
      expected.set_isValid(false);

      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(0) ), Calendar::REAL);
      calendar.update( time_duration( hours(1) ) );
      expected.calendarChanged( calendar );

      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      ecf::TimeSeries parsed_ts = TimeSeries::create(index,lineTokens,true);
      BOOST_CHECK_MESSAGE(parsed_ts == expected,
                          "Expected \n'" << expected.toString() << expected.state_to_string(false) << "'" <<
                          " But found\n'" << parsed_ts.toString() << parsed_ts.state_to_string(false) << "'");
   }

   {
      // Update nextTimeSlot, create calendar at 09:00 and increment time series.
      std::string the_time = "09:00 12:00 01:00 # isValid:false nextTimeSlot/10:00 relativeDuration/00:00:00";
      ecf::TimeSeries expected(TimeSlot(9,0),TimeSlot(12,0),TimeSlot(1,0),false);
      expected.set_isValid(false);

      Calendar calendar;
      calendar.init(ptime(date(2008,10,8), hours(9) ), Calendar::REAL);
      expected.requeue( calendar ); // this will reset isValid to true
      expected.set_isValid(false);

      std::vector<std::string> lineTokens;
      Str::split(the_time,lineTokens);
      size_t index = 0;
      ecf::TimeSeries parsed_ts = TimeSeries::create(index,lineTokens,true);

      BOOST_CHECK_MESSAGE(parsed_ts == expected,
                          "Expected \n'" << expected.dump() << expected.state_to_string(false) << "'" <<
                          " But found\n'" << parsed_ts.dump() << parsed_ts.state_to_string(false) << "'");
   }
}

BOOST_AUTO_TEST_SUITE_END()

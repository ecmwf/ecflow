//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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
#include <string>
#include <iostream>
#include <fstream>

#include "Calendar.hpp"
#include "TimeSeries.hpp"
#include <boost/date_time/posix_time/time_formatters.hpp>

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_REAL_calendar )
{
	cout << "ACore:: ...test_REAL_calendar\n";

	// init the calendar to 2009, Feb, 10th,
 	boost::gregorian::date theDate(2009,2,10);
 	ptime time(theDate, hours(22) + minutes(10));

	Calendar calendar;
	calendar.init(time, Calendar::REAL);

	// record the time for the test
 	boost::posix_time::ptime theSuiteTime =  calendar.suiteTime();
 	boost::posix_time::time_duration theDuration =  calendar.duration();

    // Take time now and add 2 minutes, use this to update calendar by 2 minutes
	boost::posix_time::ptime time_now = Calendar::second_clock_time();
	time_now += minutes(2);
  	theSuiteTime += minutes(2);
 	theDuration += minutes(2);

  	calendar.update(time_now);

 	BOOST_CHECK_MESSAGE( calendar.suiteTime() == theSuiteTime," Expected " << to_simple_string(theSuiteTime) << " but found " << to_simple_string(calendar.suiteTime()) );
 	BOOST_CHECK_MESSAGE( calendar.duration() == theDuration," Expected " << to_simple_string(theDuration) << " but found " << to_simple_string(calendar.duration()) );

	time_now += hours(24);
  	theSuiteTime += hours(24);
 	theDuration += hours(24);

  	calendar.update(time_now);

 	BOOST_CHECK_MESSAGE( calendar.suiteTime() == theSuiteTime," Expected " << to_simple_string(theSuiteTime) << " but found " << to_simple_string(calendar.suiteTime()) );
 	BOOST_CHECK_MESSAGE( calendar.duration() == theDuration," Expected " << to_simple_string(theDuration) << " but found " << to_simple_string(calendar.duration()) );
}


BOOST_AUTO_TEST_CASE( test_REAL_calendar_time_series_relative_complex )
{
	cout << "ACore:: ...test_REAL_calendar_time_series_relative_complex\n";

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::HYBRID);

	// Create a test when we can match a time series
	// Create the time series: start  10:00
	//                         finish 20:00
	//                         incr   00:15
	TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(0,15),true/*relative*/);

 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

 	for(int hour=0; hour < 24; hour++) {
 		for( int minute=0; minute<60; minute++) {

			// Update calendar every hour, then see we can match time series, *RELATIVE* to suite start
 			time_now += minutes(1);

  		 	calendar.update(time_now);

 			timeSeries.calendarChanged( calendar );

			tm suiteTm = to_tm(calendar.suiteTime());

			bool matches = timeSeries.isFree(calendar);

			bool intersects = ( suiteTm.tm_hour >= timeSeries.start().hour() &&
						        suiteTm.tm_hour <= timeSeries.finish().hour() &&
	  				           (suiteTm.tm_min == 0 || suiteTm.tm_min % timeSeries.incr().minute() == 0)
					         );
			// Ovoid overshooting past end of series
			bool boundaryOk = true;
			if ( suiteTm.tm_hour == timeSeries.finish().hour() ) {
				boundaryOk = (suiteTm.tm_min <= timeSeries.finish().minute());
			}

 			if ( intersects && boundaryOk )
			{
				BOOST_CHECK_MESSAGE(matches,
				                    "Calendar should match relative time series at "
				                    << suiteTm.tm_hour <<":"<< suiteTm.tm_min
				                    << " suite time = " << to_simple_string(calendar.suiteTime()));
				if (!matches) {
 					cerr << "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
 					    << " timeSeries.start().hour() " << timeSeries.start().hour()
 					    << " timeSeries.start().minute() " << timeSeries.start().minute()
 						<< " timeSeries.finish().hour() " << timeSeries.finish().hour()
 						<< " timeSeries.finish().minute() " << timeSeries.finish().minute()
 						<< " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15
 						<< "\n";
 				}
			}
			else {
 				BOOST_CHECK_MESSAGE(!matches,
				                    "Calendar should NOT match relative time series at "
				                    << suiteTm.tm_hour <<":"<< suiteTm.tm_min
				                    << " suite time = " << to_simple_string(calendar.suiteTime()));

 				if (matches) {
 					cerr << "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
 					    << " timeSeries.start().hour() " << timeSeries.start().hour()
 					    << " timeSeries.start().minute() " << timeSeries.start().minute()
 						<< " timeSeries.finish().hour() " << timeSeries.finish().hour()
 						<< " timeSeries.finish().minute() " << timeSeries.finish().minute()
 						<< " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15
 						<< "\n";
 				}
			}
 		}
  	}
}

BOOST_AUTO_TEST_CASE( test_REAL_calendar_time_series)
{
	cout << "ACore:: ...test_REAL_calendar_time_series\n";

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);

	// Create a test when we can match a time series
	// Create the time series: start  10:00
	//                         finish 20:00
	//                         incr    1:00
	TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(1,0));

 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

	for(int hour=1; hour < 24; hour++) {
		// Update calendar every hour, then see we can match time series, in REAL
		// Update will set the local time from the computers system clock, however
		// for testing this will need to be overriden below.

		time_now += hours(1);

 		calendar.update(time_now);

		// cerr << "hour = " << hour << " suiteTime " << to_simple_string(calendar.suiteTime())  << "\n";
		if (hour >= timeSeries.start().hour() && hour <=timeSeries.finish().hour()) {
			BOOST_CHECK_MESSAGE(timeSeries.isFree(calendar),"Calendar should match time series at hour " << hour );
		}
		else {
			BOOST_CHECK_MESSAGE(!timeSeries.isFree(calendar),"Calendar should NOT match time series at hour " << hour );
		}
	}
}

BOOST_AUTO_TEST_CASE( test_REAL_calendar_time_series_complex )
{
	cout << "ACore:: ...test_REAL_calendar_time_series_complex\n";

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);

	// Create a test when we can match a time series
	// Create the time series: start  10:00
	//                         finish 20:00
	//                         incr   00:15
	TimeSeries timeSeries(TimeSlot(10,0), TimeSlot(20,0), TimeSlot(0,15));

 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

 	for(int hour=0; hour < 24; hour++) {
 		for( int minute=0; minute<60; minute++) {

			// Update calendar every minute, then see we can match time series, *RELATIVE* to suite start
 			time_now += minutes(1);
 			calendar.update(time_now);

			tm suiteTm = to_tm(calendar.suiteTime());

			bool matches = timeSeries.isFree(calendar);

			bool intersects = ( suiteTm.tm_hour >= timeSeries.start().hour() &&
						        suiteTm.tm_hour <= timeSeries.finish().hour() &&
	  				           (suiteTm.tm_min == 0 || suiteTm.tm_min % timeSeries.incr().minute() == 0)
					         );
			// Ovoid overshooting past end of series
			bool boundaryOk = true;
			if ( suiteTm.tm_hour == timeSeries.finish().hour() ) {
				boundaryOk = (suiteTm.tm_min <= timeSeries.finish().minute());
			}

 			if ( intersects && boundaryOk )
			{
				BOOST_CHECK_MESSAGE(matches,
				                    "Calendar should match relative time series at "
				                    << suiteTm.tm_hour <<":"<< suiteTm.tm_min
				                    << " suite time = " << to_simple_string(calendar.suiteTime()));
				if (!matches) {
 					cerr << "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
 					    << " timeSeries.start().hour() " << timeSeries.start().hour()
 					    << " timeSeries.start().minute() " << timeSeries.start().minute()
 						<< " timeSeries.finish().hour() " << timeSeries.finish().hour()
 						<< " timeSeries.finish().minute() " << timeSeries.finish().minute()
 						<< " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15
 						<< "\n";
 				}
			}
			else {
 				BOOST_CHECK_MESSAGE(!matches,
				                    "Calendar should NOT match relative time series at "
				                    << suiteTm.tm_hour <<":"<< suiteTm.tm_min
				                    << " suite time = " << to_simple_string(calendar.suiteTime()));

 				if (matches) {
 					cerr << "suiteTm.tm_hour =" << suiteTm.tm_hour << " suiteTm.tm_min = " << suiteTm.tm_min
 					    << " timeSeries.start().hour() " << timeSeries.start().hour()
 					    << " timeSeries.start().minute() " << timeSeries.start().minute()
 						<< " timeSeries.finish().hour() " << timeSeries.finish().hour()
 						<< " timeSeries.finish().minute() " << timeSeries.finish().minute()
 						<< " suiteTm.tm_min % 15 = " << suiteTm.tm_min % 15
 						<< "\n";
 				}
			}
 		}
  	}
}


BOOST_AUTO_TEST_CASE( test_REAL_calendar_hybrid_date )
{
	cout << "ACore:: ...test_REAL_calendar_hybrid_date\n";

	// The hybrid calendar should not change the suite date.
	// Test by updateing calendar by more than 24 hours

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::HYBRID);


	std::string expectedDate = "2010-Feb-10";
 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

	for(int hour=1; hour < 60; hour++) {
		// Update calendar every hour, for 60 hours
		// the date should be the same, i.e 2009, Feb, 10th

 		ptime timeBeforeUpdate = calendar.suiteTime();

		time_now += hours(1);

 		calendar.update(time_now);

		ptime timeAfterUpdate = calendar.suiteTime();

//		cerr << "hour = " << hour << " timeBeforeUpdate " << to_simple_string(timeBeforeUpdate)
//		    << "   timeAfterUpdate = " << to_simple_string(timeAfterUpdate) <<  "\n";

		if (hour != 24 && hour != 48) {
			time_period diff(timeBeforeUpdate,timeAfterUpdate);
 			time_duration gap = diff.length();
			BOOST_CHECK_MESSAGE( gap.hours() == 1,"Expected one hour difference but found " << gap.hours() << " at hour " << hour);
		}

		std::string actualDate = to_simple_string(calendar.suiteTime().date());
  		BOOST_CHECK_MESSAGE( actualDate == expectedDate,"Expected '" << expectedDate << "' but found " << actualDate << " at hour " << hour);
 	}
}

BOOST_AUTO_TEST_CASE( test_REAL_day_changed )
{
	cout << "ACore:: ...test_REAL_day_changed \n";

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)), Calendar::REAL);
 	BOOST_CHECK_MESSAGE(!calendar.hybrid(),"calendar type should be real");

 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

	for(int hour=1; hour < 73; hour++) {
		// Update calendar every hour, for 72 hours

		time_now += hours(1);

 	 	calendar.update(time_now);

		if (hour == 24 || hour == 48 || hour == 72) {
			BOOST_CHECK_MESSAGE( calendar.dayChanged(),"Expected day change at hour " << hour << " calendar " << calendar.toString());
 		}
		else {
			BOOST_CHECK_MESSAGE( !calendar.dayChanged(),"Un-Expected day change at hour " << hour << " calendar " << calendar.toString());
		}
 	}
}

BOOST_AUTO_TEST_CASE( test_REAL_day_changed_for_hybrid )
{
	cout << "ACore:: ...test_REAL_day_changed_for_hybrid\n";

	// init the calendar to 2009, Feb, 10th,  0 minutes past midnight
	Calendar calendar;
	calendar.init(ptime(date(2010,2,10), minutes(0)),Calendar::HYBRID);
 	BOOST_CHECK_MESSAGE(calendar.hybrid(),"calendar type should be real");

 	// HYBRID calendars allow for day change but not date.
 	std::string expected_date = to_simple_string(calendar.date());

 	boost::posix_time::ptime time_now = Calendar::second_clock_time();

	for(int hour=1; hour < 73; hour++) {
  		// Update calendar every hour, for 72 hours

		time_now += hours(1);
 	 	calendar.update(time_now);

		BOOST_CHECK_MESSAGE( expected_date == to_simple_string(calendar.date()) ,
		                     "Unexpected date change for hybrid calendar at hour " << hour);

 		// Day should change even for hybrid calendar,
 		if (hour == 24 || hour == 48 || hour == 72) {
 			BOOST_CHECK_MESSAGE( calendar.dayChanged(),"Expected day change at hour " << hour << " calendar " << calendar.toString());
 	 	}
 		else {
 			BOOST_CHECK_MESSAGE( !calendar.dayChanged(),"Un-Expected day change at hour " << hour << " calendar " << calendar.toString());
 		}
	}
}

BOOST_AUTO_TEST_SUITE_END()

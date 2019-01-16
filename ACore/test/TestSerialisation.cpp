/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include <boost/test/unit_test.hpp>
#include "TimeSeries.hpp"
#include "SerializationTest.hpp"
#include "Calendar.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

static std::string fileName = "test.txt";

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_calendar_serialisation )
{
	cout << "ACore:: ...test_calendar_serialisation \n";

 	Calendar cal;
	doSaveAndRestore(fileName,cal);
}

BOOST_AUTO_TEST_CASE( test_TimeSlot_serialisation )
{
	cout << "ACore:: ...test_TimeSlot_serialisation \n";

	{
		doSaveAndRestore<TimeSlot>(fileName);
	}

	{
		TimeSlot saved(1,1);
		doSaveAndRestore(fileName,saved);
	}

	{
		TimeSlot saved(99,59);
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_TimeSeries_serialisation )
{
	cout << "ACore:: ...test_TimeSeries_serialisation \n";

	{
		doSaveAndRestore<TimeSeries>(fileName);
	}
	{
		TimeSeries saved = TimeSeries(TimeSlot(10,10));
		doSaveAndRestore(fileName,saved);
	}
	{
		TimeSeries saved = TimeSeries(TimeSlot(0,0),TimeSlot(10,10),TimeSlot(0,10));
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_SUITE_END()

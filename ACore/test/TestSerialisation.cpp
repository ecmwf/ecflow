/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include "boost_archive.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

static std::string fileName = "test.txt";

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_archive_version )
{
   cout << "ACore:: ...test_archive_version:  ";
   // Boost 1.47 archive version = 9
   // Bosst 1.53 archive version = 10
   BOOST_CHECK_MESSAGE(boost_archive::version() >= 9,"Expected boost archive version >= 9");
   std::cout << boost_archive::version() << "\n";
}

BOOST_AUTO_TEST_CASE( test_extract_archive_version )
{
   cout << "ACore:: ...test_extract_archive_version\n";

   // get the archive version:
   std::string boost_serial_str = "22 serialization::archive 9 0 0 0 0 1 1 0";
   BOOST_CHECK_MESSAGE(boost_archive::extract_version(boost_serial_str) == 9,"Expected version to be 9 but found " << boost_archive::extract_version(boost_serial_str));

   boost_serial_str = "22 serialization::archive 10 0 0 0 0 1 1 0";
   BOOST_CHECK_MESSAGE(boost_archive::extract_version(boost_serial_str) == 10,"Expected version to be 10 but found " << boost_archive::extract_version(boost_serial_str));

   boost_serial_str = "22 serialization::archive 999 0 0 0 0 1 1 0";
   BOOST_CHECK_MESSAGE(boost_archive::extract_version(boost_serial_str) == 999,"Expected version to be 999 but found " << boost_archive::extract_version(boost_serial_str));

   // error
   boost_serial_str = "22 serialization::archive";
   BOOST_CHECK_MESSAGE(boost_archive::extract_version(boost_serial_str) == 0,"Expected version to be 0 but found " << boost_archive::extract_version(boost_serial_str));
}


BOOST_AUTO_TEST_CASE( test_replace_archive_version )
{
   cout << "ACore:: ...test_replace_archive_version\n";

   std::string boost_serial_str = "22 serialization::archive 9 0 0 0 0 1 1 0";
   std::string expected = "22 serialization::archive 10 0 0 0 0 1 1 0";
   BOOST_CHECK(boost_archive::replace_version(boost_serial_str,10));
   BOOST_CHECK_MESSAGE(boost_serial_str == expected,"Expected '" << expected << "' but found '" << boost_serial_str << "'");

   boost_serial_str = "22 serialization::archive 10 0 0 0 0 1 1 0";
   expected         = "22 serialization::archive 9 0 0 0 0 1 1 0";
   BOOST_CHECK(boost_archive::replace_version(boost_serial_str,9));
   BOOST_CHECK_MESSAGE(boost_serial_str == expected,"Expected '" << expected << "' but found '" << boost_serial_str << "'");

   boost_serial_str = "22 serialization::archive 10 10 10";
   expected         = "22 serialization::archive 11 10 10";
   BOOST_CHECK(boost_archive::replace_version(boost_serial_str,11));
   BOOST_CHECK_MESSAGE(boost_serial_str == expected,"Expected '" << expected << "' but found '" << boost_serial_str << "'");

   boost_serial_str = "22 serialization::archive 10 10 10";
   expected         = "22 serialization::archive 44444 10 10";
   BOOST_CHECK(boost_archive::replace_version(boost_serial_str,44444));
   BOOST_CHECK_MESSAGE(boost_serial_str == expected,"Expected '" << expected << "' but found '" << boost_serial_str << "'");

   // error
   boost_serial_str = "22 serialization::archive ";
   expected         = "22 serialization::archive ";
   BOOST_CHECK(!boost_archive::replace_version(boost_serial_str,11));
   BOOST_CHECK_MESSAGE(boost_serial_str == expected,"Expected '" << expected << "' but found '" << boost_serial_str << "'");
}

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

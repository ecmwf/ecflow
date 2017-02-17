#if defined(TEXT_ARCHIVE) || !defined(BINARY_ARCHIVE) && !defined(PORTABLE_BINARY_ARCHIVE) && !defined(EOS_PORTABLE_BINARY_ARCHIVE)
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "TimeSeries.hpp"
#include "SerializationTest.hpp"
#include "Calendar.hpp"
#include "File.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

// If you are updating the tests, *MAKE SURE* to check out test/data/migration/* files
//#define UPDATE_TESTS 1

BOOST_AUTO_TEST_CASE( test_migration_restore_1_9 )
{
   cout << "ACore:: ...test_migration_restore_1_9\n";

   std::string file_name = File::test_data("ACore/test/data/migration/","ACore");

   // Note: default calendar constructor will init with current time: Hence set for comparison
   Calendar calendar;
   boost::gregorian::date theDate(2011,2,10);
   ptime time(theDate, hours(23) + minutes(59));
   calendar.init(time, Calendar::REAL);  // Calendar type is derived from the clock attribute & hence is not persisted
   Ecf::set_debug_equality(true);

#ifdef UPDATE_TESTS
   doSave<TimeSlot>(file_name + "timeslot_default_constructor_v1.9");
   doSave<TimeSeries>(file_name + "timeseries_default_constructor_v1.9");
   doSave<Calendar>(file_name + "calendar_v1.9",calendar);
   doSave(file_name + "timeslot_1_1_v1_9",TimeSlot(1,1));
   doSave(file_name + "timeslot_99_59_v1_9",TimeSlot(99,59));
   doSave(file_name + "timeseries_10_10_v1_9",TimeSeries(TimeSlot(10,10)));
#else
   do_restore<TimeSlot>(file_name + "timeslot_default_constructor_v1.9",TimeSlot());
   do_restore<TimeSeries>(file_name + "timeseries_default_constructor_v1.9",TimeSeries());
   do_restore<Calendar>(file_name + "calendar_v1.9",calendar);
   do_restore<TimeSlot>(file_name + "timeslot_1_1_v1_9",TimeSlot(1,1));
   do_restore<TimeSlot>(file_name + "timeslot_99_59_v1_9",TimeSlot(99,59));
   do_restore<TimeSeries>(file_name + "timeseries_10_10_v1_9",TimeSeries(TimeSlot(10,10)));
#endif
   Ecf::set_debug_equality(false);
}

BOOST_AUTO_TEST_SUITE_END()

#endif

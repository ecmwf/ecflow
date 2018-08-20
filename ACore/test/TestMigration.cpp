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
#include "Calendar.hpp"
#include "DState.hpp"
#include "NState.hpp"
#include "File.hpp"
#include "Ecf.hpp"
#include "SerializationTest.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

// If you are updating the tests, *MAKE SURE* to check out test/data/migration/* files
//#define UPDATE_TESTS 1

BOOST_AUTO_TEST_CASE( test_migration_restore_cereal )
{
   cout << "ACore:: ...test_migration_restore_cereal\n";

   std::string file_name = File::test_data("ACore/test/data/migration/","ACore");

   // Note: default calendar constructor will init with current time: Hence set for comparison
   Calendar calendar;
   boost::gregorian::date theDate(2011,2,10);
   ptime time(theDate, hours(23) + minutes(59));
   calendar.init(time, Calendar::REAL);  // Calendar type is derived from the clock attribute & hence is not persisted

   DebugEquality debug_equality; // only as affect in DEBUG build

   string cereal_version = "_1_2_2_";

#ifdef UPDATE_TESTS
   doSave<TimeSlot>(file_name + "timeslot_default_constructor" + cereal_version);
   doSave<TimeSeries>(file_name + "timeseries_default_constructor" + cereal_version);
   doSave<Calendar>(file_name + "calendar" + cereal_version,calendar);
   doSave(file_name + "timeslot"  + cereal_version + "11",TimeSlot(1,1));
   doSave(file_name + "timeslot"  + cereal_version + "9959",TimeSlot(99,59));
   doSave(file_name + "timeseries" + cereal_version + "1010",TimeSeries(TimeSlot(10,10)));
   doSave(file_name + "dstate" + cereal_version,DState());
   doSave(file_name + "nstate" + cereal_version,NState());
#else
   do_restore<TimeSlot>(file_name   + "timeslot_default_constructor" + cereal_version,TimeSlot());
   do_restore<TimeSeries>(file_name + "timeseries_default_constructor" + cereal_version ,TimeSeries());
   do_restore<Calendar>(file_name   + "calendar"  + cereal_version,          calendar);
   do_restore<TimeSlot>(file_name   + "timeslot"  + cereal_version + "11",   TimeSlot(1,1));
   do_restore<TimeSlot>(file_name   + "timeslot"  + cereal_version + "9959", TimeSlot(99,59));
   do_restore<TimeSeries>(file_name + "timeseries" + cereal_version + "1010",TimeSeries(TimeSlot(10,10)));
   do_restore<DState>(file_name + "dstate" + cereal_version,DState());
   do_restore<NState>(file_name + "nstate" + cereal_version,NState());
#endif
}

BOOST_AUTO_TEST_SUITE_END()

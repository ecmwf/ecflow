#if defined(TEXT_ARCHIVE) || !defined(BINARY_ARCHIVE) && !defined(PORTABLE_BINARY_ARCHIVE) && !defined(EOS_PORTABLE_BINARY_ARCHIVE)
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/test/unit_test.hpp>

#include "TodayAttr.hpp"
#include "TimeAttr.hpp"
#include "VerifyAttr.hpp"
#include "RepeatAttr.hpp"
#include "LateAttr.hpp"
#include "DayAttr.hpp"
#include "DateAttr.hpp"
#include "CronAttr.hpp"
#include "ClockAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "NodeAttr.hpp"
#include "Variable.hpp"
#include "ZombieAttr.hpp"
#include "Calendar.hpp"
#include "SerializationTest.hpp"
#include "TimeSeries.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;
namespace fs = boost::filesystem;

//#define UPDATE_TESTS 1


BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

// These test are used for future release. They help to ensure that we have
// backward compatibility.i.e future release can open file, created by an earlier release
//
BOOST_AUTO_TEST_CASE( test_migration_restore_def_con )
{
   cout << "ANattr:: ...test_migration_restore_def_con\n";

   std::string file_name = File::test_data("ANattr/test/data/migration/default_constructor_v1_9/","ANattr");

   // Create migration data
#ifdef UPDATE_TESTS
   doSave(file_name + "VerifyAttr",VerifyAttr());
   doSave(file_name + "TodayAttr",TodayAttr());
   doSave(file_name + "TimeAttr",TimeAttr());
   doSave(file_name + "RepeatDate",RepeatDate());
   doSave(file_name + "RepeatInteger",RepeatInteger());
   doSave(file_name + "RepeatEnumerated",RepeatEnumerated());
   doSave(file_name + "RepeatString",RepeatString());
   doSave(file_name + "LateAttr",LateAttr());
   doSave(file_name + "DayAttr",DayAttr());
   doSave(file_name + "DateAttr",DateAttr());
   doSave(file_name + "CronAttr",CronAttr());
   doSave(file_name + "ClockAttr",ClockAttr());
   doSave(file_name + "AutoCancelAttr",AutoCancelAttr());
   doSave(file_name + "Label",Label());
   doSave(file_name + "Variable",Variable());
   doSave(file_name + "Event",Event());
   doSave(file_name + "Meter",Meter());
   doSave(file_name + "ZombieAttr",ZombieAttr());
#endif

   do_restore<VerifyAttr>(file_name + "VerifyAttr",VerifyAttr());
   do_restore<TodayAttr>(file_name + "TodayAttr",TodayAttr());
   do_restore<TimeAttr>(file_name + "TimeAttr",TimeAttr());
   do_restore<RepeatDate>(file_name + "RepeatDate",RepeatDate());
   do_restore<RepeatInteger>(file_name + "RepeatInteger",RepeatInteger());
   do_restore<RepeatEnumerated>(file_name + "RepeatEnumerated",RepeatEnumerated());
   do_restore<RepeatString>(file_name + "RepeatString",RepeatString());
   do_restore<LateAttr>(file_name + "LateAttr",LateAttr());
   do_restore<DayAttr>(file_name + "DayAttr",DayAttr());
   do_restore<DateAttr>(file_name + "DateAttr",DateAttr());
   do_restore<CronAttr>(file_name + "CronAttr",CronAttr());
   do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr",AutoCancelAttr());
   do_restore<Label>(file_name + "Label",Label());
   do_restore<Variable>(file_name + "Variable",Variable());
   do_restore<Event>(file_name + "Event",Event());
   do_restore<Meter>(file_name + "Meter",Meter());
   do_restore<ZombieAttr>(file_name + "ZombieAttr",ZombieAttr());
}

BOOST_AUTO_TEST_CASE( test_migration_restore )
{
   cout << "ANattr:: ...test_migration_restore\n";

   std::string file_name = File::test_data("ANattr/test/data/migration/v1_9/","ANattr");

   std::vector<std::string> theVec; theVec.push_back("a");  theVec.push_back("b");
   LateAttr lateattr;
   lateattr.addSubmitted(TimeSlot(10,12));
   lateattr.addActive(TimeSlot(10,12));
   lateattr.addComplete(TimeSlot(10,12),true);

   CronAttr cron_attr;
   std::vector<int> weekDays; weekDays.push_back(1); weekDays.push_back(2);
   std::vector<int> daysOfMonth; daysOfMonth.push_back(1); daysOfMonth.push_back(2);
   std::vector<int> months; months.push_back(1); months.push_back(2);
   cron_attr.addWeekDays(weekDays);
   cron_attr.addDaysOfMonth(daysOfMonth);
   cron_attr.addMonths(months);
   cron_attr.addTimeSeries(TimeSlot(0,0),TimeSlot(20,0),TimeSlot(0,1));

   ClockAttr clock_attr(false);
   clock_attr.date(1,1,2009);
   clock_attr.set_gain_in_seconds(3600);
   clock_attr.startStopWithServer(true);

   std::vector<ecf::Child::CmdType> child_cmds;
   child_cmds.push_back(ecf::Child::INIT);
   child_cmds.push_back(ecf::Child::EVENT);
   child_cmds.push_back(ecf::Child::METER);
   child_cmds.push_back(ecf::Child::LABEL);
   child_cmds.push_back(ecf::Child::WAIT);
   child_cmds.push_back(ecf::Child::ABORT);
   child_cmds.push_back(ecf::Child::COMPLETE);

   Label label("name","value");
   label.set_new_value("new_value");

#ifdef UPDATE_TESTS
   // Create migration data
   doSave(file_name + "VerifyAttr",VerifyAttr(NState::COMPLETE,3));
   doSave(file_name + "TodayAttr",TodayAttr(10,12));
   doSave(file_name + "TimeAttr",TimeAttr(10,12));
   doSave(file_name + "RepeatDate",RepeatDate("date",20110112,20110115));
   doSave(file_name + "RepeatInteger",RepeatInteger("integer",0,100,2));
   doSave(file_name + "RepeatEnumerated",RepeatEnumerated("enum",theVec));
   doSave(file_name + "RepeatString",RepeatString("string",theVec));
   doSave(file_name + "LateAttr",lateattr);
   doSave(file_name + "DayAttr",DayAttr(DayAttr::MONDAY));
   doSave(file_name + "DateAttr",DateAttr(12,12,2012));
   doSave(file_name + "CronAttr",cron_attr);
   doSave(file_name + "ClockAttr",clock_attr);
   doSave(file_name + "AutoCancelAttr",AutoCancelAttr(100));
   doSave(file_name + "AutoCancelAttr_1",AutoCancelAttr(TimeSlot(10,12),true));
   doSave(file_name + "Label",label);
//   doSave(file_name + "Limit",limit);
   doSave(file_name + "Variable",Variable("var_name","var_value"));
   doSave(file_name + "Event_1",Event(1));
   doSave(file_name + "Event_2",Event("event"));
   doSave(file_name + "Meter",Meter("meter",10,100,100));
   doSave(file_name + "ZombieAttr",ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB));
   doSave(file_name + "ZombieAttr1",ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB,500));
#endif

   do_restore<VerifyAttr>(file_name + "VerifyAttr",VerifyAttr(NState::COMPLETE,3));
   do_restore<TodayAttr>(file_name + "TodayAttr",TodayAttr(10,12));
   do_restore<TimeAttr>(file_name + "TimeAttr",TimeAttr(10,12));
   do_restore<RepeatDate>(file_name + "RepeatDate",RepeatDate("date",20110112,20110115));
   do_restore<RepeatInteger>(file_name + "RepeatInteger",RepeatInteger("integer",0,100,2));
   do_restore<RepeatEnumerated>(file_name + "RepeatEnumerated",RepeatEnumerated("enum",theVec));
   do_restore<RepeatString>(file_name + "RepeatString",RepeatString("string",theVec));
   do_restore<LateAttr>(file_name + "LateAttr",lateattr);
   do_restore<DayAttr>(file_name + "DayAttr",DayAttr(DayAttr::MONDAY));
   do_restore<DateAttr>(file_name + "DateAttr",DateAttr(12,12,2012));
   do_restore<CronAttr>(file_name + "CronAttr",cron_attr);
   do_restore<ClockAttr>(file_name + "ClockAttr",clock_attr);
   do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr",AutoCancelAttr(100));
   do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr_1",AutoCancelAttr(TimeSlot(10,12),true));
   do_restore<Label>(file_name + "Label",label);
   do_restore<Variable>(file_name + "Variable",Variable("var_name","var_value"));
   do_restore<Event>(file_name + "Event_1",Event(1));
   do_restore<Event>(file_name + "Event_2",Event("event"));
   do_restore<Meter>(file_name + "Meter",Meter("meter",10,100,100));
   do_restore<ZombieAttr>(file_name + "ZombieAttr",ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB));
   do_restore<ZombieAttr>(file_name + "ZombieAttr1",ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB,500));
}

BOOST_AUTO_TEST_SUITE_END()

#endif

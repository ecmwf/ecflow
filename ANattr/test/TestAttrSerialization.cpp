#define BOOST_TEST_MODULE TestANattr

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
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
#include <boost/make_shared.hpp>

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
#include "QueueAttr.hpp"
#include "Calendar.hpp"
#include "SerializationTest.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

// Globals used throughout the test
static std::string fileName = "test.txt";

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_AttrDefaultConstructor_serialisation )
{
	cout << "ANattr:: ...test_AttrDefaultConstructor_serialisation \n";

	doSaveAndRestore<VerifyAttr>(fileName);
	doSaveAndRestore<TodayAttr>(fileName);
	doSaveAndRestore<TimeAttr>(fileName);
	doSaveAndRestore<RepeatDate>(fileName);
	doSaveAndRestore<RepeatInteger>(fileName);
	doSaveAndRestore<RepeatEnumerated>(fileName);
	doSaveAndRestore<RepeatString>(fileName);
	doSaveAndRestore<LateAttr>(fileName);
	doSaveAndRestore<DayAttr>(fileName);
	doSaveAndRestore<DateAttr>(fileName);
	doSaveAndRestore<CronAttr>(fileName);
	doSaveAndRestore<ClockAttr>(fileName);
	doSaveAndRestore<AutoCancelAttr>(fileName);
	doSaveAndRestore<Label>(fileName);
	doSaveAndRestore<Variable>(fileName);
	doSaveAndRestore<Event>(fileName);
   doSaveAndRestore<Meter>(fileName);
   doSaveAndRestore<ZombieAttr>(fileName);
   doSaveAndRestore<QueueAttr>(fileName);
}

BOOST_AUTO_TEST_CASE( test_VerifyAttr_serialisation )
{
	cout << "ANattr:: ...test_VerifyAttr_serialisation \n";
 	VerifyAttr saved(NState::COMPLETE,10);
	doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_TodayAttr_serialisation )
{
	cout << "ANattr:: ...test_TodayAttr_serialisation \n";
  	{
		TodayAttr saved(TimeSlot(10,12));
		doSaveAndRestore(fileName,saved);
	}
	{
		TodayAttr saved(TimeSlot(0,0),TimeSlot(10,12),TimeSlot(1,0));
 		doSaveAndRestore(fileName,saved);
	}
	{
		TodayAttr saved(TimeSeries(TimeSlot(10,12)));
 		doSaveAndRestore(fileName,saved);
	}
	{
		TodayAttr saved(TimeSeries(TimeSlot(10,12)));
 		doSaveAndRestore(fileName,saved);
	}
}


BOOST_AUTO_TEST_CASE( test_TimeAttr_serialisation )
{
	cout << "ANattr:: ...test_TimeAttr_serialisation \n";
  	{
		TimeAttr saved(TimeSlot(10,12));
 		doSaveAndRestore(fileName,saved);
	}
	{
		TimeAttr saved(TimeSlot(0,0),TimeSlot(10,12),TimeSlot(1,0));
 		doSaveAndRestore(fileName,saved);
	}
	{
		TimeAttr saved(TimeSeries(TimeSlot(10,12)));
 		doSaveAndRestore(fileName,saved);
	}
}


BOOST_AUTO_TEST_CASE( test_RepeatAttr_serialisation )
{
	cout << "ANattr:: ...test_RepeatAttr_serialisation \n";
	{
		RepeatDate saved("varname",20101210,20101230,3);
		doSaveAndRestore(fileName,saved);
	}
	{
		RepeatInteger saved("varname",0,10,1);
		doSaveAndRestore(fileName,saved);
	}
	{
		std::vector<std::string> theVec; theVec.push_back("a");  theVec.push_back("b");
		RepeatEnumerated saved = RepeatEnumerated("varname",theVec);
		doSaveAndRestore(fileName,saved);
	}
	{
		std::vector<std::string> theVec; theVec.push_back("a");
		RepeatString saved = RepeatString("varname",theVec);
		doSaveAndRestore(fileName,saved);
	}


	{
		Repeat saved(RepeatDate("varname",20101210,20101230,3));
		doSaveAndRestore(fileName,saved);
	}
	{
		Repeat saved(RepeatInteger("varname",0,10,1));
		doSaveAndRestore(fileName,saved);
	}
	{
		std::vector<std::string> theVec; theVec.push_back("a");  theVec.push_back("b");
		Repeat saved(RepeatEnumerated("varname",theVec));
		doSaveAndRestore(fileName,saved);
	}
	{
		std::vector<std::string> theVec; theVec.push_back("a");
		Repeat saved(RepeatString("varname",theVec));
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_LateAttr_serialisation )
{
	cout << "ANattr:: ...test_LateAttr_serialisation \n";
	LateAttr saved;
	saved.addSubmitted(TimeSlot(10,12));
	saved.addActive(TimeSlot(10,12));
	saved.addComplete(TimeSlot(10,12),true);
	doSaveAndRestore(fileName,saved);

}

BOOST_AUTO_TEST_CASE( test_DayAttr_serialisation )
{
 	cout << "ANattr:: ...test_DayAttr_serialisation\n";
 	std::vector< DayAttr::Day_t > dvec;
	dvec.push_back(DayAttr::SUNDAY);
	dvec.push_back(DayAttr::MONDAY);
	dvec.push_back(DayAttr::TUESDAY);
	dvec.push_back(DayAttr::WEDNESDAY);
	dvec.push_back(DayAttr::THURSDAY);
	dvec.push_back(DayAttr::FRIDAY);
	dvec.push_back(DayAttr::SATURDAY);
	for(size_t d = 0; d < dvec.size(); d++) {
		DayAttr saved(dvec[d]);
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_DateAttr_serialisation )
{
 	cout << "ANattr:: ...test_DateAttr_serialisation\n";
 	DateAttr saved(1,1,2010);
	doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_CronAttr_serialisation )
{
 	cout << "ANattr:: ...test_CronAttr_serialisation\n";
 	CronAttr saved;
 	std::vector<int> weekDays; weekDays.push_back(1); weekDays.push_back(2);
 	std::vector<int> daysOfMonth; daysOfMonth.push_back(1); daysOfMonth.push_back(2);
 	std::vector<int> months; months.push_back(1); months.push_back(2);
 	saved.addWeekDays(weekDays);
 	saved.addDaysOfMonth(daysOfMonth);
 	saved.addMonths(months);
 	saved.addTimeSeries(TimeSlot(0,0),TimeSlot(20,0),TimeSlot(0,1));

 	doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_ClockAttr_serialisation )
{
 	cout << "ANattr:: ...test_ClockAttr_serialisation\n";
 	{
		ClockAttr saved(false);
		saved.date(1,1,2009);
		saved.set_gain_in_seconds(3600);
		saved.startStopWithServer(true);

		doSaveAndRestore(fileName,saved);
	}
	{
		ClockAttr saved(Calendar::second_clock_time());
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_AutoCancelAttr_serialisation )
{
 	cout << "ANattr:: ...test_AutoCancelAttr_serialisation\n";
 	{
		AutoCancelAttr saved(100);
		doSaveAndRestore(fileName,saved);
	}
	{
		AutoCancelAttr saved( TimeSlot(12,10), true) ;
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_Label_serialisation )
{
 	cout << "ANattr:: ...test_Label_serialisation\n";
 	{
		Label saved("labelName","some text");
		doSaveAndRestore(fileName,saved);
	}
}


BOOST_AUTO_TEST_CASE( test_Variable_serialisation )
{
 	cout << "ANattr:: ...test_Variable_serialisation\n";
 	Variable saved("varname","var value 123 12 =");
	doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_Event_serialisation )
{
 	cout << "ANattr:: ...test_Event_serialisation\n";
 	{
		Event saved(3);
		doSaveAndRestore(fileName,saved);
	}
	{
		Event saved(10+1,"event_name");
		doSaveAndRestore(fileName,saved);
	}
}

BOOST_AUTO_TEST_CASE( test_Meter_serialisation )
{
 	cout << "ANattr:: ...test_Meter_serialisation\n";
	Meter saved("meter",0,20,20);
	doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_queue_serialisation )
{
   cout << "ANattr:: ...test_queue_serialisation\n";
   std::vector<std::string> queue_items; queue_items.push_back("a"); queue_items.push_back("b");
   QueueAttr saved("queue",queue_items);
   doSaveAndRestore(fileName,saved);
}

BOOST_AUTO_TEST_CASE( test_zombie_attr_serialisation )
{
   cout << "ANattr:: ...test_zombie_attr_serialisation\n";

   std::vector<ecf::Child::CmdType> child_cmds;
   child_cmds.push_back(ecf::Child::INIT);
   child_cmds.push_back(ecf::Child::EVENT);
   child_cmds.push_back(ecf::Child::METER);
   child_cmds.push_back(ecf::Child::LABEL);
   child_cmds.push_back(ecf::Child::WAIT);
   child_cmds.push_back(ecf::Child::QUEUE);
   child_cmds.push_back(ecf::Child::ABORT);
   child_cmds.push_back(ecf::Child::COMPLETE);

   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB,10));
   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::PATH, child_cmds, ecf::User::FAIL,10));
   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::ECF, child_cmds, ecf::User::BLOCK,10));
   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::ECF_PID, child_cmds, ecf::User::REMOVE,10));
   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::ECF_PID_PASSWD, child_cmds, ecf::User::KILL,10));
   doSaveAndRestore(fileName,ZombieAttr(ecf::Child::ECF_PASSWD, child_cmds, ecf::User::ADOPT,10));
}

BOOST_AUTO_TEST_SUITE_END()

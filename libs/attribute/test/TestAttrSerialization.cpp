/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/ClockAttr.hpp"
#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/attribute/GenericAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/attribute/TimeAttr.hpp"
#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

// Globals used throughout the test
static std::string fileName = "test.txt";

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_AttrSerialization)

BOOST_AUTO_TEST_CASE(test_AttrDefaultConstructor_serialisation) {
    ECF_NAME_THIS_TEST();

    doSaveAndRestore<VerifyAttr>(fileName);
    doSaveAndRestore<TodayAttr>(fileName);
    doSaveAndRestore<TimeAttr>(fileName);
    doSaveAndRestore<RepeatDate>(fileName);
    doSaveAndRestore<RepeatDateList>(fileName);
    doSaveAndRestore<RepeatInteger>(fileName);
    doSaveAndRestore<RepeatEnumerated>(fileName);
    doSaveAndRestore<RepeatString>(fileName);
    doSaveAndRestore<LateAttr>(fileName);
    doSaveAndRestore<DayAttr>(fileName);
    doSaveAndRestore<DateAttr>(fileName);
    doSaveAndRestore<CronAttr>(fileName);
    doSaveAndRestore<ClockAttr>(fileName);
    doSaveAndRestore<AutoCancelAttr>(fileName);
    doSaveAndRestore<AutoArchiveAttr>(fileName);
    doSaveAndRestore<Label>(fileName);
    doSaveAndRestore<Variable>(fileName);
    doSaveAndRestore<Event>(fileName);
    doSaveAndRestore<Meter>(fileName);
    doSaveAndRestore<ZombieAttr>(fileName);
    doSaveAndRestore<QueueAttr>(fileName);
    doSaveAndRestore<GenericAttr>(fileName);
}

BOOST_AUTO_TEST_CASE(test_VerifyAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    VerifyAttr saved(NState::COMPLETE, 10);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_TodayAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        TodayAttr saved(TimeSlot(10, 12));
        doSaveAndRestore(fileName, saved);
    }
    {
        TodayAttr saved(TimeSlot(0, 0), TimeSlot(10, 12), TimeSlot(1, 0));
        doSaveAndRestore(fileName, saved);
    }
    {
        TodayAttr saved(TimeSeries(TimeSlot(10, 12)));
        doSaveAndRestore(fileName, saved);
    }
    {
        TodayAttr saved(TimeSeries(TimeSlot(10, 12)));
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_TimeAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        TimeAttr saved(TimeSlot(10, 12));
        doSaveAndRestore(fileName, saved);
    }
    {
        TimeAttr saved(TimeSlot(0, 0), TimeSlot(10, 12), TimeSlot(1, 0));
        doSaveAndRestore(fileName, saved);
    }
    {
        TimeAttr saved(TimeSeries(TimeSlot(10, 12)));
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_RepeatAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        RepeatDate saved("varname", 20101210, 20101230, 3);
        doSaveAndRestore(fileName, saved);
    }
    {
        RepeatDateList saved("varname", {19990101, 19990103});
        doSaveAndRestore(fileName, saved);
    }
    {
        RepeatInteger saved("varname", 0, 10, 1);
        doSaveAndRestore(fileName, saved);
    }
    {
        std::vector<std::string> theVec;
        theVec.emplace_back("a");
        theVec.emplace_back("b");
        RepeatEnumerated saved = RepeatEnumerated("varname", theVec);
        doSaveAndRestore(fileName, saved);
    }
    {
        std::vector<std::string> theVec;
        theVec.emplace_back("a");
        RepeatString saved = RepeatString("varname", theVec);
        doSaveAndRestore(fileName, saved);
    }

    {
        Repeat saved(RepeatDate("varname", 20101210, 20101230, 3));
        doSaveAndRestore(fileName, saved);
    }
    {
        Repeat saved(RepeatDateList("varname", {19990101, 19990103}));
        doSaveAndRestore(fileName, saved);
    }
    {
        Repeat saved(RepeatInteger("varname", 0, 10, 1));
        doSaveAndRestore(fileName, saved);
    }
    {
        std::vector<std::string> theVec;
        theVec.emplace_back("a");
        theVec.emplace_back("b");
        Repeat saved(RepeatEnumerated("varname", theVec));
        doSaveAndRestore(fileName, saved);
    }
    {
        std::vector<std::string> theVec;
        theVec.emplace_back("a");
        Repeat saved(RepeatString("varname", theVec));
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_LateAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    LateAttr saved;
    saved.addSubmitted(TimeSlot(10, 12));
    saved.addActive(TimeSlot(10, 12));
    saved.addComplete(TimeSlot(10, 12), true);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_DayAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    std::vector<DayAttr::Day_t> dvec;
    dvec.push_back(DayAttr::SUNDAY);
    dvec.push_back(DayAttr::MONDAY);
    dvec.push_back(DayAttr::TUESDAY);
    dvec.push_back(DayAttr::WEDNESDAY);
    dvec.push_back(DayAttr::THURSDAY);
    dvec.push_back(DayAttr::FRIDAY);
    dvec.push_back(DayAttr::SATURDAY);
    for (auto& d : dvec) {
        DayAttr saved(d);
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_DateAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    DateAttr saved(1, 1, 2010);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_CronAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    CronAttr saved;
    std::vector<int> weekDays;
    weekDays.push_back(1);
    weekDays.push_back(2);
    std::vector<int> daysOfMonth;
    daysOfMonth.push_back(1);
    daysOfMonth.push_back(2);
    std::vector<int> months;
    months.push_back(1);
    months.push_back(2);
    saved.addWeekDays(weekDays);
    saved.addDaysOfMonth(daysOfMonth);
    saved.addMonths(months);
    saved.addTimeSeries(TimeSlot(0, 0), TimeSlot(20, 0), TimeSlot(0, 1));

    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_ClockAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        ClockAttr saved(false);
        saved.date(1, 1, 2009);
        saved.set_gain_in_seconds(3600);

        doSaveAndRestore(fileName, saved);
    }
    {
        ClockAttr saved(Calendar::second_clock_time());
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_AutoCancelAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        AutoCancelAttr saved(100);
        doSaveAndRestore(fileName, saved);
    }
    {
        AutoCancelAttr saved(TimeSlot(12, 10), true);
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_AutoArchiveAttr_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        AutoArchiveAttr saved(100);
        doSaveAndRestore(fileName, saved);
    }
    {
        AutoArchiveAttr saved(100, true /*idle*/);
        doSaveAndRestore(fileName, saved);
    }
    {
        AutoArchiveAttr saved(TimeSlot(12, 10), true, false /*idle*/);
        doSaveAndRestore(fileName, saved);
    }
    {
        AutoArchiveAttr saved(TimeSlot(12, 10), true, true /*idle*/);
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_Label_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        Label saved("labelName", "some text");
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_Variable_serialisation) {
    ECF_NAME_THIS_TEST();

    Variable saved("varname", "var value 123 12 =");
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_Event_serialisation) {
    ECF_NAME_THIS_TEST();

    {
        Event saved(3);
        doSaveAndRestore(fileName, saved);
    }
    {
        Event saved(10 + 1, "event_name");
        doSaveAndRestore(fileName, saved);
    }
    {
        Event saved(12, "event_name", true /*init value*/);
        doSaveAndRestore(fileName, saved);
    }
    {
        Event saved(12, "event_name", false /*init value*/);
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_Meter_serialisation) {
    ECF_NAME_THIS_TEST();

    Meter saved("meter", 0, 20, 20);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_queue_serialisation) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> queue_items;
    queue_items.emplace_back("a");
    queue_items.emplace_back("b");
    QueueAttr saved("queue", queue_items);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_generic_serialisation) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> queue_items;
    queue_items.emplace_back("a");
    queue_items.emplace_back("b");
    GenericAttr saved("gen1", queue_items);
    doSaveAndRestore(fileName, saved);
}

BOOST_AUTO_TEST_CASE(test_zombie_attr_serialisation) {
    ECF_NAME_THIS_TEST();

    std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();

    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB, 10));
    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::PATH, child_cmds, ecf::User::FAIL, 10));
    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::ECF, child_cmds, ecf::User::BLOCK, 10));
    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::ECF_PID, child_cmds, ecf::User::REMOVE, 10));
    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::ECF_PID_PASSWD, child_cmds, ecf::User::KILL, 10));
    doSaveAndRestore(fileName, ZombieAttr(ecf::Child::ECF_PASSWD, child_cmds, ecf::User::ADOPT, 10));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

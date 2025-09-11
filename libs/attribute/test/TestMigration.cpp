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
#include "ecflow/core/File.hpp"
#include "ecflow/core/TimeSlot.hpp"
#include "ecflow/core/cereal_boost_time.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace ecf;

// #define UPDATE_TESTS 1

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Migration)

// These test are used for future release. They help to ensure that we have
// backward compatibility.i.e future release can open file, created by an earlier release
//
BOOST_AUTO_TEST_CASE(test_migration_restore_def_con) {
    ECF_NAME_THIS_TEST();

    std::string file_name =
        File::test_data("libs/attribute/test/data/migration/default_constructor_1_2_2/", "libs/attribute");
    // BOOST_CHECK_MESSAGE(File::createDirectories(file_name ),"Could not create directory " << file_name);

    // Create migration data
#ifdef UPDATE_TESTS
    doSave(file_name + "VerifyAttr", VerifyAttr());
    doSave(file_name + "TodayAttr", TodayAttr());
    doSave(file_name + "TimeAttr", TimeAttr());
    doSave(file_name + "RepeatDate", RepeatDate());
    doSave(file_name + "RepeatDateList", RepeatDateList());
    doSave(file_name + "RepeatInteger", RepeatInteger());
    doSave(file_name + "RepeatEnumerated", RepeatEnumerated());
    doSave(file_name + "RepeatString", RepeatString());
    doSave(file_name + "LateAttr", LateAttr());
    doSave(file_name + "DayAttr", DayAttr());
    doSave(file_name + "DateAttr", DateAttr());
    doSave(file_name + "CronAttr", CronAttr());
    doSave(file_name + "ClockAttr", ClockAttr());
    doSave(file_name + "AutoCancelAttr", AutoCancelAttr());
    doSave(file_name + "AutoArchiveAttr", AutoArchiveAttr());
    doSave(file_name + "Label", Label());
    doSave(file_name + "Variable", Variable());
    doSave(file_name + "Event", Event());
    doSave(file_name + "Meter", Meter());
    doSave(file_name + "ZombieAttr", ZombieAttr());
    doSave(file_name + "QueueAttr", QueueAttr());
    doSave(file_name + "GenericAttr", GenericAttr());
#endif

    do_restore<VerifyAttr>(file_name + "VerifyAttr", VerifyAttr());
    do_restore<TodayAttr>(file_name + "TodayAttr", TodayAttr());
    do_restore<TimeAttr>(file_name + "TimeAttr", TimeAttr());
    do_restore<RepeatDate>(file_name + "RepeatDate", RepeatDate());
    do_restore<RepeatDateList>(file_name + "RepeatDateList", RepeatDateList());
    do_restore<RepeatInteger>(file_name + "RepeatInteger", RepeatInteger());
    do_restore<RepeatEnumerated>(file_name + "RepeatEnumerated", RepeatEnumerated());
    do_restore<RepeatString>(file_name + "RepeatString", RepeatString());
    do_restore<LateAttr>(file_name + "LateAttr", LateAttr());
    do_restore<DayAttr>(file_name + "DayAttr", DayAttr());
    do_restore<DateAttr>(file_name + "DateAttr", DateAttr());
    do_restore<CronAttr>(file_name + "CronAttr", CronAttr());
    do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr", AutoCancelAttr());
    do_restore<AutoArchiveAttr>(file_name + "AutoArchiveAttr", AutoArchiveAttr());
    do_restore<Label>(file_name + "Label", Label());
    do_restore<Variable>(file_name + "Variable", Variable());
    do_restore<Event>(file_name + "Event", Event());
    do_restore<Meter>(file_name + "Meter", Meter());
    do_restore<ZombieAttr>(file_name + "ZombieAttr", ZombieAttr());
    do_restore<QueueAttr>(file_name + "QueueAttr", QueueAttr());
    do_restore<GenericAttr>(file_name + "GenericAttr", GenericAttr());
}

BOOST_AUTO_TEST_CASE(test_migration_restore) {
    ECF_NAME_THIS_TEST();

    std::string file_name = File::test_data("libs/attribute/test/data/migration/1_2_2/", "libs/attribute");
    // BOOST_CHECK_MESSAGE(File::createDirectories(file_name ),"Could not create directory " << file_name);

    std::vector<std::string> theVec;
    theVec.emplace_back("a");
    theVec.emplace_back("b");
    LateAttr lateattr;
    lateattr.addSubmitted(TimeSlot(10, 12));
    lateattr.addActive(TimeSlot(10, 12));
    lateattr.addComplete(TimeSlot(10, 12), true);

    CronAttr cron_attr;
    std::vector<int> weekDays;
    weekDays.push_back(1);
    weekDays.push_back(2);
    std::vector<int> daysOfMonth;
    daysOfMonth.push_back(1);
    daysOfMonth.push_back(2);
    std::vector<int> months;
    months.push_back(1);
    months.push_back(2);
    cron_attr.addWeekDays(weekDays);
    cron_attr.addDaysOfMonth(daysOfMonth);
    cron_attr.addMonths(months);
    cron_attr.addTimeSeries(TimeSlot(0, 0), TimeSlot(20, 0), TimeSlot(0, 1));

    ClockAttr clock_attr(false);
    clock_attr.date(1, 1, 2009);
    clock_attr.set_gain_in_seconds(3600);

    std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();

    Label label("name", "value");
    label.set_new_value("new_value");

#ifdef UPDATE_TESTS
    // Create migration data
    doSave(file_name + "VerifyAttr", VerifyAttr(NState::COMPLETE, 3));
    doSave(file_name + "TodayAttr", TodayAttr(10, 12));
    doSave(file_name + "TimeAttr", TimeAttr(10, 12));
    doSave(file_name + "RepeatDate", RepeatDate("date", 20110112, 20110115));
    doSave(file_name + "RepeatDateList", RepeatDateList("datelist", {19990101, 19990102}));
    doSave(file_name + "RepeatInteger", RepeatInteger("integer", 0, 100, 2));
    doSave(file_name + "RepeatEnumerated", RepeatEnumerated("enum", theVec));
    doSave(file_name + "RepeatString", RepeatString("string", theVec));
    doSave(file_name + "LateAttr", lateattr);
    doSave(file_name + "DayAttr", DayAttr(DayAttr::MONDAY));
    doSave(file_name + "DateAttr", DateAttr(12, 12, 2012));
    doSave(file_name + "CronAttr", cron_attr);
    doSave(file_name + "ClockAttr", clock_attr);
    doSave(file_name + "AutoCancelAttr", AutoCancelAttr(100));
    doSave(file_name + "AutoCancelAttr_1", AutoCancelAttr(TimeSlot(10, 12), true));
    doSave(file_name + "AutoArchiveAttr", AutoArchiveAttr(100));
    doSave(file_name + "AutoArchiveAttr_1", AutoArchiveAttr(TimeSlot(10, 12), true));
    doSave(file_name + "Label", label);
    //   doSave(file_name + "Limit",limit);
    doSave(file_name + "Variable", Variable("var_name", "var_value"));
    doSave(file_name + "Event_1", Event(1));
    doSave(file_name + "Event_2", Event("event"));
    doSave(file_name + "Event_3", Event(1, "event", true));
    doSave(file_name + "Meter", Meter("meter", 10, 100, 100));
    doSave(file_name + "ZombieAttr", ZombieAttr(ecf::Child::USER, child_cmds, ecf::UserAction::FOB));
    doSave(file_name + "ZombieAttr1", ZombieAttr(ecf::Child::USER, child_cmds, ecf::UserAction::FOB, 500));
    doSave(file_name + "QueueAttr", QueueAttr("queue", theVec));
    doSave(file_name + "GenericAttr", GenericAttr("gen1", theVec));
#endif

    do_restore<VerifyAttr>(file_name + "VerifyAttr", VerifyAttr(NState::COMPLETE, 3));
    do_restore<TodayAttr>(file_name + "TodayAttr", TodayAttr(10, 12));
    do_restore<TimeAttr>(file_name + "TimeAttr", TimeAttr(10, 12));
    do_restore<RepeatDate>(file_name + "RepeatDate", RepeatDate("date", 20110112, 20110115));
    do_restore<RepeatDateList>(file_name + "RepeatDateList", RepeatDateList("datelist", {19990101, 19990102}));
    do_restore<RepeatInteger>(file_name + "RepeatInteger", RepeatInteger("integer", 0, 100, 2));
    do_restore<RepeatEnumerated>(file_name + "RepeatEnumerated", RepeatEnumerated("enum", theVec));
    do_restore<RepeatString>(file_name + "RepeatString", RepeatString("string", theVec));
    do_restore<LateAttr>(file_name + "LateAttr", lateattr);
    do_restore<DayAttr>(file_name + "DayAttr", DayAttr(DayAttr::MONDAY));
    do_restore<DateAttr>(file_name + "DateAttr", DateAttr(12, 12, 2012));
    do_restore<CronAttr>(file_name + "CronAttr", cron_attr);
    do_restore<ClockAttr>(file_name + "ClockAttr", clock_attr);
    do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr", AutoCancelAttr(100));
    do_restore<AutoCancelAttr>(file_name + "AutoCancelAttr_1", AutoCancelAttr(TimeSlot(10, 12), true));
    do_restore<AutoArchiveAttr>(file_name + "AutoArchiveAttr", AutoArchiveAttr(100));
    do_restore<AutoArchiveAttr>(file_name + "AutoArchiveAttr_1", AutoArchiveAttr(TimeSlot(10, 12), true));
    do_restore<Label>(file_name + "Label", label);
    do_restore<Variable>(file_name + "Variable", Variable("var_name", "var_value"));
    do_restore<Event>(file_name + "Event_1", Event(1));
    do_restore<Event>(file_name + "Event_2", Event(std::string("event")));
    do_restore<Event>(file_name + "Event_3", Event(1, std::string("event"), true));
    do_restore<Meter>(file_name + "Meter", Meter("meter", 10, 100, 100));
    do_restore<ZombieAttr>(file_name + "ZombieAttr",
                           ZombieAttr(ecf::Child::USER, child_cmds, ecf::ZombieCtrlAction::FOB));
    do_restore<ZombieAttr>(file_name + "ZombieAttr1",
                           ZombieAttr(ecf::Child::USER, child_cmds, ecf::ZombieCtrlAction::FOB, 500));
    do_restore<QueueAttr>(file_name + "QueueAttr", QueueAttr("queue", theVec));
    do_restore<GenericAttr>(file_name + "GenericAttr", GenericAttr("gen1", theVec));
}

// The following test shows that CEREAL ignores new data members it does not recognise.
// This allows new data members, which will be ignored by the old client.
// OLD server to NEW Client(GUI), will not be a problem, since the new data members are conditional.
// In ecflow 5.4.0/5.5.0 we added a new data members to the DayAttribute class.
// This test shows that when NEW SERVER send DayAttr to OLD Client(GUI), this new data member is ignored.
namespace version_old {
class DayAttr {
public:
    enum Day_t { SUNDAY = 0, MONDAY = 1, TUESDAY = 2, WEDNESDAY = 3, THURSDAY = 4, FRIDAY = 5, SATURDAY = 6 };
    DayAttr() = default;
    explicit DayAttr(Day_t day) : day_(day) {}
    bool operator==(const DayAttr& rhs) const { return day_ == rhs.day() && free_ == rhs.free(); }

    DayAttr::Day_t day() const { return day_; }
    bool free() const { return free_; }
    void set_free() { free_ = true; }

private:
    DayAttr::Day_t day_{DayAttr::SUNDAY};
    bool free_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(day_));
        CEREAL_OPTIONAL_NVP(ar, free_, [this]() { return free_; }); // conditionally save
    }
};
} // namespace version_old

namespace version_new_data_member {
class DayAttr {
public:
    enum Day_t { SUNDAY = 0, MONDAY = 1, TUESDAY = 2, WEDNESDAY = 3, THURSDAY = 4, FRIDAY = 5, SATURDAY = 6 };
    DayAttr() = default;
    explicit DayAttr(Day_t day) : day_(day) {}
    bool operator==(const DayAttr& rhs) const {
        return day_ == rhs.day_ && expired_ == rhs.expired() && free_ == rhs.free() && date_ == rhs.date_;
    }

    DayAttr::Day_t day() const { return day_; }
    bool expired() const { return expired_; }
    bool free() const { return free_; }
    boost::gregorian::date date() const { return date_; }

    void set_free() { free_ = true; }
    void set_expired() { expired_ = true; }
    void set_date(boost::gregorian::date date) { date_ = std::move(date); }

private:
    DayAttr::Day_t day_{DayAttr::SUNDAY};
    bool free_{false};
    bool expired_{false};         // new data member 5.4.0
    boost::gregorian::date date_; // new data member 5.5.0

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(day_));
        CEREAL_OPTIONAL_NVP(ar, free_, [this]() { return free_; });       // conditionally save
        CEREAL_OPTIONAL_NVP(ar, expired_, [this]() { return expired_; }); // conditionally save
        CEREAL_OPTIONAL_NVP(
            ar, date_, [this]() { return !date_.is_special(); }); // conditionally save, new to ecflow 5.5.0,
    }
};
} // namespace version_new_data_member

BOOST_AUTO_TEST_CASE(test_day_migration) {
    ECF_NAME_THIS_TEST();

    // OLD -> NEW  i.e. OLD SERVER --> NEW CLIENT
    {
        const version_old::DayAttr t = version_old::DayAttr();
        ecf::save("test_day_migration", t);
    }
    {
        version_new_data_member::DayAttr t;
        ecf::restore("test_day_migration", t);
        BOOST_CHECK_MESSAGE(t == version_new_data_member::DayAttr(), "Should be the same");
    }

    // NEW->OLD  i.e. NEW SERVER --> OLD CLIENT
    // IMPORTANT: This shows that CEREAL ignore data members is does NOT RECOGNIZE.
    {
        version_new_data_member::DayAttr def;
        ecf::save("test_day_migration_def", def);

        version_new_data_member::DayAttr def_free;
        def_free.set_free();
        ecf::save("test_day_migration_free", def_free);

        version_new_data_member::DayAttr def_expired;
        def_expired.set_expired();
        ecf::save("test_day_migration_expired", def_expired);

        version_new_data_member::DayAttr def_free_and_expired;
        def_free_and_expired.set_expired();
        def_free_and_expired.set_free();
        ecf::save("test_day_migration_free_and_expired", def_free_and_expired);

        version_new_data_member::DayAttr def_free_expired_date;
        def_free_expired_date.set_expired();
        def_free_expired_date.set_free();
        def_free_expired_date.set_date(boost::gregorian::date(2020, 06, 28)); // sunday
        ecf::save("test_day_migration_free_expired_date", def_free_expired_date);
    }
    {
        version_old::DayAttr def = version_old::DayAttr();
        ecf::restore("test_day_migration_def", def);
        BOOST_CHECK_MESSAGE(def == version_old::DayAttr(), "Should be the same");

        version_old::DayAttr def_free = version_old::DayAttr();
        ecf::restore("test_day_migration_free", def_free);
        version_old::DayAttr cdef_free;
        cdef_free.set_free();
        BOOST_CHECK_MESSAGE(def_free == cdef_free, "Should be the same");

        version_old::DayAttr def_expired = version_old::DayAttr();
        ecf::restore("test_day_migration_expired", def_expired);
        BOOST_CHECK_MESSAGE(def_expired == version_old::DayAttr(), "No expired, should be same as default");

        version_old::DayAttr def_free_and_expired = version_old::DayAttr();
        ecf::restore("test_day_migration_free_and_expired", def_free_and_expired);
        BOOST_CHECK_MESSAGE(def_free_and_expired == cdef_free, "No expired,But free should be set,Should be the same");

        version_old::DayAttr def_free_expired_date = version_old::DayAttr();
        ecf::restore("test_day_migration_free_expired_date", def_free_expired_date);
        BOOST_CHECK_MESSAGE(def_free_and_expired == cdef_free,
                            "No expired,No date,But free should be set,Should be the same");
    }

    // remove the generated filea, comment out to debug.
    fs::remove("test_day_migration");
    fs::remove("test_day_migration_def");
    fs::remove("test_day_migration_free");
    fs::remove("test_day_migration_expired");
    fs::remove("test_day_migration_free_and_expired");
    fs::remove("test_day_migration_free_expired_date");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

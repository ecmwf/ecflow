#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import copy

import pytest

from ecflow import (
    Defs,
    Suite,
    Variable,
    Limit,
    InLimit,
    Task,
    PartExpression,
    Event,
    Meter,
    Label,
    Queue,
    RepeatInteger,
    RepeatEnumerated,
    RepeatDate,
    RepeatDateTime,
    RepeatDateList,
    RepeatString,
    TimeSlot,
    TimeSeries,
    Today,
    Time,
    Date,
    Day,
    Days,
    Cron,
    Autocancel,
    Late,
    DState,
    Clock,
    ChildCmdType,
    ZombieType,
    ZombieAttr,
    ZombieUserActionType,
)


@pytest.fixture
def time_series():
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    return TimeSeries(start, finish, incr, True)


def test_add_nodes_functionally():
    defs = Defs()
    defs.add_suite("s1").add_task("t1").add_variable("var", "v")
    defs.add_suite("s2").add_family("f1").add_task("t1").add_variable("var", "v")
    defs.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable(
        "var", "v"
    )
    assert len(defs) == 3


def test_suite_add_delete_variables_via_dict_and_chaining():
    suite = Suite("s1")
    a_dict = {"name": "value", "name2": "value2", "name3": "value3", "name4": "value4"}
    suite.add_variable(Variable("ECF_HOME", "'/tmp/'")).add_variable(
        "Fred", 1
    ).add_variable("ECF_URL_BASE", "http://www.ecmwf.int").add_variable(
        "ECF_URL", '"publications/manuals/sms"'
    ).add_variable(
        a_dict
    )
    assert len(list(suite.variables)) == 8
    suite.delete_variable("")
    assert len(list(suite.variables)) == 0


def test_suite_add_delete_limits():
    suite = Suite("s1")
    the_limit = Limit("limitName1", 10)
    assert len(list(the_limit.node_paths())) == 0
    suite.add_limit(Limit("limitName1", 10)).add_limit(
        Limit("limitName2", 10)
    ).add_limit("limitName3", 10).add_limit("limitName4", 10)
    assert len(list(suite.limits)) == 4
    suite.delete_limit("")
    assert len(list(suite.limits)) == 0


def test_suite_add_delete_inlimits():
    suite = Suite("s1")
    suite.add_inlimit(InLimit("limitName1", "/s1/f1", 2, True)).add_inlimit(
        InLimit("limitName2", "/s1/f1", 2)
    ).add_inlimit("limitName3", "/s1/f1", 2, True).add_inlimit(
        "limitName4", "/s1/f1", 2
    )
    assert len(list(suite.inlimits)) == 4
    suite.delete_inlimit("limitName1")
    assert len(list(suite.inlimits)) == 3
    suite.delete_inlimit("")
    assert len(list(suite.inlimits)) == 0


def test_task_add_delete_trigger_and_complete():
    task = Task("task")
    task.add_trigger("t2 == active").add_complete("t2 == complete")
    assert task.get_complete()
    assert task.get_trigger()
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()


def test_task_add_delete_part_trigger_and_complete():
    task = Task("task")
    task.add_part_trigger(PartExpression("t1 == complete")).add_part_complete(
        PartExpression("t3 == complete")
    ).add_part_complete(PartExpression("t4 == active", False))
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()


def test_task_add_delete_part_trigger_and_complete_strings():
    task = Task("task")
    task.add_part_trigger("t1 == complete").add_part_complete(
        "t3 == complete"
    ).add_part_complete("t4 == active", False)
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()


def test_task_add_delete_events():
    task = Task("task")
    task.add_event(Event(1)).add_event(2).add_event(Event(10, "Eventname")).add_event(
        10, "Eventname2"
    ).add_event("fred")
    assert len(list(task.events)) == 5
    task.delete_event("")
    assert len(list(task.events)) == 0


def test_task_add_delete_meters():
    task = Task("task")
    task.add_meter(Meter("metername1", 0, 100, 50)).add_meter(
        Meter("metername2", 0, 100)
    ).add_meter("metername3", 0, 100, 50).add_meter("metername4", 0, 100)
    assert len(list(task.meters)) == 4
    task.delete_meter("")
    assert len(list(task.meters)) == 0


def test_task_add_delete_queues():
    task = Task("task")
    queue_items = ["001", "002"]
    task.add_queue(Queue("q1", queue_items)).add_queue(
        Queue("q2", queue_items)
    ).add_queue("q3", queue_items).add_queue("q4", queue_items)
    assert len(list(task.queues)) == 4
    task.delete_queue("")
    assert len(list(task.queues)) == 0


def test_task_add_delete_labels():
    task = Task("task")
    task.add_label(Label("label_name1", "value")).add_label(
        Label("label_name2", "value")
    ).add_label("label_name3", "value").add_label("label_name4", "value")
    assert len(list(task.labels)) == 4
    task.delete_label("")
    assert len(list(task.labels)) == 0


@pytest.mark.parametrize(
    "repeat_attr",
    [
        RepeatInteger("integer", 0, 100, 2),
        RepeatEnumerated("enum", ["red", "green", "blue"]),
        RepeatDate("date", 20100111, 20100115, 2),
        RepeatDateTime("datetime", "20100111T000000", "20100115T000000", "48:00:00"),
        RepeatDateList("date", [20100111, 20100115]),
        RepeatString("string", ["a", "b", "c"]),
    ],
)
def test_task_add_delete_repeats(repeat_attr):
    task = Task("task")
    task.add_repeat(repeat_attr).add_variable("var", "j")
    assert not task.get_repeat().empty()
    task.delete_repeat()
    assert task.get_repeat().empty()


def test_task_add_delete_todays(time_series):
    task = Task("task")
    task.add_today("00:30").add_today("+00:30").add_today(
        "+00:30 20:00 01:00"
    ).add_today(Today(time_series)).add_today(Today(0, 10)).add_today(
        0, 59, True
    ).add_today(
        Today(TimeSlot(20, 10))
    ).add_today(
        Today(TimeSlot(20, 20), False)
    )
    assert len(list(task.todays)) == 8
    deleting = [copy.copy(today) for today in task.todays]
    for today in deleting:
        task.delete_today(today)
    assert len(list(task.todays)) == 0


def test_task_add_delete_times(time_series):
    task = Task("task")
    task.add_time("00:30").add_time("+00:30").add_time("+00:30 20:00 01:00").add_time(
        Time(time_series)
    ).add_time(Time(0, 10)).add_time(0, 59, True).add_time(
        Time(TimeSlot(20, 10))
    ).add_time(
        Time(TimeSlot(20, 20), False)
    )
    assert len(list(task.times)) == 8
    deleting = [copy.copy(time) for time in task.times]
    for time in deleting:
        task.delete_time(time)
    assert len(list(task.times)) == 0


def test_task_add_delete_dates():
    task = Task("task")
    for i in [1, 2, 4, 8, 16]:
        task.add_date(i, 0, 0)
    task.add_date(Date(1, 1, 2010)).add_date(Date(2, 1, 2010)).add_date(
        Date(3, 1, 2010)
    ).add_date(Date(4, 1, 2010)).add_date(1, 1, 2010)
    assert len(list(task.dates)) == 10
    deleting = [copy.copy(date) for date in task.dates]
    for attr in deleting:
        task.delete_date(attr)
    assert len(list(task.dates)) == 0


def test_task_add_delete_days():
    task = Task("task")
    task.add_day(Day(Days.sunday)).add_day(Days.monday).add_day(Days.tuesday).add_day(
        "sunday"
    )
    assert len(list(task.days)) == 4
    deleting = [copy.copy(day) for day in task.days]
    for attr in deleting:
        task.delete_day(attr)
    assert len(list(task.days)) == 0


def test_task_add_delete_crons():
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries(start, finish, incr, True)
    cron.set_time_series(ts)

    cron1 = Cron()
    cron1.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron1.set_time_series(1, 30, True)

    cron2 = Cron()
    cron2.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron2.set_time_series("00:30 01:30 00:01")

    cron3 = Cron()
    cron3.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron3.set_time_series("+00:30")

    task = Task("task")
    task.add_cron(cron).add_cron(cron1).add_cron(cron2).add_cron(cron3)
    assert len(list(task.crons)) == 4
    deleting = [copy.copy(cron) for cron in task.crons]
    for attr in deleting:
        task.delete_cron(attr)
    assert len(list(task.crons)) == 0


def test_task_add_autocancel_variants():
    t1 = Task("t1")
    t1.add_autocancel(3).add_variable("A", "j")
    assert t1.get_autocancel() is not None

    t3 = Task("t3")
    t3.add_autocancel(20, 10, True).add_variable("B", "j")
    assert t3.get_autocancel() is not None

    t4 = Task("t4")
    t4.add_autocancel(TimeSlot(10, 10), True).add_variable("C", "j")
    assert t4.get_autocancel() is not None

    t5 = Task("t5")
    t5.add_autocancel(Autocancel(1, 10, True)).add_variable("D", "j")
    assert t5.get_autocancel() is not None


def test_task_add_late():
    task = Task("task")
    late = Late()
    late.submitted(TimeSlot(20, 10))
    late.active(TimeSlot(20, 10))
    late.complete(TimeSlot(20, 10), True)
    task.add_late(late).add_variable("FRED33", "j")
    assert task.get_late() is not None


def test_task_defstatus_last_set_wins():
    task = Task("task")
    task.add_defstatus(DState.complete).add_defstatus(DState.queued).add_defstatus(
        DState.aborted
    ).add_defstatus(DState.submitted).add_defstatus(DState.suspended).add_defstatus(
        DState.active
    )
    assert task.get_defstatus() == DState.active


def test_suite_clock_and_end_clock():
    clock = Clock(1, 1, 2010, False)
    clock.set_gain(1, 10, True)
    suite = Suite("suite")
    suite.add_clock(clock).add_variable("fred1", "j")
    assert suite.get_clock() is not None

    end_clock = Clock(1, 1, 2017, False)
    suite.add_end_clock(end_clock).add_variable("fred2", "j")
    assert suite.get_end_clock() is not None

    clock = Clock(1, 1, 2011, True)
    clock.set_gain_in_seconds(12, True)
    s1 = Suite("s1")
    s1.add_clock(clock).add_variable("fred3", "j")
    s1.add_end_clock(end_clock).add_variable("fred4", "j")
    assert s1.get_clock() is not None
    assert s1.get_end_clock() is not None


def test_suite_add_delete_zombies():
    s1 = Suite("s1")
    zombie_life_time_in_server = 800
    child_list = [
        ChildCmdType.init,
        ChildCmdType.event,
        ChildCmdType.meter,
        ChildCmdType.label,
        ChildCmdType.wait,
        ChildCmdType.abort,
        ChildCmdType.complete,
    ]
    zombie_type_list = [ZombieType.ecf, ZombieType.user, ZombieType.path]
    count = 1
    for zombie_type in zombie_type_list:
        zombie_attr = ZombieAttr(
            zombie_type,
            child_list,
            ZombieUserActionType.block,
            zombie_life_time_in_server,
        )
        s1.add_zombie(zombie_attr).add_variable("afred" + str(count), "j")
        count += 1
    assert len(list(s1.zombies)) == 3

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0

    child_list = []
    for zombie_type in zombie_type_list:
        zombie_attr = ZombieAttr(zombie_type, child_list, ZombieUserActionType.block)
        s1.add_zombie(zombie_attr).add_variable("bfred" + str(count), "j")
        assert len(list(s1.zombies)) == 1
        s1.delete_zombie(zombie_type)
        assert len(list(s1.zombies)) == 0
        count += 1

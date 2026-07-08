#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import os

import ecflow
import pytest

from ecflow import (
    Suite,
    Family,
    Task,
    Defs,
    Clock,
    DState,
    PartExpression,
    Variable,
    Limit,
    InLimit,
    Date,
    Day,
    Days,
    Event,
    Meter,
    Label,
    Autocancel,
    TimeSlot,
    TimeSeries,
    Style,
    State,
    RepeatString,
    RepeatDate,
    RepeatDateTime,
    RepeatInteger,
    RepeatDay,
    RepeatEnumerated,
    Verify,
    PrintStyle,
    Time,
    Today,
    Late,
    Cron,
)


def test_node_type_isinstance_relationships():
    suite = Suite("s1")
    assert isinstance(suite, ecflow.Suite)
    assert not isinstance(suite, ecflow.Family)
    assert not isinstance(suite, ecflow.Task)

    family = Family("f1")
    assert isinstance(family, ecflow.Family)
    assert not isinstance(family, ecflow.Suite)
    assert not isinstance(family, ecflow.Task)

    task = Task("f1")
    assert isinstance(task, ecflow.Task)
    assert not isinstance(task, ecflow.Suite)
    assert not isinstance(task, ecflow.Family)


def test_empty_defs_are_equal_and_default_state_unknown():
    defs = Defs()
    defs2 = Defs()
    assert defs == defs2
    assert defs.get_state() == ecflow.State.unknown


def test_defs_change_after_adding_suite():
    defs = Defs()
    defs2 = Defs()
    suite = Suite("s1")
    suite.add_family(Family("f1")).add_task(Task("f1"))
    defs.add_suite(suite)
    assert defs != defs2


def test_suite_construction_and_variables():
    defs = Defs()
    suite = defs.add_suite("s2")
    suite.add_variable(Variable("ECF_HOME", "/tmp/"))
    suite.add_variable(
        "ECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    suite.add_variable("ECF_URL_BASE", "http://www.ecmwf.int")
    suite.add_variable("ECF_URL", "publications/manuals/sms")
    suite.add_limit(Limit("limitName", 10))
    suite.add_limit("limitName_2", 10)

    clock = Clock(1, 1, 2010, False)
    clock.set_gain(1, 10, True)
    suite.add_clock(clock)


def test_defs_externs():
    defs = Defs()
    defs.add_extern("/path/to/task")
    defs.add_extern("/fred/bill:event_name")
    assert len(list(defs.externs)) == 2


def test_task_with_trigger_complete_autocancel():
    family = Family("f1")
    family.add_defstatus(DState.active)
    task = family.add_task("t1")
    task.add_trigger("t2 == active")
    task.add_complete("t2 == complete")
    task.add_autocancel(Autocancel(3))


def test_task_with_part_triggers_and_time_attributes():
    family = Family("f1")
    task2 = family.add_task("t2")
    task2.add_defstatus(DState.complete)
    task2.add_part_trigger(PartExpression("t1 == complete"))
    task2.add_part_trigger(PartExpression("t1 == active", True))
    task2.add_part_complete(PartExpression("t1 == complete"))
    task2.add_part_complete(PartExpression("t1 == active", False))
    task2.add_label(Label("labelname", "label1"))
    task2.add_label("labelname2", "label1")
    task2.add_inlimit(InLimit("limitName", "/s1/f1", 2))
    task2.add_inlimit(InLimit("limitName1", "/s1/f1", 2, True))
    task2.add_inlimit("limitName2")
    task2.add_inlimit("limitName3", "path")
    task2.add_inlimit("limitName4", "path", 1)
    task2.add_inlimit("limitName5", "path", 1, True)
    task2.add_event(10)
    task2.add_event(Event(10, "Eventname"))
    task2.add_meter(Meter("metername", 0, 100, 50))
    task2.add_meter("metername2", 0, 100, 50)
    task2.add_date(Date(1, 1, 2010))
    task2.add_date(1, 2, 2010)
    task2.add_day(Day(Days.sunday))
    task2.add_day(Days.monday)
    task2.add_autocancel(Autocancel(TimeSlot(20, 10), True))

    task2.add_today(0, 10)
    task2.add_today(Today(0, 59, True))
    task2.add_today(Today(TimeSlot(20, 10)))
    task2.add_today(Today(TimeSlot(20, 20), False))
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries(start, finish, incr, True)
    task2.add_today(Today(ts))

    task2.add_time(20, 10)
    task2.add_time(Time(20, 12, True))
    task2.add_time(Time(TimeSlot(20, 12)))
    task2.add_time(Time(TimeSlot(20, 20), True))
    task2.add_time(Time(ts))

    late = Late()
    late.submitted(20, 10)
    late.active(TimeSlot(20, 10))
    late.complete(TimeSlot(20, 10), True)
    task2.add_late(late)

    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])
    cron.set_time_series(ts)
    task2.add_cron(cron)


def test_various_repeat_types():
    defs = Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("f1")

    task3 = family.add_task("t3")
    task3.add_repeat(RepeatDate("testDate", 20100111, 20100115, 2))
    task3.add_defstatus(DState.queued)
    task3.add_verify(Verify(State.complete, 1))
    task3.add_autocancel(Autocancel(20, 10, False))
    task3.add_label(Label("label_name", "label_value"))

    task3_1 = family.add_task("t3_1")
    task3_1.add_repeat(RepeatDate("testDate", 20100111, 20100115))

    task3_2 = family.add_task("t3_2")
    task3_2.add_repeat(
        RepeatDateTime("testDateTime", "20100111T000000", "20100115T000000")
    )

    task4 = family.add_task("t4")
    task4.add_repeat(RepeatInteger("testInteger", 0, 100, 2))
    task4.add_defstatus(DState.aborted)

    task4_1 = family.add_task("t4_1")
    task4_1.add_repeat(RepeatInteger("testInteger", 0, 100))

    task5 = family.add_task("t5")
    task5.add_repeat(RepeatEnumerated("testInteger", ["red", "green", "blue"]))
    task5.add_defstatus(DState.submitted)

    task6 = family.add_task("t6")
    task6.add_repeat(RepeatString("test_string", ["a", "b", "c"]))
    task6.add_defstatus(DState.suspended)

    task7 = family.add_task("t7")
    task7.add_repeat(RepeatDay(2))
    task7.add_defstatus(DState.active)

    task8 = family.add_task("t8")
    task8.add_repeat(RepeatDay())
    assert task8.get_try_no() == "0"
    assert task8.get_int_try_no() == 0
    assert task8.get_defs() is not None


def test_defs_find_suite():
    defs = Defs()
    suite = Suite("s1")
    defs.add_suite(suite)
    assert defs.find_suite("s1") is not None
    assert defs.find_suite("sffff1") is None


def test_save_and_restore_defs(tmp_path):
    defs = Defs()
    suite = Suite("s1")
    suite.add_family(Family("f1")).add_task(Task("t1"))
    defs.add_suite(suite)
    defs.add_extern("/path/to/task")
    defs.add_extern("/fred/bill:event_name")

    checkpt_file = str(tmp_path / "py_u_TestDefs.check")
    defs_file = str(tmp_path / "py_u_TestDefs.def")

    defs.save_as_checkpt(checkpt_file)
    restored_checkpt_defs = Defs()
    restored_checkpt_defs.restore_from_checkpt(checkpt_file)

    defs.save_as_defs(defs_file)
    restored_from_defs = Defs(defs_file)

    defs.save_as_defs(defs_file, Style.DEFS)
    restored_from_defs = Defs(defs_file)

    defs.save_as_defs(defs_file, Style.STATE)
    restored_from_defs = Defs(defs_file)

    defs.save_as_defs(defs_file, Style.MIGRATE)
    restored_from_defs = Defs(defs_file)

    assert restored_checkpt_defs == restored_from_defs


def test_print_styles():
    defs = Defs()
    suite = Suite("s1")
    suite.add_family(Family("f1")).add_task(Task("t1"))
    defs.add_suite(suite)
    defs_file = "tmp_def_for_print.def"
    defs.save_as_defs(defs_file, Style.MIGRATE)
    restored_from_defs = Defs(defs_file)
    try:
        os.remove(defs_file)
    except OSError:
        pass

    PrintStyle.set_style(Style.DEFS)
    the_string = str(restored_from_defs)
    assert "defs_state" not in the_string

    PrintStyle.set_style(Style.STATE)
    the_string = str(restored_from_defs)
    assert "defs_state STATE" in the_string

    try:
        PrintStyle.set_style(Style.MIGRATE)
        the_string = str(restored_from_defs)
        assert "defs_state MIGRATE" in the_string
    finally:
        PrintStyle.set_style(Style.DEFS)


def test_job_creation_failure():
    defs = Defs()
    suite = Suite("s1")
    suite.add_family(Family("f1")).add_task(Task("t1"))
    defs.add_suite(suite)
    msg = defs.check_job_creation()
    assert len(msg) != 0

    with pytest.raises(RuntimeError):
        defs.check_job_creation(throw_on_error=True)

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

import ecflow


@pytest.fixture
def time_series():
    start = ecflow.TimeSlot(0, 0)
    finish = ecflow.TimeSlot(23, 0)
    incr = ecflow.TimeSlot(0, 30)
    return ecflow.TimeSeries(start, finish, incr, True)


def _names(collection):
    return [item.name() for item in collection]


def test_defs_user_variable_add_delete_and_sort():
    defs = ecflow.Defs()
    defs.add_variable("ZFRED", "/tmp/")
    defs.add_variable(
        "YECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    defs.add_variable("XECF_URL_BASE", "http://www.ecmwf.int")
    defs.add_variable("AECF_URL", "publications/manuals/sms")
    assert len(list(defs.user_variables)) == 4

    defs.sort_attributes("variable")
    assert _names(defs.user_variables) == [
        "AECF_URL",
        "XECF_URL_BASE",
        "YECF_URL_CMD",
        "ZFRED",
    ]

    defs.add_variable("ZZ", "x")
    defs.sort_attributes(ecflow.AttrType.variable)
    assert _names(defs.user_variables) == [
        "AECF_URL",
        "XECF_URL_BASE",
        "YECF_URL_CMD",
        "ZFRED",
        "ZZ",
    ]

    defs.add_variable("AA", "x")
    defs.sort_attributes("all")
    assert _names(defs.user_variables) == [
        "AA",
        "AECF_URL",
        "XECF_URL_BASE",
        "YECF_URL_CMD",
        "ZFRED",
        "ZZ",
    ]

    defs.add_variable("BB", "x")
    defs.sort_attributes(ecflow.AttrType.all)
    assert _names(defs.user_variables) == [
        "AA",
        "AECF_URL",
        "BB",
        "XECF_URL_BASE",
        "YECF_URL_CMD",
        "ZFRED",
        "ZZ",
    ]

    defs.delete_variable("ZFRED")
    assert len(list(defs.user_variables)) == 6
    defs.delete_variable("")
    assert len(list(defs.user_variables)) == 0

    a_dict = {"name": "value", "name2": "value2", "name3": "value3", "name4": "value4"}
    defs.add_variable(a_dict)
    assert len(list(defs.user_variables)) == 4
    defs.delete_variable("")
    assert len(list(defs.user_variables)) == 0

    defs.add_variable({})
    assert len(list(defs.user_variables)) == 0

    a_dict = {"name": 0, "name2": 1, "name3": 2, "name4": 3}
    defs.add_variable(a_dict)
    assert len(list(defs.user_variables)) == 4
    defs.delete_variable("")
    assert len(list(defs.user_variables)) == 0


def test_suite_variable_add_delete_and_sort():
    suite = ecflow.Suite("s1")
    suite.add_variable(ecflow.Variable("ECF_HOME", "/tmp/"))
    suite.add_variable(
        "ZZZZZZ", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    suite.add_variable("YYYYY", "http://www.ecmwf.int")
    suite.add_variable("aaaa", "publications/manuals/sms")
    assert len(list(suite.variables)) == 4

    suite.sort_attributes("variable")
    suite.sort_attributes(ecflow.AttrType.variable)
    actual = _names(suite.variables)
    expected = ["aaaa", "ECF_HOME", "YYYYY", "ZZZZZZ"]
    assert set(expected).issubset(actual)

    suite.delete_variable("ECF_HOME")
    assert len(list(suite.variables)) == 3
    suite.delete_variable("")
    assert len(list(suite.variables)) == 0

    a_dict = {"name": "value", "name2": "value2", "name3": "value3", "name4": "value4"}
    suite.add_variable(a_dict)
    assert len(list(suite.variables)) == 4
    suite.delete_variable("")
    assert len(list(suite.variables)) == 0

    a_dict = {"name": 1, "name2": 2, "name3": 3, "name4": 4}
    suite.add_variable(a_dict)
    assert len(list(suite.variables)) == 4
    suite.delete_variable("")
    assert len(list(suite.variables)) == 0

    suite.add_variable({})
    assert len(list(suite.variables)) == 0

    with pytest.raises((TypeError, RuntimeError)):
        suite.add_variable(
            {"name": "fred", "name2": 14, "name3": list("123"), "name4": 12}
        )
    assert len(list(suite.variables)) == 0

    suite.add_variable("ECF_URL_CMD", "test duplicates")
    suite.add_variable("ECF_URL_CMD", "Expected warning")


def test_suite_limit_add_delete_and_sort():
    suite = ecflow.Suite("s1")
    suite.add_limit(ecflow.Limit("zlimitName1", 10))
    suite.add_limit(ecflow.Limit("ylimitName2", 10))
    suite.add_limit("xlimitName3", 10)
    suite.add_limit("alimitName4", 10)
    assert len(list(suite.limits)) == 4

    suite.sort_attributes(ecflow.AttrType.limit)
    suite.sort_attributes("limit")
    assert _names(suite.limits) == [
        "alimitName4",
        "xlimitName3",
        "ylimitName2",
        "zlimitName1",
    ]

    suite.delete_limit("zlimitName1")
    assert len(list(suite.limits)) == 3
    suite.delete_limit("")
    assert len(list(suite.limits)) == 0

    for name in ["limitName1", "limitName2", "limitName3", "limitName4"]:
        suite.add_limit(name, 10)
    limit_names = [limit.name() for limit in suite.limits]
    for limit in limit_names:
        suite.delete_limit(limit)
    assert len(list(suite.limits)) == 0


def test_limit_increment_decrement_and_node_paths():
    the_limit = ecflow.Limit("limitName1", 10)
    assert the_limit.name() == "limitName1"
    assert the_limit.value() == 0
    assert the_limit.limit() == 10
    assert len(list(the_limit.node_paths())) == 0

    for i, path in enumerate(["/path1", "/path2", "/path3", "/path4"], start=1):
        the_limit.increment(1, path)
        assert the_limit.value() == i  # verify value after each increment
    assert the_limit.value() == 4
    assert len(list(the_limit.node_paths())) == 4

    for i, path in enumerate(["/path1", "/path2", "/path3", "/path4"], start=1):
        the_limit.decrement(1, path)
        assert the_limit.value() == 4 - i  # verify value after each decrement
    assert the_limit.value() == 0
    assert len(list(the_limit.node_paths())) == 0

    for _ in range(4):
        the_limit.increment(1, "/path1")
    assert len(list(the_limit.node_paths())) == 1
    assert the_limit.value() == 1


def test_suite_inlimit_add_and_delete():
    suite = ecflow.Suite("s1")
    suite.add_inlimit(ecflow.InLimit("limitName1", "/s1/f1", 2))
    suite.add_inlimit(ecflow.InLimit("limitName2", "/s1/f1", 2, True))
    suite.add_inlimit("limitName3", "/s1/f1", 2)
    suite.add_inlimit("limitName4", "/s1/f1", 2)
    suite.add_inlimit("limitNameA", "/s1/f1", 2)
    suite.add_inlimit("limitNameA", "/s1/f1/a", 2)
    suite.add_inlimit("limitNameA", "/s1/f1/b", 2)
    assert len(list(suite.inlimits)) == 7

    suite.delete_inlimit("limitName1")
    assert len(list(suite.inlimits)) == 6
    suite.delete_inlimit("/s1/f1:limitNameA")
    assert len(list(suite.inlimits)) == 5
    suite.delete_inlimit("/s1/f1/a:limitNameA")
    assert len(list(suite.inlimits)) == 4
    suite.delete_inlimit("/s1/f1/b:limitNameA")
    assert len(list(suite.inlimits)) == 3
    suite.delete_inlimit("")
    assert len(list(suite.inlimits)) == 0


def test_task_trigger_complete_and_part_expressions():
    task = ecflow.Task("task")
    task.add_trigger("t2 == active")
    task.add_complete("t2 == complete")
    assert task.get_complete()
    assert task.get_trigger()
    assert task.get_trigger().get_expression() == "t2 == active"
    assert task.get_complete().get_expression() == "t2 == complete"
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()

    task.add_part_trigger(ecflow.PartExpression("t1 == complete"))
    task.add_part_trigger(ecflow.PartExpression("t2 == active", True))
    task.add_part_complete(ecflow.PartExpression("t3 == complete"))
    task.add_part_complete(ecflow.PartExpression("t4 == active", False))
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()

    task.add_part_trigger("t1 == complete")
    task.add_part_trigger("t2 == active", True)
    task.add_part_complete("t3 == complete")
    task.add_part_complete("t4 == active", False)
    task.delete_trigger()
    assert not task.get_trigger()
    task.delete_complete()
    assert not task.get_complete()


def test_task_trigger_from_expression():
    expr = ecflow.Expression("t1 == complete")
    task = ecflow.Task("task")
    task.add_trigger(expr)
    assert task.get_trigger().get_expression() == "t1 == complete"


def test_task_compound_expression_trigger():
    expr = ecflow.Expression("t1 == complete")
    expr.add(ecflow.PartExpression("t2 == complete", True))
    expr.add(ecflow.PartExpression("t3 == complete", True))
    task = ecflow.Task("task")
    task.add_trigger(expr)
    assert (
        task.get_trigger().get_expression()
        == "t1 == complete AND t2 == complete AND t3 == complete"
    )


def test_task_event_add_find_delete():
    task = ecflow.Task("task")
    task.add_event(ecflow.Event(1))
    task.add_event(2)
    task.add_event(ecflow.Event(10, "Eventname"))
    task.add_event(10, "Eventname2")
    task.add_event("fred")

    task.sort_attributes("event")
    task.sort_attributes(ecflow.AttrType.event)
    assert [e.name_or_number() for e in task.events] == [
        "1",
        "2",
        "Eventname",
        "Eventname2",
        "fred",
    ]

    event = task.find_event("EVENT")
    assert event.empty()
    assert event.name() == ""
    assert event.value() == 0

    event = task.find_event("1")
    assert not event.empty()
    assert event.number() == 1
    assert event.name() == ""
    assert event.value() == 0

    event = task.find_event("2")
    assert not event.empty()
    assert event.number() == 2
    assert event.name() == ""
    assert event.value() == 0

    event = task.find_event("10")
    assert not event.empty()
    assert event.number() == 10
    assert event.name() == "Eventname"
    assert event.value() == 0

    event = task.find_event("fred")
    assert not event.empty()
    assert event.name() == "fred"
    assert event.value() == 0

    assert len(list(task.events)) == 5
    task.delete_event("1")
    assert len(list(task.events)) == 4
    task.delete_event("Eventname")
    assert len(list(task.events)) == 3
    task.delete_event("")
    assert len(list(task.events)) == 0


def test_task_meter_add_delete_and_sort():
    task = ecflow.Task("task")
    task.add_meter(ecflow.Meter("zzzz", 0, 100, 50))
    task.add_meter(ecflow.Meter("yyyy", 0, 100))
    task.add_meter("bbbb", 0, 100, 50)
    task.add_meter("aaaa", 0, 100)
    assert len(list(task.meters)) == 4

    task.sort_attributes(ecflow.AttrType.meter)
    task.sort_attributes("meter")
    assert _names(task.meters) == ["aaaa", "bbbb", "yyyy", "zzzz"]

    task.delete_meter("zzzz")
    assert len(list(task.meters)) == 3
    task.delete_meter("yyyy")
    assert len(list(task.meters)) == 2
    task.delete_meter("")
    assert len(list(task.meters)) == 0


def test_task_queue_add_delete():
    task = ecflow.Task("task")
    queue_items = ["001", "002"]
    task.add_queue(ecflow.Queue("queue", queue_items))
    task.add_queue("queue1", queue_items)
    task.add_queue("queue2", queue_items)
    task.add_queue("queue3", queue_items)
    assert len(list(task.queues)) == 4
    task.delete_queue("queue3")
    assert len(list(task.queues)) == 3
    task.delete_queue("queue2")
    assert len(list(task.queues)) == 2
    task.delete_queue("")
    assert len(list(task.queues)) == 0


def test_task_generic_add_find_delete():
    task = ecflow.Task("task")
    generic_items = ["001", "002"]
    task.add_generic(ecflow.Generic("gen", generic_items))
    task.add_generic("gen1", generic_items)
    task.add_generic("gen2", generic_items)
    task.add_generic("gen3", generic_items)
    assert len(list(task.generics)) == 4

    gen = task.find_generic("gen3")
    assert not gen.empty()
    assert gen.name() == "gen3"

    task.delete_generic("gen1")
    assert len(list(task.generics)) == 3
    task.delete_generic("gen2")
    assert len(list(task.generics)) == 2
    task.delete_generic("")
    assert len(list(task.generics)) == 0


def test_task_label_add_delete_and_sort():
    task = ecflow.Task("task")
    task.add_label(ecflow.Label("labela", "value"))
    task.add_label(ecflow.Label("labelb", "value"))
    task.add_label("labelc", "value")
    task.add_label("labeld", "value")
    assert len(list(task.labels)) == 4

    task.sort_attributes("label")
    task.sort_attributes(ecflow.AttrType.label)
    assert _names(task.labels) == ["labela", "labelb", "labelc", "labeld"]

    task.delete_label("labela")
    assert len(list(task.labels)) == 3
    task.delete_label("labelb")
    assert len(list(task.labels)) == 2
    task.delete_label("")
    assert len(list(task.labels)) == 0


@pytest.mark.parametrize(
    "repeat_attr",
    [
        ecflow.RepeatInteger("integer", 0, 100, 2),
        ecflow.RepeatEnumerated("enum", ["red", "green", "blue"]),
        ecflow.RepeatDate("date", 20100111, 20100115, 2),
        ecflow.RepeatDateTime(
            "datetime", "20100111T000000", "20100115T000000", "48:00:00"
        ),
        ecflow.RepeatDateList("date", [20100111, 20100115]),
        ecflow.RepeatDateTimeList("datetime", ["20100111T000000", "20100115T000000"]),
        ecflow.RepeatString("string", ["a", "b", "c"]),
        ecflow.RepeatDay(1),
    ],
)
def test_task_repeat_present_then_deleted(repeat_attr):
    task = ecflow.Task("task")
    task.add_repeat(repeat_attr)
    assert not task.get_repeat().empty()
    task.delete_repeat()
    assert task.get_repeat().empty()


def test_task_today_add_delete(time_series):
    task = ecflow.Task("task")
    task.add_today("00:30")
    task.add_today("+00:30")
    task.add_today("+00:30 20:00 01:00")
    task.add_today(ecflow.Today(time_series))
    task.add_today(ecflow.Today("+00:30"))
    task.add_today(ecflow.Today("+00:30 20:00 01:00"))
    task.add_today(ecflow.Today(0, 10))
    task.add_today(0, 59, True)
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 10)))
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 20), False))
    assert len(list(task.todays)) == 10
    deleting = [copy.copy(today) for today in task.todays]
    for today in deleting:
        task.delete_today(today)
    assert len(list(task.todays)) == 0


def test_task_time_add_delete(time_series):
    task = ecflow.Task("task")
    task.add_time("00:30")
    task.add_time("+00:30")
    task.add_time("+00:30 20:00 01:00")
    task.add_time(ecflow.Time(time_series))
    task.add_time(ecflow.Time(0, 10))
    task.add_time(ecflow.Time("+00:30"))
    task.add_time(ecflow.Time("+00:30 20:00 01:00"))
    task.add_time(0, 59, True)
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 10)))
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 20), False))
    assert len(list(task.times)) == 10
    deleting = [copy.copy(time) for time in task.times]
    for time in deleting:
        task.delete_time(time)
    assert len(list(task.times)) == 0


def test_task_date_add_delete():
    task = ecflow.Task("task")
    for i in [1, 2, 4, 8, 16]:
        task.add_date(i, 0, 0)
    task.add_date(ecflow.Date(1, 1, 2010))
    task.add_date(1, 1, 2010)
    task.add_date(ecflow.Date(2, 1, 2010))
    task.add_date(ecflow.Date(3, 1, 2010))
    task.add_date(ecflow.Date(4, 1, 2010))
    assert len(list(task.dates)) == 10
    deleting = [copy.copy(date) for date in task.dates]
    for attr in deleting:
        task.delete_date(attr)
    assert len(list(task.dates)) == 0


def test_task_day_add_delete():
    task = ecflow.Task("task")
    task.add_day(ecflow.Day(ecflow.Days.sunday))
    task.add_day(ecflow.Days.monday)
    task.add_day(ecflow.Days.tuesday)
    task.add_day(ecflow.Day("tuesday"))
    task.add_day("sunday")
    assert len(list(task.days)) == 5
    deleting = [copy.copy(attr) for attr in task.days]
    for attr in deleting:
        task.delete_day(attr)
    assert len(list(task.days)) == 0


def test_task_cron_add_delete():
    cron = ecflow.Cron()
    start = ecflow.TimeSlot(23, 0)
    ts = ecflow.TimeSeries(start, True)
    cron.set_time_series(ts)

    cron = ecflow.Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])
    start = ecflow.TimeSlot(0, 0)
    finish = ecflow.TimeSlot(23, 0)
    incr = ecflow.TimeSlot(0, 30)
    ts = ecflow.TimeSeries(start, finish, incr, True)
    cron.set_time_series(ts)

    cron0 = ecflow.Cron()
    cron0.set_week_days([0, 1, 2, 3, 4, 5])
    cron0.set_time_series(1, 30)

    cron1 = ecflow.Cron()
    cron1.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron1.set_time_series(1, 30, True)

    cron2 = ecflow.Cron()
    cron2.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron2.set_time_series("00:30 01:30 00:01")

    cron3 = ecflow.Cron()
    cron3.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron3.set_time_series("+00:30")

    task = ecflow.Task("task")
    task.add_cron(cron)
    task.add_cron(cron1)
    task.add_cron(cron2)
    task.add_cron(cron3)
    assert len(list(task.crons)) == 4
    deleting = [copy.copy(cron) for cron in task.crons]
    for attr in deleting:
        task.delete_cron(attr)
    assert len(list(task.crons)) == 0


def test_task_autocancel_variants():
    t1 = ecflow.Task("t1")
    assert t1.get_autocancel() is None
    t1.add_autocancel(3)
    assert t1.get_autocancel() is not None

    t3 = ecflow.Task("t3")
    t3.add_autocancel(20, 10, True)
    assert t3.get_autocancel() is not None

    t4 = ecflow.Task("t4")
    t4.add_autocancel(ecflow.TimeSlot(10, 10), True)
    assert t4.get_autocancel() is not None

    t5 = ecflow.Task("t5")
    t5.add_autocancel(ecflow.Autocancel(1, 10, True))
    assert t5.get_autocancel() is not None


def test_family_autoarchive_variants():
    f1 = ecflow.Family("f1")
    assert f1.get_autoarchive() is None
    f1.add_autoarchive(3)
    assert f1.get_autoarchive() is not None

    f2 = ecflow.Family("f2")
    f2.add_autoarchive(3, True)
    assert f2.get_autoarchive() is not None

    f3 = ecflow.Family("f3")
    f3.add_autoarchive(20, 10, True)
    assert f3.get_autoarchive() is not None

    f3_1 = ecflow.Family("f3_1")
    f3_1.add_autoarchive(20, 10, True, True)
    assert f3_1.get_autoarchive() is not None

    f4 = ecflow.Family("f4")
    f4.add_autoarchive(ecflow.TimeSlot(10, 10), True)
    assert f4.get_autoarchive() is not None

    f4_1 = ecflow.Family("f4_1")
    f4_1.add_autoarchive(ecflow.TimeSlot(10, 10), True, True)
    assert f4_1.get_autoarchive() is not None

    f4_2 = ecflow.Family("f4_2")
    assert f4_2.get_autoarchive() is None
    f4_2.add_autoarchive(ecflow.Autoarchive(10, True))
    assert f4_2.get_autoarchive() is not None


def test_node_autorestore_variants():
    t1 = ecflow.Task("t1")
    assert t1.get_autorestore() is None
    t1.add_autorestore(["/s1/f2"])
    assert t1.get_autorestore() is not None

    f5 = ecflow.Family("f5")
    assert f5.get_autorestore() is None
    f5.add_autorestore(["/s1/f2", "/s2", "/s3"])
    assert f5.get_autorestore() is not None

    f6 = ecflow.Family("f6")
    assert f6.get_autorestore() is None
    f6.add_autorestore(ecflow.Autorestore(["/s1/f2", "/s2", "/s3"]))
    assert f6.get_autorestore() is not None


def test_task_late_variants():
    late = ecflow.Late()
    late.submitted(ecflow.TimeSlot(20, 10))
    late.active(ecflow.TimeSlot(20, 10))
    late.complete(ecflow.TimeSlot(20, 10), True)
    task = ecflow.Task("task")
    task.add_late(late)
    assert task.get_late() is not None

    t1 = ecflow.Task("t1")
    late = ecflow.Late()
    late.submitted(20, 10)
    late.active(20, 10)
    late.complete(20, 10, True)
    t1.add_late(late)
    assert t1.get_late() is not None


def test_task_defstatus_last_set_wins():
    task = ecflow.Task("task")
    task.add_defstatus(ecflow.DState.complete)
    assert task.get_defstatus() == ecflow.DState.complete
    task.add_defstatus(ecflow.DState.queued)
    assert task.get_defstatus() == ecflow.DState.queued
    task.add_defstatus(ecflow.DState.aborted)
    assert task.get_defstatus() == ecflow.DState.aborted
    task.add_defstatus(ecflow.DState.submitted)
    assert task.get_defstatus() == ecflow.DState.submitted
    task.add_defstatus(ecflow.Defstatus(ecflow.DState.suspended))
    assert task.get_defstatus() == ecflow.DState.suspended
    task.add_defstatus(ecflow.Defstatus("active"))
    assert task.get_defstatus() == ecflow.DState.active

    assert task.get_state() == ecflow.State.unknown
    assert task.get_dstate() == ecflow.DState.unknown


def test_suite_clock_and_end_clock():
    clock = ecflow.Clock(1, 1, 2010, False)
    clock.set_gain(1, 10, True)
    suite = ecflow.Suite("suite")
    suite.add_clock(clock)

    clock = ecflow.Clock(1, 1, 2011, True)
    clock.set_gain_in_seconds(12, True)
    s1 = ecflow.Suite("s1")
    s1.add_clock(clock)

    s0 = ecflow.Suite("s0")
    assert s0.get_clock() is None
    s0.add_clock(ecflow.Clock(1, 1, 2010, False))
    assert s0.get_clock() is not None

    clock = ecflow.Clock(1, 1, 2010, False)
    clock.set_gain(1, 10, True)
    suite = ecflow.Suite("suite")
    suite.add_end_clock(clock)

    clock = ecflow.Clock(1, 1, 2011, True)
    clock.set_gain_in_seconds(12, True)
    s1 = ecflow.Suite("s1")
    s1.add_end_clock(clock)

    s0 = ecflow.Suite("s0")
    assert s0.get_end_clock() is None
    s0.add_end_clock(ecflow.Clock(1, 1, 2010, False))
    assert s0.get_end_clock() is not None


def test_suite_zombie_add_delete():
    s1 = ecflow.Suite("s1")
    zombie_life_time_in_server = 800
    child_list = [
        ecflow.ChildCmdType.init,
        ecflow.ChildCmdType.event,
        ecflow.ChildCmdType.meter,
        ecflow.ChildCmdType.label,
        ecflow.ChildCmdType.wait,
        ecflow.ChildCmdType.abort,
        ecflow.ChildCmdType.complete,
    ]
    zombie_type_list = [
        ecflow.ZombieType.ecf,
        ecflow.ZombieType.ecf_pid,
        ecflow.ZombieType.ecf_pid_passwd,
        ecflow.ZombieType.ecf_passwd,
        ecflow.ZombieType.user,
        ecflow.ZombieType.path,
    ]

    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(
            zombie_type,
            child_list,
            ecflow.ZombieUserActionType.block,
            zombie_life_time_in_server,
        )
        s1.add_zombie(zombie_attr)
    assert len(list(s1.zombies)) == 6

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0

    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(
            zombie_type, child_list, ecflow.ZombieUserActionType.block
        )
        s1.add_zombie(zombie_attr)
    assert len(list(s1.zombies)) == 6

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0

    child_list = []
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(
            zombie_type,
            child_list,
            ecflow.ZombieUserActionType.block,
            zombie_life_time_in_server,
        )
        s1.add_zombie(zombie_attr)
        assert len(list(s1.zombies)) == 1
        s1.delete_zombie(zombie_type)
        assert len(list(s1.zombies)) == 0

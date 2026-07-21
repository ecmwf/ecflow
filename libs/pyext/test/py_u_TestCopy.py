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


def test_copy_defs_preserves_equality():
    defs = ecflow.Defs()
    defs.add_suite("a").add_family("f1").add_task("t1").add_variable(
        "a", "b"
    ).add_event(1).add_meter("meter", 0, 100).add_label("label", "v").add_time(
        "+00:30 20:00 01:00"
    )

    defs_copy = copy.copy(defs)
    assert defs_copy == defs, "defs should be equal after copy"


def test_copy_defs_user_variables_is_independent():
    defs = ecflow.Defs()
    defs.add_variable("FRED", "/tmp/")
    defs.add_variable(
        "ECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    defs.add_variable("ECF_URL_BASE", "http://www.ecmwf.int")
    defs.add_variable("ECF_URL", "publications/manuals/sms")
    assert len(list(defs.user_variables)) == 4, "Expected *user* 4 variable"

    defs_copy = copy.copy(defs)
    assert len(list(defs_copy.user_variables)) == 4, "Expected *user* 4 variable"

    defs.delete_variable("FRED")
    assert (
        len(list(defs.user_variables)) == 3
    ), "Expected 3 variables since we just delete FRED"
    defs.delete_variable("")
    assert (
        len(list(defs.user_variables)) == 0
    ), "Expected 0 variables since we should have deleted all"
    assert len(list(defs_copy.user_variables)) == 4, "Expected *user* 4 variable"


def test_copy_suite_variables_and_limits_are_independent():
    suite = ecflow.Suite("s1")
    suite.add_variable(ecflow.Variable("ECF_HOME", "/tmp/"))
    suite.add_variable(
        "ECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    suite.add_variable("ECF_URL_BASE", "http://www.ecmwf.int")
    suite.add_variable("ECF_URL", "publications/manuals/sms")
    assert len(list(suite.variables)) == 4, "Expected 4 variable"
    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.variables)) == 4, "Expected 4 variables in copy"

    suite.add_limit(ecflow.Limit("limitName1", 10))
    suite.add_limit(ecflow.Limit("limitName2", 10))
    suite.add_limit("limitName3", 10)
    suite.add_limit("limitName4", 10)
    assert len(list(suite.limits)) == 4, "Expected 4 Limits"

    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.limits)) == 4, "Expected 4 Limits in copy"
    suite_copy.delete_limit("limitName1")
    assert (
        len(list(suite_copy.limits)) == 3
    ), "Expected 3 limits since we just deleted one limitName1"
    suite_copy.delete_limit("")
    assert (
        len(list(suite_copy.limits)) == 0
    ), "Expected 0 limits since we just deleted all of them"


def test_limit_increment_and_node_paths():
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


def test_limit_copy_is_independent_and_decrement_resets():
    the_limit = ecflow.Limit("limitName1", 10)
    for path in ("/path1", "/path2", "/path3", "/path4"):
        the_limit.increment(1, path)
    limit_copy = copy.copy(the_limit)
    assert len(list(limit_copy.node_paths())) == 4

    for i, path in enumerate(("/path1", "/path2", "/path3", "/path4"), start=1):
        limit_copy.decrement(1, path)
        assert limit_copy.value() == 4 - i  # verify value after each decrement
    assert limit_copy.value() == 0
    assert len(list(limit_copy.node_paths())) == 0


def test_limit_same_path_reconsumes_single_token():
    limit_copy = ecflow.Limit("limitName1", 10)
    for _ in range(4):
        limit_copy.increment(1, "/path1")
    assert len(list(limit_copy.node_paths())) == 1
    assert limit_copy.value() == 1


def test_copy_suite_inlimits_independent():
    suite = ecflow.Suite("s1")
    suite.add_inlimit(ecflow.InLimit("limitName1", "/s1/f1", 2))
    suite.add_inlimit(ecflow.InLimit("limitName2", "/s1/f1", 2, True))
    suite.add_inlimit("limitName3", "/s1/f1", 2)
    suite.add_inlimit("limitName4", "/s1/f1", 2, True)
    assert len(list(suite.inlimits)) == 4

    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.inlimits)) == 4
    suite_copy.delete_inlimit("limitName1")
    assert len(list(suite_copy.inlimits)) == 3
    suite_copy.delete_inlimit("")
    assert len(list(suite_copy.inlimits)) == 0


def test_copy_task_trigger_and_complete():
    task = ecflow.Task("task")
    task.add_trigger("t2 == active")
    task.add_complete("t2 == complete")

    task_copy = copy.copy(task)
    assert task_copy.get_complete()
    assert task_copy.get_trigger()
    task_copy.delete_trigger()
    assert not task_copy.get_trigger()
    task_copy.delete_complete()
    assert not task_copy.get_complete()


def test_copy_task_trigger_from_expression():
    expr = ecflow.Expression("t1 == complete")
    task = ecflow.Task("task")
    task.add_trigger(expr)

    task_copy = copy.copy(task)
    assert task_copy.get_trigger()


def test_copy_task_compound_expression_trigger():
    expr = ecflow.Expression("t1 == complete")
    expr.add(ecflow.PartExpression("t1 == complete", True))
    expr.add(ecflow.PartExpression("t2 == complete", True))
    task = ecflow.Task("task")
    task.add_trigger(expr)

    task_copy = copy.copy(task)
    assert task_copy.get_trigger()


def test_copy_task_event_add_delete_find():
    task = ecflow.Task("task")
    task.add_event(ecflow.Event(1))
    task.add_event(2)
    task.add_event(ecflow.Event(10, "Eventname"))
    task.add_event(10, "Eventname2")
    task.add_event("fred")

    task_copy = copy.copy(task)
    event = task_copy.find_event("EVENT")
    assert event.empty()
    assert event.name() == ""
    assert event.value() == 0

    event = task_copy.find_event("1")
    assert not event.empty()
    assert event.number() == 1
    assert event.name() == ""
    assert event.value() == 0

    event = task_copy.find_event("2")
    assert not event.empty()
    assert event.number() == 2
    assert event.name() == ""
    assert event.value() == 0

    event = task_copy.find_event("10")
    assert not event.empty()
    assert event.number() == 10
    assert event.name() == "Eventname"
    assert event.value() == 0

    event = task_copy.find_event("fred")
    assert not event.empty()
    assert event.name() == "fred"
    assert event.value() == 0

    assert len(list(task_copy.events)) == 5
    task_copy.delete_event("1")
    assert len(list(task_copy.events)) == 4
    task_copy.delete_event("Eventname")
    assert len(list(task_copy.events)) == 3
    task_copy.delete_event("")
    assert len(list(task_copy.events)) == 0


def test_copy_task_meter_delete():
    task = ecflow.Task("task")
    task.add_meter(ecflow.Meter("metername1", 0, 100, 50))
    task.add_meter(ecflow.Meter("metername2", 0, 100))
    task.add_meter("metername3", 0, 100, 50)
    task.add_meter("metername4", 0, 100)

    task_copy = copy.copy(task)
    assert len(list(task_copy.meters)) == 4
    task_copy.delete_meter("metername1")
    assert len(list(task_copy.meters)) == 3
    task_copy.delete_meter("metername4")
    assert len(list(task_copy.meters)) == 2
    task_copy.delete_meter("")
    assert len(list(task_copy.meters)) == 0


def test_copy_task_queue_delete():
    queue_items = ["a", "b"]
    task = ecflow.Task("task")
    task.add_queue(ecflow.Queue("q1", queue_items))
    task.add_queue(ecflow.Queue("q2", queue_items))
    task.add_queue("q3", ["a", "b"])
    task.add_queue("q4", ["a", "b"])

    task_copy = copy.copy(task)
    assert len(list(task_copy.queues)) == 4
    task_copy.delete_queue("q1")
    assert len(list(task_copy.queues)) == 3
    task_copy.delete_queue("q2")
    assert len(list(task_copy.queues)) == 2
    task_copy.delete_queue("")
    assert len(list(task_copy.queues)) == 0


def test_copy_task_generic_delete():
    task = ecflow.Task("task")
    task.add_generic(ecflow.Generic("q1", ["a", "b"]))
    task.add_generic(ecflow.Generic("q2", ["a", "b"]))
    task.add_generic("q3", ["a", "b"])
    task.add_generic("q4", ["a", "b"])

    task_copy = copy.copy(task)
    assert len(list(task_copy.generics)) == 4
    task_copy.delete_generic("q1")
    assert len(list(task_copy.generics)) == 3
    task_copy.delete_generic("q2")
    assert len(list(task_copy.generics)) == 2
    task_copy.delete_generic("")
    assert len(list(task_copy.generics)) == 0


def test_copy_task_label_delete():
    task = ecflow.Task("task")
    task.add_label(ecflow.Label("label_name1", "value"))
    task.add_label(ecflow.Label("label_name2", "value"))
    task.add_label("label_name3", "value")
    task.add_label("label_name4", "value")

    task_copy = copy.copy(task)
    assert len(list(task_copy.labels)) == 4
    task_copy.delete_label("label_name1")
    assert len(list(task_copy.labels)) == 3
    task_copy.delete_label("label_name4")
    assert len(list(task_copy.labels)) == 2
    task_copy.delete_label("")
    assert len(list(task_copy.labels)) == 0


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
    ],
)
def test_copy_task_repeat_present_then_deleted(repeat_attr):
    task = ecflow.Task("task")
    task.add_repeat(repeat_attr)
    task_copy = copy.copy(task)
    assert not task_copy.get_repeat().empty(), "Expected repeat"
    task_copy.delete_repeat()
    assert task_copy.get_repeat().empty(), "Expected no repeat"


def test_copy_task_today_delete(time_series):
    task = ecflow.Task("task")
    task.add_today("00:30")
    task.add_today("+00:30")
    task.add_today("+00:30 20:00 01:00")
    task.add_today(ecflow.Today(time_series))
    task.add_today(ecflow.Today(0, 10))
    task.add_today(0, 59, True)
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 10)))
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 20), False))

    task_copy = copy.copy(task)
    assert len(list(task_copy.todays)) == 8
    deleting = [copy.copy(today) for today in task_copy.todays]
    for today in deleting:
        task_copy.delete_today(today)
    assert len(list(task_copy.todays)) == 0


def test_copy_task_time_delete(time_series):
    task = ecflow.Task("task")
    task.add_time("00:30")
    task.add_time("+00:30")
    task.add_time("+00:30 20:00 01:00")
    task.add_time(ecflow.Time(time_series))
    task.add_time(ecflow.Time(0, 10))
    task.add_time(0, 59, True)
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 10)))
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 20), False))

    task_copy = copy.copy(task)
    assert len(list(task_copy.times)) == 8
    deleting = [copy.copy(time) for time in task_copy.times]
    for time in deleting:
        task_copy.delete_time(time)
    assert len(list(task_copy.times)) == 0


def test_copy_task_date_delete():
    task = ecflow.Task("task")
    for i in [1, 2, 4, 8, 16]:
        task.add_date(i, 0, 0)
    task.add_date(ecflow.Date(1, 1, 2010))
    task.add_date(1, 1, 2010)
    task.add_date(ecflow.Date(2, 1, 2010))
    task.add_date(ecflow.Date(3, 1, 2010))
    task.add_date(ecflow.Date(4, 1, 2010))

    task_copy = copy.copy(task)
    assert len(list(task_copy.dates)) == 10
    deleting = [copy.copy(attr) for attr in task_copy.dates]
    for attr in deleting:
        task_copy.delete_date(attr)
    assert len(list(task_copy.dates)) == 0


def test_copy_task_day_delete():
    task = ecflow.Task("task")
    task.add_day(ecflow.Day(ecflow.Days.sunday))
    task.add_day(ecflow.Days.monday)
    task.add_day(ecflow.Days.tuesday)
    task.add_day("sunday")

    task_copy = copy.copy(task)
    assert len(list(task_copy.days)) == 4
    deleting = [copy.copy(attr) for attr in task_copy.days]
    for attr in deleting:
        task_copy.delete_day(attr)
    assert len(list(task_copy.days)) == 0


def test_copy_task_cron_delete():
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

    task_copy = copy.copy(task)
    assert len(list(task_copy.crons)) == 4
    deleting = [copy.copy(attr) for attr in task_copy.crons]
    for attr in deleting:
        task_copy.delete_cron(attr)
    assert len(list(task_copy.crons)) == 0


def test_copy_task_autocancel():
    t1 = ecflow.Task("t1")
    assert t1.get_autocancel() is None
    t1.add_autocancel(3)

    task_copy = copy.copy(t1)
    assert task_copy.get_autocancel() is not None


def test_copy_task_late():
    task = ecflow.Task("task")
    late = ecflow.Late()
    late.submitted(ecflow.TimeSlot(20, 10))
    late.active(ecflow.TimeSlot(20, 10))
    late.complete(ecflow.TimeSlot(20, 10), True)
    task.add_late(late)
    task_copy = copy.copy(task)
    assert task_copy.get_late() is not None


def test_copy_task_defstatus():
    task = ecflow.Task("task")
    task.add_defstatus(ecflow.DState.complete)
    task_copy = copy.copy(task)
    assert task_copy.get_defstatus() == ecflow.DState.complete


def test_copy_suite_clock():
    clock = ecflow.Clock(1, 1, 2010, False)
    clock.set_gain(1, 10, True)
    suite = ecflow.Suite("suite")
    assert suite.get_clock() is None
    suite.add_clock(clock)

    suite_copy = copy.copy(suite)
    assert suite_copy.get_clock() is not None


def test_copy_zombie_attributes():
    s1 = ecflow.Task("s1")
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
    task_copy = copy.copy(s1)
    assert len(list(task_copy.zombies)) == 6

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0

    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(
            zombie_type, child_list, ecflow.ZombieUserActionType.block
        )
        s1.add_zombie(zombie_attr)
    task_copy = copy.copy(s1)
    assert len(list(task_copy.zombies)) == 6

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0

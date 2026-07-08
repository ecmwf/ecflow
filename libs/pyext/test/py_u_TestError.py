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

import pytest

from ecflow import (
    Day,
    Date,
    Meter,
    Event,
    Queue,
    Clock,
    Variable,
    Label,
    Limit,
    InLimit,
    RepeatDate,
    RepeatDateTime,
    RepeatEnumerated,
    RepeatInteger,
    RepeatString,
    Task,
    Family,
    Suite,
    Defs,
    Trigger,
)


@pytest.mark.parametrize(
    "day, expected_valid",
    [
        ("monday", True),
        ("tuesday", True),
        ("wednesday", True),
        ("thursday", True),
        ("friday", True),
        ("saturday", True),
        ("sunday", True),
        ("", False),
        ("sunday1", False),
        ("2", False),
    ],
)
def test_day_validation(day, expected_valid):
    try:
        Day(day)
        assert expected_valid, f"Expected {day!r} to be invalid"
    except RuntimeError:
        assert not expected_valid, f"Expected {day!r} to be valid"


@pytest.mark.parametrize(
    "day, month, year, expected_valid",
    [
        (0, 1, 2010, True),
        (10, 0, 2010, True),
        (10, 1, 0, True),
        (0, 0, 0, True),
        (40, 1, 2010, False),
        (-10, 1, 2010, False),
        (1, 14, 2010, False),
        (1, -1, 2010, False),
        (1, 1, -2, False),
    ],
)
def test_date_validation(day, month, year, expected_valid):
    try:
        Date(day, month, year)
        assert expected_valid
    except IndexError:
        assert not expected_valid


@pytest.mark.parametrize(
    "str_date, expected_valid",
    [
        ("*.1.2010", True),
        ("10.*.2010", True),
        ("10.1.*", True),
        ("*.*.*", True),
        ("40.1.2010", False),
        ("-10.1.2010", False),
        ("1.14.2010", False),
        ("1.-1.2010", False),
        ("1.1.-2", False),
    ],
)
def test_date_string_validation(str_date, expected_valid):
    try:
        Date(str_date)
        assert expected_valid
    except (IndexError, RuntimeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "day, month, year, expected_valid",
    [
        (12, 1, 2010, True),
        (10, 1, 2010, True),
        (10, 1, 1400, True),
        (31, 12, 2010, True),
        (40, 1, 2010, False),
        (-10, 1, 2010, False),
        (1, 14, 2010, False),
        (1, -1, 2010, False),
        (1, 1, -2, False),
    ],
)
def test_clock_validation(day, month, year, expected_valid):
    try:
        Clock(day, month, year)
        assert expected_valid
    except IndexError:
        assert not expected_valid


@pytest.mark.parametrize(
    "min_m, max_m, color_change",
    [
        (0, 100, 100),
        (0, 100, 0),
    ],
)
def test_meter_valid_parameters(min_m, max_m, color_change):
    Meter("m", min_m, max_m, color_change)  # must not raise


@pytest.mark.parametrize(
    "min_m, max_m, color_change",
    [
        (200, 100, 100),  # min > max
        (0, 100, -20),  # color_change < min
        (0, 100, 200),  # color_change > max
    ],
)
def test_meter_invalid_numeric_bounds_raise_with_valid_name(min_m, max_m, color_change):
    # Use a VALID name so that the failure can only be due to the numeric bounds
    with pytest.raises((IndexError, RuntimeError)):
        Meter("m", min_m, max_m, color_change)


def test_meter_empty_name_raises():
    with pytest.raises((IndexError, RuntimeError)):
        Meter("", 0, 100, 100)


def test_meter_space_name_raises():
    with pytest.raises((IndexError, RuntimeError)):
        Meter(" ", 0, 100, 100)


@pytest.mark.parametrize(
    "name, queue_items, expected_valid",
    [
        ("m", ["a"], True),
        ("m", ["a", "b"], True),
        ("", ["a", "b"], False),
        (" ", ["a", "b"], False),
        (".", ["a", "b"], False),
        ("m", [], False),
    ],
)
def test_queue_validation(name, queue_items, expected_valid):
    try:
        Queue(name, queue_items)
        assert expected_valid
    except (IndexError, RuntimeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "number, name, expected_valid",
    [
        (1, None, True),
        (2, None, True),
        (2, "fred", True),
        (2, 2, False),
    ],
)
def test_event_validation(number, name, expected_valid):
    try:
        if name is None:
            Event(number)
        else:
            Event(number, name)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "start, end, step, expected_valid",
    [
        (20000101, 20001201, 200, True),
        (20001201, 20000101, 200, False),
        (200001011, 20001201, 200, False),
        (20000101, 200012013, 200, False),
        (00000000, 00000000, 200, False),
    ],
)
def test_repeat_date_validation(start, end, step, expected_valid):
    try:
        RepeatDate("m", start, end, step)
        assert expected_valid
    except RuntimeError:
        assert not expected_valid


@pytest.mark.parametrize("name", ["", " "])
def test_repeat_date_invalid_name_raises(name):
    with pytest.raises(RuntimeError):
        RepeatDate(name, 20000101, 20001201, 200)


@pytest.mark.parametrize(
    "start, end, step, expected_valid",
    [
        ("20000101T000000", "20001201T000000", "4800:00:00", True),
        ("20001201T000000", "20000101T000000", "4800:00:00", False),
        ("200001011T000000", "20001201T000000", "4800:00:00", False),
        ("20000101T000000", "200012013T000000", "4800:00:00", False),
        ("00000000T000000", "00000000T000000", "4800:00:00", False),
    ],
)
def test_repeat_datetime_validation(start, end, step, expected_valid):
    try:
        RepeatDateTime("m", start, end, step)
        assert expected_valid
    except RuntimeError:
        assert not expected_valid


@pytest.mark.parametrize("name", ["", " "])
def test_repeat_datetime_invalid_name_raises(name):
    with pytest.raises(RuntimeError):
        RepeatDateTime(name, "20000101T000000", "20001201T000000", "4800:00:00")


@pytest.mark.parametrize(
    "name, start, end, step, expected_valid",
    [
        ("name", 0, 10, 2, True),
        ("", 0, 10, 2, False),
        (" ", 0, 10, 2, False),
    ],
)
def test_repeat_integer_validation(name, start, end, step, expected_valid):
    try:
        RepeatInteger(name, start, end, step)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "name, values, expected_valid",
    [
        ("name", ["a"], True),
        ("", ["a"], False),
        (" ", ["a"], False),
        ("name", [1, 2], False),
        ("name", [], False),
    ],
)
def test_repeat_string_validation(name, values, expected_valid):
    try:
        RepeatString(name, values)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "name, values, expected_valid",
    [
        ("name", ["a"], True),
        ("", ["a"], False),
        (" ", ["a"], False),
        ("name", [1, 2], False),
        ("name", [], False),
    ],
)
def test_repeat_enumerated_validation(name, values, expected_valid):
    try:
        RepeatEnumerated(name, values)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "name, value, expected_valid",
    [
        ("name", "value", True),
        ("name", "", True),
        ("name", " ", True),
        ("name", "12", True),
        ("", "12", False),
        (" ", "12", False),
    ],
)
def test_variable_validation(name, value, expected_valid):
    try:
        Variable(name, value)
        assert expected_valid
    except RuntimeError:
        assert not expected_valid


@pytest.mark.parametrize(
    "name, value, expected_valid",
    [
        ("name", "value", True),
        ("name", "", True),
        ("name", " ", True),
        ("name", "12", True),
        ("", "12", False),
        (" ", "12", False),
    ],
)
def test_label_validation(name, value, expected_valid):
    try:
        Label(name, value)
        assert expected_valid
    except RuntimeError:
        assert not expected_valid


@pytest.mark.parametrize(
    "name, token, expected_valid",
    [
        ("name", 1, True),
        ("name", 20000, True),
        ("name", "ten", False),
        ("name", "2", False),
        ("", "2", False),
        (" ", "2", False),
    ],
)
def test_limit_validation(name, token, expected_valid):
    try:
        Limit(name, token)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "name, path, token, expected_valid",
    [
        ("limit_name", "/path/to/limit", 1, True),
        ("limit_name", "/path/to/limit", 999999, True),
        ("limit_name", "", 1, True),
        ("", "", 1, False),
        (" ", "", 1, False),
    ],
)
def test_inlimit_validation(name, path, token, expected_valid):
    try:
        InLimit(name, path, token)
        assert expected_valid
    except (RuntimeError, TypeError):
        assert not expected_valid


@pytest.mark.parametrize(
    "name, expected_valid",
    [(str(i), True) for i in range(25)]
    + [
        ("_", True),
        ("__", True),
        ("_._", True),
        ("1.2", True),
        ("fred.doc", True),
        ("_.1", True),
        (".", False),
        ("", False),
        (" ", False),
        ("   ", False),
        ("fred doc", False),
        ("1 ", False),
    ],
)
def test_node_name_validation(name, expected_valid):
    try:
        Task(name)
        Family(name)
        Suite(name)
        assert expected_valid
    except RuntimeError:
        assert not expected_valid


def test_defs_load_from_nonexistent_file_raises():
    with pytest.raises(RuntimeError):
        Defs("a_made_up_path_that_doesnt_not_exit.def")


def test_save_and_reload_defs(tmp_path):
    defs = Defs()
    defs.add_suite("s1")
    defs_file = tmp_path / "testerror.def"
    defs.save_as_defs(str(defs_file))
    assert Defs(str(defs_file))


def test_duplicate_suite_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        defs.add_suite("s1")
        defs.add_suite("s1")


def test_adding_today_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_today("00:30")


def test_adding_time_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_time("00:30")


def test_adding_date_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_date(1, 1, 2016)


def test_adding_day_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_day("monday")


def test_adding_trigger_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_trigger("1 == 0")


def test_adding_part_trigger_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_part_trigger("1 == 0")


def test_adding_complete_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_complete("1 == 0")


def test_adding_part_complete_at_suite_level_raises():
    with pytest.raises(RuntimeError):
        Defs().add_suite("1").add_part_complete("1 == 0")


def test_trigger_referencing_parent_without_dot_fails():
    defs = Defs()
    defs += Suite("obs") + Family("anon", Task("t1", Trigger("anon == complete")))
    assert (
        len(defs.check()) > 0
    ), "Adding a trigger referencing a parent without .. should fail"


def test_duplicate_family_not_allowed():
    suite = Suite("1")
    suite.add_family(Family("1"))
    with pytest.raises(RuntimeError):
        suite.add_family(Family("1"))


def test_duplicate_task_not_allowed():
    suite = Suite("1")
    suite.add_task(Task("a"))
    with pytest.raises(RuntimeError):
        suite.add_task(Task("a"))


def test_task_and_family_same_name_not_allowed():
    suite = Suite("1")
    suite.add_task(Task("a"))
    with pytest.raises(RuntimeError):
        suite.add_family(Family("a"))


def test_duplicate_meter_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_meter("meter", 0, 100)
        ta.add_meter("meter", 0, 100)


def test_duplicate_queue_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        suite = defs.add_suite("1")
        suite.add_queue("q", ["a"])
        suite.add_queue("q", ["a", "b"])


def test_duplicate_event_by_number_and_string_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event(1)
        ta.add_event("1")


def test_duplicate_named_event_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event("name")
        ta.add_event("name")


def test_duplicate_numbered_named_event_not_allowed():
    with pytest.raises(RuntimeError):
        defs = Defs()
        suite = defs.add_suite("1")
        ta = suite.add_task("a")
        ta.add_event(1, "name")
        ta.add_event(1, "name")


def test_cannot_add_same_suite_to_two_defs():
    defs1 = Defs()
    suite = defs1.add_suite("s1")
    with pytest.raises(RuntimeError):
        Defs().add_suite(suite)


def test_cannot_add_same_family_to_two_suites():
    defs3 = Defs()
    suite = defs3.add_suite("s1")
    family = suite.add_family("f1")
    with pytest.raises(RuntimeError):
        new_suite = Defs().add_suite("s2")
        new_suite.add_family(family)


def test_cannot_add_same_task_to_two_containers():
    defs4 = Defs()
    suite = defs4.add_suite("s1")
    family = suite.add_family("f1")
    task = family.add_task("t1")
    with pytest.raises(RuntimeError):
        Defs().add_suite("s1").add_task(task)


def test_cannot_add_two_autocancel_on_same_node():
    with pytest.raises(RuntimeError):
        suite = Defs().add_suite("s1")
        suite.add_autocancel(3)
        suite.add_autocancel(4)


def test_cannot_add_autocancel_and_autoarchive_together():
    with pytest.raises(RuntimeError):
        suite = Defs().add_suite("s1")
        suite.add_autoarchive(3)
        suite.add_autocancel(4)


def test_cannot_add_two_autorestore_on_same_node():
    with pytest.raises(RuntimeError):
        suite = Defs().add_suite("s1")
        suite.add_autorestore(["/s1"])
        suite.add_autorestore(["/s1"])

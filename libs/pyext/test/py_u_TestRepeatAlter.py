# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Tests the change of the begin, end, step and current value of Repeats from Python: the change() of the Repeat
# objects, and Node.change_repeat().

import pytest

import ecflow


def test_date_change():
    repeat = ecflow.RepeatDate("YMD", 20260101, 20261231)
    repeat.change("--begin 20260201 --end 20260630 --step 7 --current 20260215")
    assert (repeat.start(), repeat.end(), repeat.step()) == (20260201, 20260630, 7)
    assert repeat.current_value() == 20260215

    # the current value is kept within the range and on the step grid, towards the begin
    repeat.change("--begin 20260210")
    assert repeat.current_value() == 20260210
    repeat.change("20260217")
    repeat.change("--end 20260216")  # 17 is clamped to 16, off the grid of 10 and 7 days: moved to 10
    assert repeat.current_value() == 20260210


def test_datetime_change():
    repeat = ecflow.RepeatDateTime("DT", "20260101T000000", "20260102T000000", "06:00:00")
    repeat.change("20260101T180000")
    repeat.change("--step 04:00:00")
    assert repeat.current_value() == "20260101T160000"
    repeat.change("--begin 20260101T010000 --end 20260101T210000")
    assert repeat.current_value() == "20260101T130000"
    assert str(repeat).strip() == "repeat datetime DT 20260101T010000 20260101T210000 04:00:00"


def test_integer_change():
    repeat = ecflow.RepeatInteger("N", 0, 10)
    repeat.change("7")
    repeat.change("--step 3")
    assert repeat.current_value() == 6
    repeat.change("--begin 3 --end 20")
    assert (repeat.start(), repeat.end(), repeat.step()) == (3, 20, 3)
    assert repeat.current_value() == 6


def test_day_change():
    repeat = ecflow.RepeatDay(1)
    repeat.change("--step 2")
    assert str(repeat).strip() == "repeat day 2"
    repeat.change("5")  # a plain value is ignored
    assert str(repeat).strip() == "repeat day 2"
    with pytest.raises(RuntimeError, match="the option --begin does not apply to repeat day"):
        repeat.change("--begin 1")


def test_no_setters():
    for repeat in (
        ecflow.RepeatDate("YMD", 20260101, 20261231),
        ecflow.RepeatDateTime("DT", "20260101T000000", "20260102T000000"),
        ecflow.RepeatInteger("N", 0, 10),
        ecflow.RepeatDay(1),
    ):
        for setter in ("set_begin", "set_end", "set_step", "set_current"):
            assert not hasattr(repeat, setter)


def test_list_based_repeats_change_through_the_node_only():
    for repeat in (
        ecflow.RepeatString("S", ["a", "b"]),
        ecflow.RepeatEnumerated("E", ["a", "b"]),
        ecflow.RepeatDateList("D", [20260101]),
    ):
        assert not hasattr(repeat, "change")


def test_change_applies_the_options_together():
    repeat = ecflow.RepeatDate("YMD", 20250101, 20251231)
    # a begin after the current end needs the end to move at the same time
    with pytest.raises(RuntimeError, match="The end must be greater than the start date"):
        repeat.change("--begin 20260101")
    repeat.change("--begin 20260101 --end 20261231 --current 20260115")
    assert (repeat.start(), repeat.end(), repeat.current_value()) == (20260101, 20261231, 20260115)
    repeat.change("20260120")  # a plain value
    assert repeat.current_value() == 20260120


def test_refused_changes_leave_the_repeat_unchanged():
    repeat = ecflow.RepeatInteger("N", 0, 10, 2)
    repeat.change("4")
    for value, message in (
        ("--step 0", "the step cannot be zero"),
        ("--end -1", "the end cannot be less than the start"),
        ("5", "is not in line with the delta/step"),
        ("--current 12", "should be in the range"),
        ("--foo 1", "unknown option '--foo'"),
        ("--end 20 --step 0", "the step cannot be zero"),
    ):
        with pytest.raises(RuntimeError, match=message):
            repeat.change(value)
        assert str(repeat).strip() == "repeat integer N 0 10 2"
        assert repeat.current_value() == 4

    with pytest.raises(TypeError):
        repeat.change(20)  # a string is expected
    with pytest.raises(RuntimeError, match="is not a valid date"):
        ecflow.RepeatDate("YMD", 20260101, 20261231).change("--begin 20260230")
    with pytest.raises(RuntimeError, match="is not a valid date and time"):
        ecflow.RepeatDateTime("DT", "20260101T000000", "20260102T000000").change("--end garbage")


def test_a_node_holds_a_copy_of_the_repeat():
    task = ecflow.Task("t")
    repeat = ecflow.RepeatInteger("N", 0, 10)
    task.add_repeat(repeat)
    repeat.change("--end 20")
    assert task.get_repeat().end() == 10


def test_node_change_repeat():
    defs = ecflow.Defs()
    suite = defs.add_suite("s")
    task = suite.add_task("t")
    task.add_repeat(ecflow.RepeatDate("YMD", 20260101, 20261231))

    task.change_repeat("--begin 20260201 --end 20260228 --step 7")
    repeat = task.get_repeat()
    assert (repeat.start(), repeat.end(), repeat.step(), repeat.value()) == (20260201, 20260228, 7, 20260201)
    assert task.find_gen_variable("YMD_MM").value() == "02"

    task.change_repeat("20260215")  # a plain value
    assert task.get_repeat().value() == 20260215

    with pytest.raises(RuntimeError, match="is not in line with the delta/step"):
        task.change_repeat("--current 20260216")
    assert task.get_repeat().value() == 20260215

    string_task = suite.add_task("string")
    string_task.add_repeat(ecflow.RepeatString("S", ["a", "b"]))
    string_task.change_repeat("--current b")
    assert string_task.get_repeat().value() == 1
    with pytest.raises(RuntimeError, match="the option --begin does not apply to repeat string"):
        string_task.change_repeat("--begin 1")

    with pytest.raises(RuntimeError, match="Could not find repeat"):
        suite.add_task("none").change_repeat("1")

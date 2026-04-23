#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# Tests for RepeatDateTimeList attribute, which allows repeating over an arbitrary
# list of datetime instants in yyyymmddTHHMMSS format.

import copy
import os
import ecflow
import ecflow_test_util as Test


def test_construction():
    """Test basic construction and attribute access."""
    rep = ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000", "20240103T000000"])
    assert rep.name() == "DT", f"name should be 'DT' but got '{rep.name()}'"
    assert rep.start() is not None, "start() should return a value"
    assert rep.start() == 1704067200, "Expected 1704067200 (i.e. second since 19700101T000000 until 20240101T000000)"
    assert rep.end() is not None, "end() should return a value"
    assert rep.end() == 1704240000, "Expected 1704240000 (i.e. second since 19700101T000000 until 20240103T000000)"
    assert str(rep) != "", "str(rep) should not be empty"


def test_construction_single_element():
    """A single-element list is valid."""
    rep = ecflow.RepeatDateTimeList("X", ["20240315T103045"])
    assert rep.name() == "X"


def test_construction_error_empty_list():
    """Construction with an empty list must raise RuntimeError."""
    raised = False
    try:
        ecflow.RepeatDateTimeList("DT", [])
    except RuntimeError:
        raised = True
    assert raised, "RepeatDateTimeList with empty list should raise RuntimeError"


def test_construction_error_invalid_datetime():
    """Construction with an invalid datetime string must raise RuntimeError."""
    raised = False
    try:
        ecflow.RepeatDateTimeList("DT", ["not-a-datetime"])
    except RuntimeError:
        raised = True
    assert raised, "RepeatDateTimeList with invalid datetime string should raise RuntimeError"


def test_equality():
    """Two identically constructed RepeatDateTimeList objects must compare equal."""
    r1 = ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"])
    r2 = ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"])
    assert r1 == r2, "Equal RepeatDateTimeList objects should compare equal"


def test_inequality():
    """Objects with different lists must not compare equal."""
    r1 = ecflow.RepeatDateTimeList("DT", ["20240101T000000"])
    r2 = ecflow.RepeatDateTimeList("DT", ["20240102T000000"])
    assert r1 != r2, "RepeatDateTimeList objects with different lists should not compare equal"


def test_copy():
    """copy.copy() must produce an independent equal object."""
    rep = ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T120000"])
    rep_copy = copy.copy(rep)
    assert rep == rep_copy, "copy should equal original"
    assert rep is not rep_copy, "copy should be a different object"


def test_str_round_trip():
    """str(rep) must produce the canonical defs representation."""
    rep = ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T120000"])
    s = str(rep)
    assert "repeat datetimelist" in s, f"str should contain 'repeat datetimelist', got: {s}"
    assert "DT" in s, "str should contain repeat name"
    assert "20240101T000000" in s, "str should contain first datetime"
    assert "20240102T120000" in s, "str should contain second datetime"


def test_add_to_task_via_constructor():
    """RepeatDateTimeList can be added to a Task via the constructor (+= operator)."""
    t = ecflow.Task("t") + ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"])
    repeat = t.get_repeat()
    assert not repeat.empty(), "Task should have a non-empty repeat"


def test_add_repeat_method():
    """add_repeat() accepts RepeatDateTimeList."""
    t = ecflow.Task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"]))
    repeat = t.get_repeat()
    assert not repeat.empty(), "Expected non-empty repeat after add_repeat"


def test_delete_repeat():
    """delete_repeat() removes the RepeatDateTimeList."""
    t = ecflow.Task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"]))
    assert not t.get_repeat().empty(), "Expected repeat before delete"
    t.delete_repeat()
    assert t.get_repeat().empty(), "Expected no repeat after delete_repeat"


def test_generated_variables():
    """Generated datetime variables (_DATE, _YYYY, _MM, _DD, _JULIAN, _TIME, _HOURS, _MINUTES, _SECONDS)
    must be populated when the repeat is attached to a node in a Defs."""
    defs = ecflow.Defs()
    s = defs.add_suite("s")
    f = s.add_family("f")
    t = f.add_task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", ["20240315T103045"]))

    # Access via dot notation (triggers generated variable resolution)
    assert defs.s.f.t.DT is not None, "expected generated variable DT"
    assert defs.s.f.t.DT_DATE is not None, "expected generated variable DT_DATE"
    assert defs.s.f.t.DT_YYYY is not None, "expected generated variable DT_YYYY"
    assert defs.s.f.t.DT_MM is not None, "expected generated variable DT_MM"
    assert defs.s.f.t.DT_DD is not None, "expected generated variable DT_DD"
    assert defs.s.f.t.DT_JULIAN is not None, "expected generated variable DT_JULIAN"
    assert defs.s.f.t.DT_TIME is not None, "expected generated variable DT_TIME"
    assert defs.s.f.t.DT_HOURS is not None, "expected generated variable DT_HOURS"
    assert defs.s.f.t.DT_MINUTES is not None, "expected generated variable DT_MINUTES"
    assert defs.s.f.t.DT_SECONDS is not None, "expected generated variable DT_SECONDS"

    assert defs.s.f.t.DT.value() == "20240315T103045", \
        f"expected DT='20240315T103045' but got '{defs.s.f.t.DT.value()}'"
    assert defs.s.f.t.DT_DATE.value() == "20240315", \
        f"expected DT_DATE='20240315' but got '{defs.s.f.t.DT_DATE.value()}'"
    assert defs.s.f.t.DT_YYYY.value() == "2024", \
        f"expected DT_YYYY='2024' but got '{defs.s.f.t.DT_YYYY.value()}'"
    assert defs.s.f.t.DT_MM.value() == "03", \
        f"expected DT_MM='03' but got '{defs.s.f.t.DT_MM.value()}'"
    assert defs.s.f.t.DT_DD.value() == "15", \
        f"expected DT_DD='15' but got '{defs.s.f.t.DT_DD.value()}'"
    assert defs.s.f.t.DT_HOURS.value() == "10", \
        f"expected DT_HOURS='10' but got '{defs.s.f.t.DT_HOURS.value()}'"
    assert defs.s.f.t.DT_MINUTES.value() == "30", \
        f"expected DT_MINUTES='30' but got '{defs.s.f.t.DT_MINUTES.value()}'"
    assert defs.s.f.t.DT_SECONDS.value() == "45", \
        f"expected DT_SECONDS='45' but got '{defs.s.f.t.DT_SECONDS.value()}'"


def test_trigger_arithmetic():
    """Trigger expressions referencing a RepeatDateTimeList variable must evaluate correctly,
    using seconds-based arithmetic (86400 = 1 day)."""
    defs = ecflow.Defs()
    s = defs.add_suite("s")
    t1 = s.add_task("t1")
    t1.add_repeat(ecflow.RepeatDateTimeList("DT", ["20090101T000000", "20091231T000000"]))
    t2 = s.add_task("t2")
    t2.add_trigger("t1:DT ge 20100601T000000")

    assert len(defs.check()) == 0, "Expected no errors in trigger expressions"

    # Initial value is 20090101T000000, trigger should not fire
    assert not t2.evaluate_trigger(), "Trigger should not fire: 20090101T000000 < 20100601T000000"

    # Subtraction: 20090101T000000 - 86400 == 20081231T000000
    t2.change_trigger("t1:DT - 86400 eq 20081231T000000")
    assert t2.evaluate_trigger(), "Expected 20090101T000000 - 86400 == 20081231T000000"

    # Addition: 20090101T000000 + 86400 == 20090102T000000
    t2.change_trigger("t1:DT + 86400 eq 20090102T000000")
    assert t2.evaluate_trigger(), "Expected 20090101T000000 + 86400 == 20090102T000000"

    # After moving to second element (20091231T000000):
    # 20091231T000000 + 86400 == 20100101T000000
    t1.delete_repeat()
    t1.add_repeat(ecflow.RepeatDateTimeList("DT", ["20090131T000000", "20101231T000000"]))
    t2.change_trigger("t1:DT + 86400 eq 20090201T000000")
    assert t2.evaluate_trigger(), "Expected 20090131T000000 + 86400 == 20090201T000000"


def test_defs_check_no_errors():
    """A Defs with RepeatDateTimeList must pass the check with no errors."""
    defs = ecflow.Defs()
    s = defs.add_suite("s")
    t = s.add_task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000", "20240103T000000"]))
    errors = defs.check()
    assert len(errors) == 0, f"Expected no errors but got: {errors}"


def test_defs_str_contains_datetimelist():
    """Printing a Defs with RepeatDateTimeList must produce 'repeat datetimelist' output."""
    defs = ecflow.Defs()
    s = defs.add_suite("s")
    t = s.add_task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", ["20240101T000000", "20240102T000000"]))
    defs_str = str(defs)
    assert "repeat datetimelist" in defs_str, \
        f"Defs str should contain 'repeat datetimelist' but got:\n{defs_str}"


def test_first_and_last_value():
    """Repeat can be reset to start and moved to last value."""
    defs = ecflow.Defs()
    s = defs.add_suite("s")
    t = s.add_task("t")
    t.add_repeat(ecflow.RepeatDateTimeList("DT", [
        "20240101T000000", "20240102T000000", "20240103T000000"
    ]))
    rep = t.get_repeat()

    assert rep.start() == 1704067200, "Expected 1704067200 (i.e. second since 19700101T000000 until 20240101T000000)"
    assert rep.value() == rep.start(), "Expected initial value to be start()"
    assert rep.end() == 1704240000, "Expected 1704240000 (i.e. second since 19700101T000000 until 20240103T000000)"


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    test_construction()
    test_construction_single_element()
    test_construction_error_empty_list()
    test_construction_error_invalid_datetime()
    test_equality()
    test_inequality()
    test_copy()
    test_str_round_trip()
    test_add_to_task_via_constructor()
    test_add_repeat_method()
    test_delete_repeat()
    test_generated_variables()
    test_trigger_arithmetic()
    test_defs_check_no_errors()
    test_defs_str_contains_datetimelist()
    test_first_and_last_value

    print("All Tests pass")

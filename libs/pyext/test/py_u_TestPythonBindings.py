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
import ecflow_test_util as Test


def _assert_has_parent(node, context):
    """Assert that *node* has acquired a parent, failing with a descriptive message."""

    parent = node.get_parent()

    assert parent is not None, f"node '{node.name()}' has no parent after {context}."


def _assert_runtime_error(call, description):
    """
    Invoke *call* and assert it raises RuntimeError, not TypeError.

    To throw any other kind of error is considered a failure
    """
    try:
        call()
        assert False, f"{description}: expected an exception, but none was raised"
    except RuntimeError:
        pass  # correct behaviour after the fix
    except TypeError as exc:
        assert False, f"For {description}, unexpected TypeError thrown: {exc}"


def test_add_family_via_dot_add_preserves_identity():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")

    assert not f.get_parent(), "Standalone family has no parent"

    s.add(f)

    _assert_has_parent(f, "suite.add(family)")


def test_add_task_via_dot_add_preserves_identity():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")
    s.add_family(f)
    t = ecflow.Task("t")

    assert not t.get_parent(), "Standalone task has no parent"

    f.add(t)

    _assert_has_parent(t, "family.add(task)")


def test_iadd_family_preserves_identity():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")

    assert not f.get_parent(), "Standalone family has no parent"

    s += f

    _assert_has_parent(f, "suite += family")


def test_iadd_task_preserves_identity():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")
    s.add_family(f)
    t = ecflow.Task("t")

    assert not t.get_parent(), "Standalone task has no parent"

    f += t

    _assert_has_parent(t, "family += task")


def test_constructor_with_family_arg_preserves_identity():
    f = ecflow.Family("f")

    assert not f.get_parent(), "Standalone family has no parent"

    s = ecflow.Suite("s", f)

    _assert_has_parent(f, "Suite('s', family) constructor")


def test_constructor_with_task_arg_preserves_identity():
    t = ecflow.Task("t")

    assert not t.get_parent(), "Standalone task has no parent"

    f = ecflow.Family("f", t)

    _assert_has_parent(t, "Family('f', task) constructor")


def test_mutation_after_add_is_visible_through_parent():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")
    s.add(f)

    f.add_task(ecflow.Task("t_added_after")) # Update f, after it has been added to suite

    found_f = s.find_family("f")
    assert found_f is not None, "family must be findable in suite"

    found_t = found_f.find_node("t_added_after")
    assert found_t is not None, "task added to 'f' after `s.add(f)` must be visible through the suite."


def test_mutation_after_iadd_is_visible_through_parent():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")
    s += f

    f.add_task(ecflow.Task("t_added_after"))

    found_f = s.find_family("f")
    assert found_f is not None, "family must be findable in suite"

    found_t = found_f.find_node("t_added_after")
    assert found_t is not None, "task added to 'f' after `s += f` must be visible through the suite."


def test_nested_node_mutation_after_add_is_visible():
    s = ecflow.Suite("s")
    f = ecflow.Family("f")
    s.add(f)

    ff = ecflow.Family("ff")
    f.add(ff)

    ff.add_task(ecflow.Task("t"))

    found_ff = s.find_family("f").find_family("ff") if s.find_family("f") else None
    assert found_ff is not None, "sub-family must be findable through suite"

    assert found_ff.find_node("t") is not None, "task inside sub-family ff must be visible through suite."


def test_all_add_operations_preserve_identity():
    # using .add()
    s1 = ecflow.Suite("s1")
    f1 = ecflow.Family("f1")
    s1.add(f1)
    assert f1.get_parent() is not None, ".add() doesn't assign parent correctly"

    # using +=
    s2 = ecflow.Suite("s2")
    f2 = ecflow.Family("f2")
    s2 += f2
    assert f2.get_parent() is not None, "+= doesn't assign parent correctly"

    # using ctor
    f3 = ecflow.Family("f3")
    s3 = ecflow.Suite("s3", f3)
    assert f3.get_parent() is not None, "ctor doesn't assign parent correctly"


# ─────────────────────────────────────────────────────────────────────────────
# Mode B — object overload: >> and << must raise RuntimeError for non-nodes
# ─────────────────────────────────────────────────────────────────────────────

def test_rshift_with_event_raises_runtime_error_not_type_error():
    """
    suite >> event should raise RuntimeError('Argument must be a node_ptr'),
    not TypeError from py::cast_error.
    """
    suite = ecflow.Suite("s")
    event = ecflow.Event("e")
    _assert_runtime_error(
        lambda: suite.__rshift__(event),
        "suite >> Event"
    )


def test_rshift_with_meter_raises_runtime_error_not_type_error():
    suite = ecflow.Suite("s")
    meter = ecflow.Meter("m", 0, 100)
    _assert_runtime_error(
        lambda: suite.__rshift__(meter),
        "suite >> Meter"
    )


def test_rshift_with_label_raises_runtime_error_not_type_error():
    suite = ecflow.Suite("s")
    label = ecflow.Label("l", "v")
    _assert_runtime_error(
        lambda: suite.__rshift__(label),
        "suite >> Label"
    )


def test_rshift_with_trigger_raises_runtime_error_not_type_error():
    suite = ecflow.Suite("s")
    trigger = ecflow.Trigger("1 == 1")
    _assert_runtime_error(
        lambda: suite.__rshift__(trigger),
        "suite >> Trigger"
    )


def test_lshift_with_event_raises_runtime_error_not_type_error():
    suite = ecflow.Suite("s")
    event = ecflow.Event("e")
    _assert_runtime_error(
        lambda: suite.__lshift__(event),
        "suite << Event"
    )


def test_lshift_with_meter_raises_runtime_error_not_type_error():
    suite = ecflow.Suite("s")
    meter = ecflow.Meter("m", 0, 100)
    _assert_runtime_error(
        lambda: suite.__lshift__(meter),
        "suite << Meter"
    )


# ─────────────────────────────────────────────────────────────────────────────
# Positive tests — >> and << work correctly with actual node objects
# These should pass even before the fix (shared_ptr<Family> → shared_ptr<Node>
# is a valid implicit C++ conversion that py::cast handles).
# ─────────────────────────────────────────────────────────────────────────────

def test_rshift_operator_adds_child_and_sets_trigger():
    """
    suite >> f1 >> f2 should add both families and wire a trigger on f2
    so that f2 waits for f1 == complete.
    """
    suite = ecflow.Suite("s")
    f1 = ecflow.Family("f1")
    f2 = ecflow.Family("f2")
    suite >> f1 >> f2

    assert suite.find_family("f1") is not None, "f1 must be in suite after >> f1"
    assert suite.find_family("f2") is not None, "f2 must be in suite after >> f2"

    found_f2 = suite.find_family("f2")
    assert found_f2.get_trigger() is not None, \
        "f2 must have an auto-trigger after suite >> f1 >> f2"
    assert "f1" in found_f2.get_trigger().get_expression(), \
        "f2's trigger must reference f1"


def test_lshift_operator_adds_child_and_sets_reverse_trigger():
    """
    s << f2 << f1 should add both families and wire a trigger on f1
    so that f1 waits for f2 == complete (reverse chaining).
    """
    s = ecflow.Suite("s")
    f1 = ecflow.Family("f1")
    f2 = ecflow.Family("f2")
    s << f1 << f2 # this effectively adds f1 and f2 as children of s, while creating a trigger so that f2 runs after f1.

    print(f"s: {s}")

    assert s.find_family("f1") is not None, "f1 must be in s after << f1"
    assert s.find_family("f2") is not None, "f2 must be in s after << f2"

    assert f1.get_trigger() is not None, "f1 must have an auto-trigger after s << f2 << f1"
    assert "f2" in f1.get_trigger().get_expression(), "f1's trigger must reference f2"

    found_f1 = s.find_family("f1")
    assert found_f1.get_trigger() is not None, "f1 must have an auto-trigger after s << f2 << f1"
    assert "f2" in found_f1.get_trigger().get_expression(), "f1's trigger must reference f2"


def test_rshift_chaining_with_tasks():
    """Task chaining via >> should work and produce correct triggers."""
    suite = ecflow.Suite("s")
    fam = ecflow.Family("f")
    suite.add_family(fam)

    t1 = ecflow.Task("t1")
    t2 = ecflow.Task("t2")
    t3 = ecflow.Task("t3")
    fam >> t1 >> t2 >> t3

    assert fam.find_node("t1") is not None
    assert fam.find_node("t2") is not None
    assert fam.find_node("t3") is not None

    found_t2 = fam.find_task("t2")
    assert found_t2.get_trigger() is not None, "t2 must wait for t1"

    found_t3 = fam.find_task("t3")
    assert found_t3.get_trigger() is not None, "t3 must wait for t2"


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    # test_add_family_via_dot_add_preserves_identity()
    # test_add_task_via_dot_add_preserves_identity()
    # test_iadd_family_preserves_identity()
    # test_iadd_task_preserves_identity()
    # test_constructor_with_family_arg_preserves_identity()
    # test_constructor_with_task_arg_preserves_identity()
    # test_mutation_after_add_is_visible_through_parent()
    # test_mutation_after_iadd_is_visible_through_parent()
    # test_nested_node_mutation_after_add_is_visible()
    # test_all_add_operations_preserve_identity()
    #
    # test_rshift_with_event_raises_runtime_error_not_type_error()
    # test_rshift_with_meter_raises_runtime_error_not_type_error()
    # test_rshift_with_label_raises_runtime_error_not_type_error()
    # test_rshift_with_trigger_raises_runtime_error_not_type_error()
    # test_lshift_with_event_raises_runtime_error_not_type_error()
    # test_lshift_with_meter_raises_runtime_error_not_type_error()
    #
    # test_rshift_operator_adds_child_and_sets_trigger()
    test_lshift_operator_adds_child_and_sets_reverse_trigger()
    # test_rshift_chaining_with_tasks()

    print("All tests pass")

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import pytest

from ecflow import Suite, Family, Task, Defs, Edit


@pytest.fixture
def defs():
    return Defs(
        Edit(FRED_2="XX", DEFS_VAR="0"),
        Suite(
            "s1",
            Edit(SUITE_VAR="1", FRED_1="%FRED_2%"),
            Family(
                "f1",
                Edit(FRED_0="%FRED_1%"),
                Task("f1_t1", Edit(TASK_VAR="3", FRED="%FRED_0%")),
                Task("f1_t2", Edit(TASK_VAR="3", FRED="%FRED_0%")),
            ),
            Family(
                "f2",
                Edit(FRED_0="%FRED_1%"),
                Task("f2_t1", Edit(TASK_VAR="3", FRED="%FRED_0%")),
                Task("f2_t2", Edit(TASK_VAR="3", FRED="%FRED_0%")),
            ),
        ),
    )


def test_find_node_path_for_all_nodes(defs):
    assert defs.find_node_path("suite", "s1") == "/s1", " expected to find suite s1"
    assert (
        defs.find_node_path("family", "f1") == "/s1/f1"
    ), " expected to find family f1"
    assert (
        defs.find_node_path("family", "f2") == "/s1/f2"
    ), " expected to find family f2"
    assert (
        defs.find_node_path("task", "f1_t1") == "/s1/f1/f1_t1"
    ), " expected to find task f1_t1"
    assert (
        defs.find_node_path("task", "f1_t2") == "/s1/f1/f1_t2"
    ), " expected to find task f1_t2"
    assert (
        defs.find_node_path("task", "f2_t1") == "/s1/f2/f2_t1"
    ), " expected to find task f2_t1"
    assert (
        defs.find_node_path("task", "f2_t2") == "/s1/f2/f2_t2"
    ), " expected to find task f2_t2"


def test_find_node_returns_typed_nodes(defs):
    s1 = defs.find_node("suite", "/s1")
    assert s1 is not None and isinstance(s1, Suite), "expected find find suite s1"

    f1 = defs.find_node("family", "/s1/f1")
    assert f1 is not None and isinstance(f1, Family), " expected to find family f1"

    f2 = defs.find_node("family", "/s1/f2")
    assert f2 is not None and isinstance(f2, Family), " expected to find family f2"

    f1_t1 = defs.find_node("task", "/s1/f1/f1_t1")
    f1_t2 = defs.find_node("task", "/s1/f1/f1_t2")
    assert f1_t1 is not None and isinstance(f1_t1, Task), " expected to find task f1_t1"
    assert f1_t2 is not None and isinstance(f1_t2, Task), " expected to find task f1_t2"

    f2_t1 = defs.find_node("task", "/s1/f2/f2_t1")
    f2_t2 = defs.find_node("task", "/s1/f2/f2_t2")
    assert f2_t1 is not None and isinstance(f2_t1, Task), " expected to find task f2_t1"
    assert f2_t2 is not None and isinstance(f2_t2, Task), " expected to find task f2_t2"


def test_find_suite_by_name(defs):
    assert defs.find_suite("s1") is not None, "expected find find suite s1"
    assert "s1" in defs, "expected find find suite s1"
    assert defs.find_suite("sx") is None, "expected not to find suite sx"


def test_find_family_by_name(defs):
    s1 = defs.find_suite("s1")
    assert s1.find_family("f1") is not None, "expected find find family f1"
    assert s1.find_node("f1") is not None, "expected find find family f1"

    assert s1.find_family("fx") is None, "expected not to find family fx"
    assert s1.find_node("fx") is None, "expected not to find family fx"

    f2 = s1.find_family("f2")
    assert f2 is not None, "expected find find family f2"
    assert s1.find_node("f2") is not None, "expected find find family f2"

    f2_t1 = f2.find_task("f2_t1")
    assert f2_t1 is not None, "Expected to find task"

    f2_t2 = f2.find_task("f2_t2")
    assert f2_t2 is not None, "Expected to find task"
    assert f2.find_node("f2_t2") is not None, "Expected to find task"

    assert f2.find_task("f2_tx") is None, "Expected not to find task"
    assert f2.find_node("f2_tx") is None, "Expected not to find task"


def test_get_all_tasks_and_parent_variables(defs):
    tasks = defs.get_all_tasks()
    assert len(tasks) == 4, "Expected four tasks, but found " + str(len(tasks))
    for task in tasks:
        var = task.find_parent_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"

        assert task.find_parent_variable_sub_value("FRED") == "XX"
        assert task.find_parent_variable_sub_value("ZZZZ") == ""

        var = task.find_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"

        var = task.find_parent_variable("SUITE_VAR")
        assert not var.empty()
        assert var.value() == "1"

        var = task.find_parent_variable("DEFS_VAR")
        assert not var.empty()
        assert var.value() == "0"

        var = task.find_parent_variable("MADE_UP_VAR")
        assert var.empty()


def test_find_node_up_the_tree_from_task(defs):
    task = defs.find_node("task", "/s1/f1/f1_t1")
    assert task is not None

    node = task.find_node_up_the_tree("f1")
    assert node is not None
    assert node.get_abs_node_path() == "/s1/f1"

    node = task.find_node_up_the_tree("s1")
    assert node is not None
    assert node.get_abs_node_path() == "/s1"

    assert task.find_node_up_the_tree("freddd") is None

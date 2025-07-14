#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

from ecflow import Suite, Family, Task, Defs, Client, debug_build, Edit
import ecflow_test_util as Test
import os


def create_defs(name=""):
    suite_name = name
    if len(suite_name) == 0: suite_name = "s1"
    defs = Defs(Edit(FRED_2="XX", DEFS_VAR="0"),
                Suite(suite_name, Edit(SUITE_VAR="1", FRED_1="%FRED_2%"),
                      Family("f1", Edit(FRED_0="%FRED_1%"),
                             Task("f1_t1", Edit(TASK_VAR="3", FRED="%FRED_0%")),
                             Task("f1_t2", Edit(TASK_VAR="3", FRED="%FRED_0%"))
                             ),
                      Family("f2", Edit(FRED_0="%FRED_1%"),
                             Task("f2_t1", Edit(TASK_VAR="3", FRED="%FRED_0%")),
                             Task("f2_t2", Edit(TASK_VAR="3", FRED="%FRED_0%"))
                             )
                      )
                )
    return defs;


def test_ecflow_1213(defs):
    s1 = defs.find_node_path("suite", "s1")
    assert s1 == "/s1", " expected to find suite s1"

    f1 = defs.find_node_path("family", "f1")
    assert f1 == "/s1/f1", " expected to find family f1"

    f2 = defs.find_node_path("family", "f2")
    assert f2 == "/s1/f2", " expected to find family f2"

    f1_t1 = defs.find_node_path("task", "f1_t1")
    f1_t2 = defs.find_node_path("task", "f1_t2")
    assert f1_t1 == "/s1/f1/f1_t1", " expected to find task f1_t1"
    assert f1_t2 == "/s1/f1/f1_t2", " expected to find task f1_t2"

    f2_t1 = defs.find_node_path("task", "f2_t1")
    f2_t2 = defs.find_node_path("task", "f2_t2")
    assert f2_t1 == "/s1/f2/f2_t1", " expected to find task f2_t1"
    assert f2_t2 == "/s1/f2/f2_t2", " expected to find task f2_t2"

    # find_node
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
    assert f2_t2 is not None and isinstance(f2_t1, Task), " expected to find task f2_t2"


if __name__ == "__main__":

    Test.print_test_start(os.path.basename(__file__))

    defs = create_defs()

    test_ecflow_1213(defs)

    s1 = defs.find_suite("s1")
    assert s1 is not None, "expected find find suite s1"
    assert "s1" in defs, "expected find find suite s1"

    sx = defs.find_suite("sx")
    assert sx is None, "expected not to find suite sx"

    f1 = s1.find_family("f1")
    assert f1 is not None, "expected find find family f1"
    f1 = s1.find_node("f1")
    assert f1 is not None, "expected find find family f1"

    fx = s1.find_family("fx")
    assert fx is None, "expected not to find family fx"
    fx = s1.find_node("fx")
    assert fx is None, "expected not to find family fx"

    f2 = s1.find_family("f2")
    assert f2 is not None, "expected find find family f2"
    f2 = s1.find_node("f2")
    assert f2 is not None, "expected find find family f2"

    f2_t1 = f2.find_task("f2_t1")
    assert f2_t1 is not None, "Expected to find task"

    f2_t2 = f2.find_task("f2_t2")
    assert f2_t2 is not None, "Expected to find task"
    f2_t2 = f2.find_node("f2_t2")
    assert f2_t2 is not None, "Expected to find task"

    f2_tx = f2.find_task("f2_tx")
    assert f2_tx is None, "Expected not to find task"
    f2_tx = f2.find_node("f2_tx")
    assert f2_tx is None, "Expected not to find task"

    tasks = defs.get_all_tasks()
    assert len(tasks) == 4, "Expected four tasks, but found " + str(len(tasks))
    for task in tasks:
        # test find variable
        var = task.find_parent_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"

        var = task.find_parent_variable_sub_value("FRED")
        assert var == "XX"

        var = task.find_parent_variable_sub_value("ZZZZ")
        assert var == ""

        var = task.find_variable("TASK_VAR")
        assert not var.empty()
        assert var.value() == "3"

        var = task.find_parent_variable("SUITE_VAR");
        assert not var.empty()
        assert var.value() == "1"

        var = task.find_parent_variable("DEFS_VAR")
        assert not var.empty()
        assert var.value() == "0"

        var = task.find_parent_variable("MADE_UP_VAR")
        assert var.empty()

        if task.name() == "f1_t1":
            node = task.find_node_up_the_tree("f1")
            assert node is not None
            assert node.get_abs_node_path() == "/s1/f1"

            node = task.find_node_up_the_tree("s1")
            assert node is not None
            assert node.get_abs_node_path() == "/s1"

            node = task.find_node_up_the_tree("freddd")
            assert node is None

    print("All Tests pass")

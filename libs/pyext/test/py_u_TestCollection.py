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

import ecflow


@pytest.fixture
def empty_containers():
    return {
        "defs": ecflow.Defs(),
        "suite": ecflow.Suite("s"),
        "family": ecflow.Family("s"),
        "task": ecflow.Task("s"),
    }


@pytest.fixture
def single_item_containers():
    defs = ecflow.Defs()
    defs.add_suite("s1")
    suite = ecflow.Suite("s")
    suite.add_task("t")
    family = ecflow.Family("s")
    family.add_task("t")
    return {"defs": defs, "suite": suite, "family": family}


@pytest.fixture
def three_item_containers():
    defs = ecflow.Defs()
    suite = ecflow.Suite("s")
    family = ecflow.Family("s")
    [defs.add_suite("s" + str(i)) for i in [1, 2, 3]]
    [suite.add_family("f" + str(i)) for i in [1, 2, 3]]
    [family.add_task("t" + str(i)) for i in [1, 2, 3]]
    return {"defs": defs, "suite": suite, "family": family}


def test_empty_containers_have_length_zero(empty_containers):
    assert len(empty_containers["defs"]) == 0, "Expected empty defs to be of size zero"
    assert (
        len(empty_containers["suite"]) == 0
    ), "Expected empty suite to be of size zero"
    assert (
        len(empty_containers["family"]) == 0
    ), "Expected empty family to be of size zero"
    assert len(empty_containers["task"]) == 0, "Expected empty task to be of size zero"


def test_single_item_containers_have_length_one(single_item_containers):
    assert len(single_item_containers["defs"]) == 1, "Expected defs with one suite"
    assert len(single_item_containers["suite"]) == 1, "Expected suite with one child"
    assert len(single_item_containers["family"]) == 1, "Expected family with one child"


def test_three_item_containers_have_length_three(three_item_containers):
    assert (
        len(three_item_containers["defs"]) == 3
    ), "Expected defs with 3 suite but found " + str(len(three_item_containers["defs"]))
    assert (
        len(three_item_containers["suite"]) == 3
    ), "Expected suite with 3 child but found " + str(
        len(three_item_containers["suite"])
    )
    assert (
        len(three_item_containers["family"]) == 3
    ), "Expected family with 3 child but found " + str(
        len(three_item_containers["family"])
    )


def test_container_protocol_membership(three_item_containers, empty_containers):
    defs0 = empty_containers["defs"]
    defs3 = three_item_containers["defs"]
    assert "s" not in defs0, "Err in defs container"
    assert "z" not in defs3, "Err in defs container"
    assert "s1" in defs3, "Err in defs container"
    assert "s2" in defs3, "Err in defs container"
    assert "s3" in defs3, "Err in defs container"

    suite0 = empty_containers["suite"]
    suite3 = three_item_containers["suite"]
    assert "f" not in suite0, "Err in suites container"
    assert "z" not in suite3, "Err in suites container"
    assert "f1" in suite3, "Err in suites container"
    assert "f2" in suite3, "Err in suites container"
    assert "f3" in suite3, "Err in suites container"

    family0 = empty_containers["family"]
    family3 = three_item_containers["family"]
    assert "z" not in family0, "Err in family container"
    assert "t" not in family0, "Err in family container"
    assert "t1" in family3, "Err in family container"
    assert "t2" in family3, "Err in family container"
    assert "t3" in family3, "Err in family container"


def test_iterable_protocol_iteration(three_item_containers, empty_containers):
    defs3 = three_item_containers["defs"]
    suite3 = three_item_containers["suite"]
    family3 = three_item_containers["family"]
    task0 = empty_containers["task"]

    assert hasattr(defs3, "__iter__"), "defs has no __iter__"
    assert hasattr(suite3, "__iter__"), "suite has no __iter__"
    assert hasattr(family3, "__iter__"), "family has no __iter__"
    assert hasattr(task0, "__iter__"), "task has no __iter__ for aliases"
    assert [suite.name() for suite in defs3] == [
        "s1",
        "s2",
        "s3",
    ], "Defs iterator protocol not working"
    assert [child.name() for child in suite3] == [
        "f1",
        "f2",
        "f3",
    ], "Suites iterator protocol not working"
    assert [child.name() for child in family3] == [
        "t1",
        "t2",
        "t3",
    ], "Family iterator protocol not working"


def test_named_node_iteration(three_item_containers, empty_containers):
    defs3 = three_item_containers["defs"]
    suite3 = three_item_containers["suite"]
    family3 = three_item_containers["family"]
    task0 = empty_containers["task"]

    assert [suite.name() for suite in defs3.suites] == [
        "s1",
        "s2",
        "s3",
    ], "Defs.suites not working"
    assert [child.name() for child in suite3.nodes] == [
        "f1",
        "f2",
        "f3",
    ], "Suite.nodes working"
    assert [child.name() for child in family3.nodes] == [
        "t1",
        "t2",
        "t3",
    ], "Family.nodes not working"
    assert [alias.name() for alias in task0.nodes] == [], "Task.nodes not working"

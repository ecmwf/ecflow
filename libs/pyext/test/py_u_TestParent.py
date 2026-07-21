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

from ecflow import Suite, Family, Task, Defs


def _abs_node_path(node):
    """Given a node return its absolute node path by walking parents."""
    name_list = [node.name()]
    parent = node.get_parent()
    while parent:
        name_list.append(parent.name())
        parent = parent.get_parent()

    name_list.reverse()
    return "/" + "/".join(name_list)


@pytest.fixture
def populated_definition():
    suite = Suite("s1")
    family = Family("f1")
    task = Task("t1")

    suite.add_family(family)
    family.add_task(task)

    family2 = Family("f2")
    family.add_family(family2)

    t1 = Task("t1")
    family2.add_task(t1)

    defs = Defs()
    defs.add_suite(suite)
    return suite, family, family2, task, t1


def test_unattached_nodes_have_no_parent_or_defs():
    suite = Suite("s1")
    family = Family("f1")
    task = Task("t1")

    assert not suite.get_parent(), "Suite parent is always NULL"
    assert not family.get_parent(), "Expect no parent"
    assert not task.get_parent(), "Expect no parent"

    assert not suite.get_defs(), "Expected no defs, since suite not added to defs yet"
    assert not family.get_defs(), "Expected no defs"
    assert not task.get_defs(), "Expected no defs"


def test_attached_node_paths(populated_definition):
    suite, family, family2, task, t1 = populated_definition

    assert t1.get_abs_node_path() == _abs_node_path(t1), (
        "Expected " + t1.get_abs_node_path() + " but got " + _abs_node_path(t1)
    )
    assert family2.get_abs_node_path() == _abs_node_path(family2), (
        "Expected "
        + family2.get_abs_node_path()
        + " but got "
        + _abs_node_path(family2)
    )
    assert family.get_abs_node_path() == _abs_node_path(family), (
        "Expected " + family.get_abs_node_path() + " but got " + _abs_node_path(family)
    )
    assert suite.get_abs_node_path() == _abs_node_path(suite), (
        "Expected " + family.get_abs_node_path() + " but got " + _abs_node_path(suite)
    )

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that ecflow classes can be derived from.

import ecflow


class MyDefs(ecflow.Defs):
    pass


class MySuite(ecflow.Suite):
    pass


class MyFamily(ecflow.Family):
    pass


class MyTask(ecflow.Task):
    pass


class MyClient(ecflow.Client):
    pass


class MySuite2(ecflow.Suite):
    def __init__(self, name):
        super().__init__(name)


class MyFamily2(ecflow.Family):
    def __init__(self, name):
        super().__init__(name)


class MyTask2(ecflow.Task):
    def __init__(self, name):
        super().__init__(name)


def test_can_subclass_defs():
    assert isinstance(MyDefs(), ecflow.Defs)


def test_can_subclass_suite():
    assert isinstance(MySuite("s"), ecflow.Suite)


def test_can_subclass_family():
    assert isinstance(MyFamily("f"), ecflow.Family)


def test_can_subclass_task():
    assert isinstance(MyTask("t"), ecflow.Task)


def test_can_subclass_client():
    assert isinstance(MyClient(), ecflow.Client)


def test_can_subclass_suite_with_custom_init():
    node = MySuite2("s")
    assert isinstance(node, ecflow.Suite)
    assert node.name() == "s"


def test_can_subclass_family_with_custom_init():
    node = MyFamily2("f")
    assert isinstance(node, ecflow.Family)
    assert node.name() == "f"


def test_can_subclass_task_with_custom_init():
    node = MyTask2("t")
    assert isinstance(node, ecflow.Task)
    assert node.name() == "t"

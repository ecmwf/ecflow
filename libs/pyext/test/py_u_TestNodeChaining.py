#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import ecflow as ecf
import sys
import os
import ecflow_test_util as Test


def can_chain_adding_variables_using_dicts():
    t = ecf.Task("t")
    d1 = {'n1': 'v1', 'n10': 'v10', }
    d2 = {'n2': 'v2'}
    d3 = {'n3': 'v3'}
    t.add_variable(d1).add_variable(d2).add_variable(d3)

    assert t.find_variable("n1").value() == "v1", "expected variable n1 to have value v1"
    assert t.find_variable("n10").value() == "v10", "expected variable n10 to have value v10"
    assert t.find_variable("n2").value() == "v2", "expected variable n2 to have value v2"
    assert t.find_variable("n3").value() == "v3", "expected variable n3 to have value v3"


def can_chain_adding_variables_using_variable_objects():
    t = ecf.Task("t")
    v1 = ecf.Variable("name1", "value1")
    v2 = ecf.Variable("name2", "value2")
    v3 = ecf.Variable("name3", "value3")
    t.add_variable(v1).add_variable(v2).add_variable(v3)

    assert t.find_variable("name1").value() == "value1", "expected variable name1 to have value value1"
    assert t.find_variable("name2").value() == "value2", "expected variable name2 to have value value2"
    assert t.find_variable("name3").value() == "value3", "expected variable name3 to have value value3"


def can_chain_adding_variables_using_edit_objects():
    t = ecf.Task("t")
    e1 = ecf.Edit({"ea1": "ev1", "ea2": "ev2"})
    e2 = ecf.Edit(eb1="ev3")
    t.add_variable(e1).add_variable(e2)

    assert t.find_variable("ea1").value() == "ev1", "expected variable ea1 to have value ev1"
    assert t.find_variable("ea2").value() == "ev2", "expected variable ea2 to have value ev2"
    assert t.find_variable("eb1").value() == "ev3", "expected variable eb1 to have value ev3"


def can_chain_adding_variables_using_name_and_string_value():
    t = ecf.Task("t")
    t.add_variable("sv1", "hello").add_variable("sv2", "world").add_variable("sv3", "!")

    assert t.find_variable("sv1").value() == "hello", "expected variable sv1 to have value hello"
    assert t.find_variable("sv2").value() == "world", "expected variable sv2 to have value world"
    assert t.find_variable("sv3").value() == "!", "expected variable sv3 to have value !"


def can_chain_adding_variables_using_name_and_integer_value():
    t = ecf.Task("t")
    t.add_variable("iv1", 1).add_variable("iv2", 42).add_variable("iv3", 0)

    assert t.find_variable("iv1").value() == "1", "expected variable iv1 to have value 1"
    assert t.find_variable("iv2").value() == "42", "expected variable iv2 to have value 42"
    assert t.find_variable("iv3").value() == "0", "expected variable iv3 to have value 0"


def can_add_task_to_family_to_suite():
    s = ecf.Suite("s")
    f = s.add_family("f")
    t = f.add_task("t")

    assert t.name() == "t"
    assert f.name() == "f"
    assert s.name() == "s"


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    can_chain_adding_variables_using_dicts()
    can_chain_adding_variables_using_variable_objects()
    can_chain_adding_variables_using_edit_objects()
    can_chain_adding_variables_using_name_and_string_value()
    can_chain_adding_variables_using_name_and_integer_value()
    can_add_task_to_family_to_suite()

    print("All tests pass")

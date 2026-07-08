#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import ecflow


def test_suite_generated_variables():
    defs = ecflow.Defs()
    suite = defs.add_suite("s1")
    _ = suite.add_family("f1").add_task("t1")

    variable_list = ecflow.VariableList()
    suite.get_generated_variables(variable_list)

    for gen_var in variable_list:
        found = suite.find_gen_variable(gen_var.name())
        assert not found.empty(), "Could not find suite generated variable " + str(
            gen_var
        )

    assert len(list(variable_list)) == 16, "Expected 16 generated variables for suites"
    assert variable_list[0].name() == "SUITE", (
        "expected generated variable of name SUITE but found " + variable_list[0].name()
    )
    assert variable_list[0].value() == "s1", (
        "expected generated variable of value 's1' but found "
        + variable_list[0].value()
    )
    assert variable_list[1].name() == "ECF_DIRNAME", (
        "expected generated variable of name ECF_DIRNAME but found "
        + variable_list[1].name()
    )
    assert variable_list[1].value() == "", (
        "expected generated variable of value '' but found " + variable_list[1].value()
    )
    assert variable_list[2].name() == "ECF_BASENAME", (
        "expected generated variable of name ECF_BASENAME but found "
        + variable_list[2].name()
    )
    assert variable_list[2].value() == "s1", (
        "expected generated variable of value 's1' but found "
        + variable_list[2].value()
    )


def test_family_generated_variables():
    defs = ecflow.Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("f1")
    _ = family.add_task("t1")

    variable_list = ecflow.VariableList()
    family.get_generated_variables(variable_list)

    for gen_var in variable_list:
        found = family.find_gen_variable(gen_var.name())
        assert not found.empty(), "Could not find family generated variable " + str(
            gen_var
        )

    assert len(list(variable_list)) == 4, "Expected 4 generated variables for families"
    assert variable_list[0].name() == "FAMILY", (
        "expected generated variable of name FAMILY but found "
        + variable_list[0].name()
    )
    assert variable_list[0].value() == "f1", (
        "expected generated variable of value 'f1' but found "
        + variable_list[0].value()
    )
    assert variable_list[1].name() == "FAMILY1", (
        "expected generated variable of name FAMILY1 but found "
        + variable_list[1].name()
    )
    assert variable_list[1].value() == "f1", (
        "expected generated variable of value 'f1' but found "
        + variable_list[1].value()
    )
    assert variable_list[2].name() == "ECF_DIRNAME", (
        "expected generated variable of name ECF_DIRNAME but found "
        + variable_list[2].name()
    )
    assert variable_list[2].value() == "/s1", (
        "expected generated variable of value '/s1' but found "
        + variable_list[2].value()
    )
    assert variable_list[3].name() == "ECF_BASENAME", (
        "expected generated variable of name ECF_BASENAME but found "
        + variable_list[3].name()
    )
    assert variable_list[3].value() == "f1", (
        "expected generated variable of value 'f1' but found "
        + variable_list[3].value()
    )


def test_task_generated_variables():
    defs = ecflow.Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("f1")
    task = family.add_task("t1")

    variable_list = ecflow.VariableList()
    task.get_generated_variables(variable_list)

    for gen_var in variable_list:
        found = task.find_gen_variable(gen_var.name())
        assert not found.empty(), "Could not find task generated variable " + str(
            gen_var
        )

    assert len(list(variable_list)) == 10, "Expected 10 generated variables for tasks"
    assert variable_list[0].name() == "TASK", (
        "expected generated variable of name TASK but found " + variable_list[0].name()
    )
    assert variable_list[0].value() == "t1", (
        "expected generated variable of value 't1' but found "
        + variable_list[0].value()
    )
    assert variable_list[1].name() == "ECF_DIRNAME", (
        "expected generated variable of name ECF_DIRNAME but found "
        + variable_list[1].name()
    )
    assert variable_list[1].value() == "/s1/f1", (
        "expected generated variable of value '/s1/f1' but found "
        + variable_list[1].value()
    )
    assert variable_list[2].name() == "ECF_BASENAME", (
        "expected generated variable of name ECF_BASENAME but found "
        + variable_list[2].name()
    )
    assert variable_list[2].value() == "t1", (
        "expected generated variable of value 't1' but found "
        + variable_list[2].value()
    )
    assert variable_list[3].name() == "ECF_JOB", (
        "expected generated variable of name ECF_JOB but found "
        + variable_list[3].name()
    )
    assert variable_list[3].value() == "./s1/f1/t1.job0", (
        "expected generated variable of value './s1/f1/t1.job0' but found "
        + variable_list[3].value()
    )
    assert variable_list[4].name() == "ECF_SCRIPT", (
        "expected generated variable of name ECF_SCRIPT but found "
        + variable_list[4].name()
    )
    assert variable_list[4].value() == "./s1/f1/t1.ecf", (
        "expected generated variable of value './s1/f1/t1.ecf' but found "
        + variable_list[4].value()
    )
    assert variable_list[5].name() == "ECF_JOBOUT", (
        "expected generated variable of name ECF_JOBOUT but found "
        + variable_list[5].name()
    )
    assert variable_list[5].value() == "./s1/f1/t1.0", (
        "expected generated variable of value './s1/f1/t1.0' but found "
        + variable_list[5].value()
    )
    assert variable_list[6].name() == "ECF_TRYNO", (
        "expected generated variable of name ECF_TRYNO but found "
        + variable_list[6].name()
    )
    assert variable_list[6].value() == "0", (
        "expected generated variable of value '0' but found " + variable_list[6].value()
    )
    assert variable_list[7].name() == "ECF_RID", (
        "expected generated variable of name ECF_RID but found "
        + variable_list[7].name()
    )
    assert variable_list[7].value() == "", (
        "expected generated variable of value '' but found " + variable_list[7].value()
    )
    assert variable_list[8].name() == "ECF_NAME", (
        "expected generated variable of name ECF_NAME but found "
        + variable_list[8].name()
    )
    assert variable_list[8].value() == "/s1/f1/t1", (
        "expected generated variable of value '/s1/f1/t1' but found "
        + variable_list[8].value()
    )
    assert variable_list[9].name() == "ECF_PASS", (
        "expected generated variable of name ECF_PASS but found "
        + variable_list[9].name()
    )
    assert variable_list[9].value() == "", (
        "expected generated variable of value '' but found " + variable_list[9].value()
    )

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

import ecflow as ecf


@pytest.fixture
def suite_task():
    defs = ecf.Defs()
    suite = defs.add_suite("suite")
    suite.add_variable("VARIABLE", "value")
    family = suite.add_family("family")
    family.add_repeat(ecf.RepeatDate("REPEAT", 20010101, 20010102, 1))
    task = family.add_task("task")
    return suite, family, task


SUITE_GENERATED_VAR_NAMES = [
    "SUITE",
    "ECF_DATE",
    "ECF_CLOCK",
    "ECF_TIME",
    "ECF_JULIAN",
    "YYYY",
    "DD",
    "MM",
    "DAY",
    "MONTH",
    "DATE",
    "TIME",
    "DOW",
    "DOY",
]

FAMILY_GENERATED_VAR_NAMES = [
    "FAMILY",
    "FAMILY1",
    "REPEAT",
    "REPEAT_YYYY",
    "REPEAT_MM",
    "REPEAT_DD",
    "REPEAT_DOW",
    "REPEAT_JULIAN",
]

TASK_GENERATED_VAR_NAMES = [
    "TASK",
    "ECF_JOB",
    "ECF_SCRIPT",
    "ECF_JOBOUT",
    "ECF_TRYNO",
    "ECF_RID",
    "ECF_PASS",
    "ECF_NAME",
]


def test_retrieve_suite_generated_variables_using_variable_list(suite_task):
    suite, _, _ = suite_task
    vars = ecf.VariableList()
    suite.get_generated_variables(vars)
    names = {v.name() for v in vars}

    for expected_name in SUITE_GENERATED_VAR_NAMES:
        assert expected_name in names


def test_retrieve_suite_generated_variables_using_python_list(suite_task):
    suite, _, _ = suite_task
    vars = suite.get_generated_variables()
    names = {v.name() for v in vars}

    for expected_name in SUITE_GENERATED_VAR_NAMES:
        assert expected_name in names


def test_retrieve_family_generated_variables_using_variable_list(suite_task):
    _, family, _ = suite_task
    vars = ecf.VariableList()
    family.get_generated_variables(vars)
    names = {v.name() for v in vars}

    for expected_name in FAMILY_GENERATED_VAR_NAMES:
        assert expected_name in names


def test_retrieve_family_generated_variables_using_python_list(suite_task):
    _, family, _ = suite_task
    vars = family.get_generated_variables()
    names = {v.name() for v in vars}

    for expected_name in FAMILY_GENERATED_VAR_NAMES:
        assert expected_name in names


def test_retrieve_task_generated_variables_using_variable_list(suite_task):
    _, _, task = suite_task
    vars = ecf.VariableList()
    task.get_generated_variables(vars)
    names = {v.name() for v in vars}

    for expected_name in TASK_GENERATED_VAR_NAMES:
        assert expected_name in names


def test_retrieve_task_generated_variables_using_python_list(suite_task):
    _, _, task = suite_task
    vars = task.get_generated_variables()
    names = {v.name() for v in vars}

    for expected_name in TASK_GENERATED_VAR_NAMES:
        assert expected_name in names

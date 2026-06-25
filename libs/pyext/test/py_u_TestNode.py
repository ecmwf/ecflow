#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import ecflow_test_util as ect
import ecflow as ecf
import unittest
import sys
import os


class TestNode(unittest.TestCase):

    def setUp(self):
        self.defs = ecf.Defs()
        self.suite = self.defs.add_suite("suite");

        self.suite.add_variable("VARIABLE", "value");

        self.family = self.suite.add_family("family")
        self.family.add_repeat(ecf.RepeatDate("REPEAT", 20010101, 20010102, 1))

        self.task = self.family.add_task("task")

    def test_is_able_to_retrieve_suite_generated_variables_using_variable_list(self):
        vars = ecf.VariableList()
        self.suite.get_generated_variables(vars)

        names = [v.name() for v in vars]
        self.assertIn("SUITE", names)
        self.assertIn("ECF_DATE", names)
        self.assertIn("ECF_CLOCK", names)
        self.assertIn("ECF_TIME", names)
        self.assertIn("ECF_JULIAN", names)

        self.assertIn("YYYY", names)
        self.assertIn("DD", names)
        self.assertIn("MM", names)
        self.assertIn("DAY", names)
        self.assertIn("MONTH", names)
        self.assertIn("DATE", names)
        self.assertIn("TIME", names)
        self.assertIn("DOW", names)
        self.assertIn("DOY", names)

    def test_is_able_to_retrieve_suite_generated_variables_using_python_list(self):
        vars = self.suite.get_generated_variables()

        names = [v.name() for v in vars]
        self.assertIn("SUITE", names)
        self.assertIn("ECF_DATE", names)
        self.assertIn("ECF_CLOCK", names)
        self.assertIn("ECF_TIME", names)
        self.assertIn("ECF_JULIAN", names)

        self.assertIn("YYYY", names)
        self.assertIn("DD", names)
        self.assertIn("MM", names)
        self.assertIn("DAY", names)
        self.assertIn("MONTH", names)
        self.assertIn("DATE", names)
        self.assertIn("TIME", names)
        self.assertIn("DOW", names)
        self.assertIn("DOY", names)

    def test_is_able_to_retrieve_family_generated_variables_using_variable_list(self):
        vars = ecf.VariableList()
        self.family.get_generated_variables(vars)

        names = [v.name() for v in vars]
        self.assertIn("FAMILY", names)
        self.assertIn("FAMILY1", names)
        self.assertIn("REPEAT", names)
        self.assertIn("REPEAT_YYYY", names)
        self.assertIn("REPEAT_MM", names)
        self.assertIn("REPEAT_DD", names)
        self.assertIn("REPEAT_DOW", names)
        self.assertIn("REPEAT_JULIAN", names)

    def test_is_able_to_retrieve_family_generated_variables_using_python_list(self):
        vars = self.family.get_generated_variables()

        names = [v.name() for v in vars]
        self.assertIn("FAMILY", names)
        self.assertIn("FAMILY1", names)
        self.assertIn("REPEAT", names)
        self.assertIn("REPEAT_YYYY", names)
        self.assertIn("REPEAT_MM", names)
        self.assertIn("REPEAT_DD", names)
        self.assertIn("REPEAT_DOW", names)
        self.assertIn("REPEAT_JULIAN", names)

    def test_is_able_to_retrieve_task_generated_variables_using_variable_list(self):
        vars = ecf.VariableList()
        self.task.get_generated_variables(vars)

        names = [v.name() for v in vars]
        self.assertIn("TASK", names)
        self.assertIn("ECF_JOB", names)
        self.assertIn("ECF_SCRIPT", names)
        self.assertIn("ECF_JOBOUT", names)
        self.assertIn("ECF_TRYNO", names)
        self.assertIn("ECF_RID", names)
        self.assertIn("ECF_PASS", names)
        self.assertIn("ECF_NAME", names)

    def test_is_able_to_retrieve_task_generated_variables_using_python_list(self):
        vars = self.task.get_generated_variables()

        names = [v.name() for v in vars]
        self.assertIn("TASK", names)
        self.assertIn("ECF_JOB", names)
        self.assertIn("ECF_SCRIPT", names)
        self.assertIn("ECF_JOBOUT", names)
        self.assertIn("ECF_TRYNO", names)
        self.assertIn("ECF_RID", names)
        self.assertIn("ECF_PASS", names)
        self.assertIn("ECF_NAME", names)


if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that the nodes are printed using the expected format.

from ecflow import Defs, Suite, Task, Family, Client
import ecflow_test_util as Test
import os


def test_print_defs():
    defs = Defs()
    with Suite("s") as s:
        defs.add_suite(s)
        with Family("f") as f:
            s.add_family(f)
            with Task("t") as t:
                f.add_task(t)
                t.add_variable("NAME", "VALUE")

                expected = f"#{Client().version()}\nsuite s\n  family f\n    task t\n      edit NAME 'VALUE'\n  endfamily\nendsuite\n# enddef\n"
                actual = defs.__str__()
                assert actual == expected, "Expected:\n" + expected + "\n" + "Actual:\n" + actual


def test_print_suite():
    with Suite("s") as s:
        with Family("f") as f:
            s.add_family(f)
            with Task("t") as t:
                f.add_task(t)
                t.add_variable("NAME", "VALUE")

                expected = "suite s\n  family f\n    task t\n      edit NAME 'VALUE'\n  endfamily\nendsuite\n"
                actual = s.__str__()
                assert actual == expected, "Expected:\n" + expected + "\n" + "Actual:\n" + actual


def test_print_family():
    with Family("f") as f:
        with Task("t") as t:
            f.add_task(t)
            t.add_variable("NAME", "VALUE")

            expected = "  family f\n    task t\n      edit NAME 'VALUE'\n  endfamily\n"
            actual = f.__str__()
            assert actual == expected, "Expected:\n" + expected + "\n" + "Actual:\n" + actual


def test_print_task():
    with Task("t") as t:
        t.add_variable("NAME", "VALUE")

        expected = "  task t\n    edit NAME 'VALUE'\n"
        actual = t.__str__()
        assert actual == expected, "Expected:\n" + expected + "\n" + "Actual:\n" + actual


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    test_print_defs()
    test_print_suite()
    test_print_family()
    test_print_task()

    print("All tests pass")

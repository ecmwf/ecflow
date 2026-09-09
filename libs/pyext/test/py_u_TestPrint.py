# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# This test ensures that the nodes are printed using the expected format.

from ecflow import Defs, Suite, Task, Family, Client


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
                actual = str(defs)
                assert actual == expected, (
                    "Expected:\n" + expected + "\n" + "Actual:\n" + actual
                )


def test_print_suite():
    with Suite("s") as s:
        with Family("f") as f:
            s.add_family(f)
            with Task("t") as t:
                f.add_task(t)
                t.add_variable("NAME", "VALUE")

                expected = "suite s\n  family f\n    task t\n      edit NAME 'VALUE'\n  endfamily\nendsuite\n"
                actual = str(s)
                assert actual == expected, (
                    "Expected:\n" + expected + "\n" + "Actual:\n" + actual
                )


def test_print_family():
    with Family("f") as f:
        with Task("t") as t:
            f.add_task(t)
            t.add_variable("NAME", "VALUE")

            expected = "  family f\n    task t\n      edit NAME 'VALUE'\n  endfamily\n"
            actual = str(f)
            assert actual == expected, (
                "Expected:\n" + expected + "\n" + "Actual:\n" + actual
            )


def test_print_task():
    with Task("t") as t:
        t.add_variable("NAME", "VALUE")

        expected = "  task t\n    edit NAME 'VALUE'\n"
        actual = str(t)
        assert actual == expected, (
            "Expected:\n" + expected + "\n" + "Actual:\n" + actual
        )

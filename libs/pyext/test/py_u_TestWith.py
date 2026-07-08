#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures the use of the `with` statement, and thus runtime context, works as expected.

from ecflow import (
    Defs,
    Suite,
    Task,
    Family,
    Edit,
)


def test_context_manager_adds_hierarchy_to_defs():
    with Defs() as defs:
        with defs.add_suite("s2") as s2:
            with s2.add_family("f1") as f1:
                with f1.add_task("t1") as t1:
                    assert len(f1) == 1, "Expected 1 task"
            s2.add_family("f2")
            s2.add_task("t1")
        assert len(s2) == 3, "Expected 2 families and one task suite"
    assert len(defs) == 1, "Expected 1 suite"


def test_context_manager_on_suite():
    with Suite("s2") as s2:
        s2.add_family("f1")
        s2.add_task("t1")
        assert len(s2) == 2, "Expected 2 nodes family and task"


def test_context_manager_on_family():
    with Family("f1") as f1:
        f1.add_family("f2")
        f1.add_task("t1")
        assert len(f1) == 2, "Expected 2 nodes family and task"


def test_functional_add_equals_context_manager_add():
    defs1 = Defs()
    defs1.add_suite("s1").add_task("t1").add_variable("var", "v")
    defs1.add_suite("s2").add_family("f1").add_task("t1").add_variable("var", "v")
    defs1.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable(
        "var", "v"
    )

    with Defs() as defs2:
        with defs2.add_suite("s1") as s1:
            t1 = s1.add_task("t1")
            t1.add_variable("var", "v")
        with defs2.add_suite("s2") as s2:
            with s2.add_family("f1") as f1:
                with f1.add_task("t1") as t1:
                    t1.add_variable("var", "v")
        with defs2.add_suite("s3") as s3:
            with s3.add_family("f1") as f1:
                with f1.add_family("f2") as f2:
                    with f2.add_task("t1") as t1:
                        t1.add_variable("var", "v")

    assert defs1 == defs2, "expected defs to be the same"


def test_add_nodes_using_operators_equals_context_manager_add():
    defs1 = Defs()
    defs1.add_suite("s1").add_task("t1").add_variable("var", "v")
    defs1.add_suite("s2").add_family("f1").add_task("t1").add_variable("var", "v")
    defs1.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable(
        "var", "v"
    )

    with Defs() as defs3:
        with defs3.add_suite("s1") as s1:
            s1 + Task("t1", var="v")
        with defs3.add_suite("s2") as s2:
            with s2.add_family("f1") as f1:
                with f1.add_task("t1") as t1:
                    t1 += Edit(var="v")
        with defs3.add_suite("s3") as s3:
            with s3.add_family("f1") as f1:
                with f1.add_family("f2") as f2:
                    with f2.add_task("t1") as t1:
                        t1 += Edit(var="v")

    assert defs1 == defs3, "expected defs to be the same"

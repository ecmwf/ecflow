#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import os

import pytest

from ecflow import Defs, Suite, State, SState, Variable, Edit


@pytest.fixture
def temp_defs_file(tmp_path):
    file_name = str(tmp_path / "Test_default_defs.defs")
    defs = Defs() + Suite("s1")
    defs.save_as_defs(file_name)
    yield file_name
    try:
        os.remove(file_name)
    except OSError:
        pass


def test_defs_default_constructor(temp_defs_file):
    defs = Defs()
    assert defs.get_state() == State.unknown, "expected unknown state but found " + str(
        defs.get_state()
    )
    assert (
        defs.get_server_state() == SState.RUNNING
    ), "expected default server state to be running but found " + str(
        defs.get_server_state()
    )
    assert len(list(defs.suites)) == 0, "expected no suites but found " + str(
        len(list(defs.suites))
    )

    defs = Defs(temp_defs_file)
    assert defs.get_state() == State.unknown, "expected unknown state but found " + str(
        defs.get_state()
    )
    assert (
        defs.get_server_state() == SState.RUNNING
    ), "expected default server state to be running but found " + str(
        defs.get_server_state()
    )
    assert len(list(defs.suites)) == 1, "expected 1 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_filename_and_suite_raises():
    with pytest.raises(RuntimeError):
        Defs("file_name.def", Suite("s1"))


def test_defs_raw_constructor_with_filename_and_edit_raises():
    with pytest.raises(RuntimeError):
        Defs("file_name.def", Edit(a="b"))


def test_defs_raw_constructor_with_filename_and_dict_raises():
    with pytest.raises(RuntimeError):
        Defs("file_name.def", {"a": "b"})


def test_defs_raw_constructor_with_filename_and_variable_raises():
    with pytest.raises(RuntimeError):
        Defs("file_name.def", Variable("a", "b"))


def test_defs_raw_constructor_with_filename_and_variable_list_raises():
    with pytest.raises(RuntimeError):
        Defs("file_name.def", [Variable("a", "b")])


def test_defs_raw_constructor_with_single_suite():
    defs = Defs(Suite("s1"))
    assert len(list(defs.suites)) == 1, "expected 1 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_multiple_suites():
    defs = Defs(Suite("s1"), Suite("s2"), Suite("s3"))
    assert len(list(defs.suites)) == 3, "expected 3 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_suite_list():
    defs = Defs([Suite("s{0}".format(i)) for i in range(1, 5)])
    assert len(list(defs.suites)) == 4, "expected 4 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_edit():
    defs = Defs(Edit(a="b"))
    assert (
        len(list(defs.user_variables)) == 1
    ), "expected 1 user variable but found " + str(len(list(defs.user_variables)))


def test_defs_raw_constructor_with_variable():
    defs = Defs(Variable("a", "b"))
    assert (
        len(list(defs.user_variables)) == 1
    ), "expected 1 user variable but found " + str(len(list(defs.user_variables)))


def test_defs_raw_constructor_with_dict():
    defs = Defs({"a": "a", "b": "b"})
    assert (
        len(list(defs.user_variables)) == 2
    ), "expected 2 user variable but found " + str(len(list(defs.user_variables)))


def test_defs_raw_constructor_with_kwargs():
    defs = Defs(a="a", b="b")  # uses keyword arguments
    assert (
        len(list(defs.user_variables)) == 2
    ), "expected 2 user variable but found " + str(len(list(defs.user_variables)))


def test_defs_raw_constructor_with_mixed_variable_sources():
    defs = Defs(Edit(a="b"), Variable("b", "b"), {"c": "a", "d": "b"}, e="a", f="b")
    assert (
        len(list(defs.user_variables)) == 6
    ), "expected 6 user variable but found " + str(len(list(defs.user_variables)))


def test_defs_raw_constructor_with_suite_and_variables():
    defs = Defs(Suite("s1"), Edit(a="b"))
    assert (
        len(list(defs.user_variables)) == 1
    ), "expected 1 user variable but found " + str(len(list(defs.user_variables)))
    assert len(list(defs.suites)) == 1, "expected 1 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_multiple_suites_and_variable():
    defs = Defs(Suite("s1"), Suite("s2"), Suite("s3"), Variable("a", "b"))
    assert (
        len(list(defs.user_variables)) == 1
    ), "expected 1 user variable but found " + str(len(list(defs.user_variables)))
    assert len(list(defs.suites)) == 3, "expected 3 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_suite_list_and_dict():
    defs = Defs([Suite("s{0}".format(i)) for i in range(1, 5)], {"a": "a", "b": "b"})
    assert (
        len(list(defs.user_variables)) == 2
    ), "expected 2 user variable but found " + str(len(list(defs.user_variables)))
    assert len(list(defs.suites)) == 4, "expected 4 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_kwargs_suite_list_and_dict():
    defs = Defs(Suite("s1"), Suite("s2"), Suite("s3"), a="a", b="b")
    assert (
        len(list(defs.user_variables)) == 2
    ), "expected 2 user variable but found " + str(len(list(defs.user_variables)))
    assert len(list(defs.suites)) == 3, "expected 3 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_raw_constructor_with_all_sources():
    defs = Defs(
        Suite("s1"), Edit(a="b"), Variable("b", "b"), {"c": "a", "d": "b"}, e="a", f="b"
    )
    assert (
        len(list(defs.user_variables)) == 6
    ), "expected 6 user variable but found " + str(len(list(defs.user_variables)))
    assert len(list(defs.suites)) == 1, "expected 1 suites but found " + str(
        len(list(defs.suites))
    )


def test_defs_constructor_equivalent_to_plus_operator():
    defs = Defs(Suite("s1"), Edit(a="b"))
    defs2 = Defs() + Suite("s1") + Edit(a="b")
    defs3 = Defs()
    defs3 += Suite("s1")
    defs3 += Edit(a="b")
    assert defs == defs2, "defs not equal\n" + str(defs) + "\n\n" + str(defs2)
    assert defs == defs3, "defs not equal\n" + str(defs) + "\n\n" + str(defs3)


def test_defs_constructor_equivalent_to_add_call():
    defs = Defs(Suite("s1"), Suite("s2"), Suite("s3"), Variable("a", "b"))
    defs2 = Defs() + Suite("s1") + Suite("s2") + Suite("s3") + Variable("a", "b")
    defs3 = Defs().add(Suite("s1"), Suite("s2"), Suite("s3"), Variable("a", "b"))
    assert defs == defs2, "defs not equal\n" + str(defs) + "\n\n" + str(defs2)
    assert defs == defs3, "defs not equal\n" + str(defs) + "\n\n" + str(defs3)


def test_defs_constructor_with_list_and_dict_variants():
    defs = Defs([Suite("s{0}".format(i)) for i in range(1, 5)], {"a": "a"})
    defs2 = Defs() + [Suite("s{0}".format(i)) for i in range(1, 5)] + {"a": "a"}
    defs3 = Defs()
    for i in range(1, 5):
        defs3.add_suite("s{0}".format(i))
    defs3.add_variable("a", "a")
    assert defs == defs2, "defs not equal\n" + str(defs) + "\n\n" + str(defs2)
    assert defs == defs3, "defs not equal\n" + str(defs) + "\n\n" + str(defs3)


def test_defs_constructor_with_multiple_dict_variable_sources():
    defs = Defs(Suite("s1"), Edit(a="a"), Variable("b", "b"), {"c": "c"}, {"d": "d"})
    defs2 = Defs().add(
        Suite("s1"), Edit(a="a"), Variable("b", "b"), {"c": "c"}, {"d": "d"}
    )
    defs3 = Defs()
    defs3.add_suite("s1")
    defs3.add_variable("a", "a")
    defs3.add_variable("b", "b")
    defs3.add_variable("c", "c")
    defs3.add_variable("d", "d")
    assert defs == defs2, "defs not equal\n" + str(defs) + "\n\n" + str(defs2)
    assert defs == defs3, "defs not equal\n" + str(defs) + "\n\n" + str(defs3)

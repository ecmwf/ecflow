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

from ecflow import Late, TimeSlot


@pytest.fixture
def late_full():
    late = Late()
    late.submitted(TimeSlot(20, 10))
    late.active(TimeSlot(20, 10))
    late.complete(TimeSlot(20, 10), True)
    return late


@pytest.fixture
def late_complete_non_relative():
    late = Late()
    late.submitted(10, 10)
    late.active(10, 10)
    late.complete(10, 10, False)
    return late


@pytest.fixture
def late_submitted_only():
    late = Late()
    late.submitted(10, 10)
    return late


@pytest.fixture
def late_active_only():
    late = Late()
    late.active(10, 10)
    return late


@pytest.fixture
def late_complete_only():
    late = Late()
    late.complete(10, 12, False)
    return late


def test_late_constructor_with_all_keywords(late_full, late_complete_non_relative):
    assert late_full == Late(
        submitted="20:10", active="20:10", complete="+20:10"
    ), "late should be the same:\n" + str(late_full)

    assert late_complete_non_relative == Late(
        submitted="10:10", active="10:10", complete="10:10"
    ), "late should be the same:\n" + str(late_complete_non_relative)


def test_late_constructor_with_submitted_only(late_submitted_only):
    assert late_submitted_only == Late(
        submitted="10:10"
    ), "late should be the same:\n" + str(late_submitted_only)


def test_late_constructor_with_active_only(late_active_only):
    assert late_active_only == Late(active="10:10"), "late should be the same:\n" + str(
        late_active_only
    )


def test_late_constructor_with_complete_only(late_complete_only):
    assert late_complete_only == Late(
        complete="10:12"
    ), "late should be the same:\n" + str(late_complete_only)


def test_late_instances_are_not_equal(late_full, late_complete_non_relative):
    assert late_full != late_complete_non_relative, "Late should not be equal"


def test_late_with_no_arguments_raises():
    with pytest.raises(RuntimeError):
        Late("")  # no keyword arguments


def test_late_with_misspelt_submitted_keyword_raises():
    with pytest.raises(RuntimeError):
        Late(submittedd="20:10", active="20:10", complete="+20:10")  # misspelt keyword


def test_late_with_misspelt_active_keyword_raises():
    with pytest.raises(RuntimeError):
        Late(submitted="20:10", activee="20:10", complete="+20:10")  # misspelt keyword


def test_late_with_misspelt_complete_keyword_raises():
    with pytest.raises(RuntimeError):
        Late(submitted="20:10", active="20:10", completee="+20:10")  # misspelt keyword

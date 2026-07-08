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

from ecflow import Defs, Suite, Task, RepeatDateList, RepeatDateTimeList


@pytest.fixture
def suite():
    return Defs().add_suite("s1")


def test_suite_disallows_today_string(suite):
    with pytest.raises(RuntimeError):
        suite.add_today("00:30")


def test_suite_disallows_today_ints(suite):
    with pytest.raises(RuntimeError):
        suite.add_today(0, 30)


def test_suite_disallows_time_string(suite):
    with pytest.raises(RuntimeError):
        suite.add_time("+00:30")


def test_suite_disallows_time_ints(suite):
    with pytest.raises(RuntimeError):
        suite.add_time(0, 30)


def test_suite_disallows_date(suite):
    with pytest.raises(RuntimeError):
        suite.add_date(1, 1, 2010)


def test_suite_disallows_day(suite):
    with pytest.raises(RuntimeError):
        suite.add_day("sunday")


def test_repeat_date_list_empty_list_raises():
    task = Task("t")
    with pytest.raises(RuntimeError):
        task.add_repeat(RepeatDateList("date", []))


def test_repeat_datetime_list_empty_list_raises():
    task = Task("t")
    with pytest.raises(RuntimeError):
        task.add_repeat(RepeatDateTimeList("dt", []))


def test_repeat_datetime_list_invalid_datetime_raises():
    task = Task("t")
    with pytest.raises(RuntimeError):
        task.add_repeat(RepeatDateTimeList("dt", ["not-a-datetime"]))

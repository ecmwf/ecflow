# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import pytest

from ecflow import (
    Defs,
    Task,
    RepeatDateList,
    RepeatDateTimeList,
)


@pytest.fixture
def suite():
    defs = Defs()
    yield defs.add_suite("s1")


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

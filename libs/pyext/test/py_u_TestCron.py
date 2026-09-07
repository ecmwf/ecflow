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

from ecflow import Cron, TimeSlot, TimeSeries


@pytest.fixture
def cron1():
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])
    cron.set_time_series("+00:00 23:00 00:30")
    return cron


@pytest.fixture
def cron2():
    cron = Cron()
    cron.set_time_series(1, 30)  # default relative = false, added in release 4.0.7
    return cron


@pytest.fixture
def cron3():
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_time_series(1, 30, True)
    return cron


@pytest.fixture
def cron4():
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_time_series("00:30 01:30 00:01")
    return cron


@pytest.fixture
def cron5():
    cron = Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_time_series("+00:30")
    return cron


@pytest.fixture
def cron6():
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    time_series = TimeSeries(start, finish, incr, True)
    cron = Cron(time_series)
    cron.set_week_days([0, 1, 2, 3])
    return cron


@pytest.fixture
def cron7():
    cron = Cron()
    cron.set_week_days([0, 1, 2])  # This can't overlap with last_week_days_of_the_month
    cron.set_last_week_days_of_the_month(
        [3, 4, 5, 6]
    )  # This can't overlap with days_of_week
    cron.set_days_of_month([1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])
    cron.set_time_series("+00:00 23:00 00:30")
    cron.set_last_day_of_the_month()
    return cron


def test_cron_with_week_days_days_of_month_and_months(cron1):
    cron = Cron(
        "+00:00 23:00 00:30",
        days_of_week=[0, 1, 2, 3, 4, 5, 6],
        days_of_month=[1, 2, 3, 4, 5, 6],
        months=[1, 2, 3, 4, 5, 6],
    )
    assert cron1 == cron, "cron not equal\n" + str(cron1) + "\n" + str(cron)


def test_cron_with_time_series_only(cron2):
    cron = Cron("01:30")
    assert cron2 == cron, "cron not equal\n" + str(cron2) + "\n" + str(cron)


def test_cron_with_relative_time_series_and_week_days(cron3):
    cron = Cron("+01:30", days_of_week=[0, 1, 2, 3, 4, 5, 6])
    assert cron3 == cron, "cron not equal\n" + str(cron3) + "\n" + str(cron)


def test_cron_with_three_part_time_series_and_week_days(cron4):
    cron = Cron("00:30 01:30 00:01", days_of_week=[0, 1, 2, 3, 4, 5, 6])
    assert cron4 == cron, "cron not equal\n" + str(cron4) + "\n" + str(cron)


def test_cron_with_single_relative_time_series_and_week_days(cron5):
    cron = Cron("+00:30", days_of_week=[0, 1, 2, 3, 4, 5, 6])
    assert cron5 == cron, "cron not equal\n" + str(cron5) + "\n" + str(cron)


def test_cron_with_time_series_object(cron6):
    cron = Cron("+00:00 23:00 00:30", days_of_week=[0, 1, 2, 3])
    assert cron6 == cron, "cron not equal\n" + str(cron6) + "\n" + str(cron)


def test_cron_with_last_day_and_last_week_days(cron7):
    cron = Cron(
        "+00:00 23:00 00:30",
        days_of_week=[0, 1, 2],  # This can't overlap with last_week_days_of_the_month
        last_week_days_of_the_month=[
            3,
            4,
            5,
            6,
        ],  # This can't overlap with days_of_week
        days_of_month=[1, 2, 3, 4, 5, 6],
        months=[1, 2, 3, 4, 5, 6],
        last_day_of_the_month=True,
    )
    assert cron7 == cron, "cron not equal\n" + str(cron7) + "\n" + str(cron)


def test_cron_with_empty_string_raises():
    with pytest.raises(RuntimeError):
        Cron("")  # empty


def test_cron_with_empty_string_and_valid_kwargs_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "",
            days_of_week=[0, 1, 2, 3, 4, 5, 6],
            days_of_month=[1, 2, 3, 4, 5, 6],
            months=[1, 2, 3, 4, 5, 6],
        )  # empty


def test_cron_with_bad_time_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "0023",
            days_of_week=[0, 1, 2, 3, 4, 5, 6],
            days_of_month=[1, 2, 3, 4, 5, 6],
            months=[1, 2, 3, 4, 5, 6],
        )  # bad time


def test_cron_with_misspelt_days_of_week_keyword_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "00:10",
            days_of_weekss=[0, 1, 2, 3, 4, 5, 6],
            days_of_month=[1, 2, 3, 4, 5, 6],
            months=[1, 2, 3, 4, 5, 6],
        )  # bad kw,days_of_weekss


def test_cron_with_misspelt_days_of_month_keyword_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "00:10",
            days_of_week=[0, 1, 2, 3, 4, 5, 6],
            days_of_months=[1, 2, 3, 4, 5, 6],
            months=[1, 2, 3, 4, 5, 6],
        )  # bad kw, days_of_months


def test_cron_with_misspelt_months_keyword_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "00:10",
            days_of_week=[0, 1, 2, 3, 4, 5, 6],
            days_of_month=[1, 2, 3, 4, 5, 6],
            monthss=[1, 2, 3, 4, 5, 6],
        )  # bad kw, monthss


def test_cron_with_non_vector_days_of_week_raises():
    with pytest.raises(RuntimeError):
        Cron(
            "00:10",
            days_of_week="0,1,2,3,4,5,6",
            days_of_month=[1, 2, 3, 4, 5, 6],
            months=[1, 2, 3, 4, 5, 6],
        )  # bad kw, expect vector of int

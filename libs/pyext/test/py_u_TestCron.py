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
import ecflow as ecf
import itertools as it

import ecflow_test_util as Test


def can_create_cron_from_default_parameters():
    cron = ecf.Cron()

    cron.set_days_of_month([10, 20, 30])
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_months([1, 2, 3, 4, 5, 6])

    assert cron.days_of_month == [10, 20, 30]
    assert cron.week_days == [0, 1, 2, 3, 4, 5, 6]
    assert cron.months == [1, 2, 3, 4, 5, 6]


def can_create_cron_that_runs_every_day_at_2pm():
    cron = ecf.Cron('14:00')
    assert cron.time() == ecf.TimeSeries(14, 00)


def can_create_cron_that_runs_every_30_minutes_between_0am_and_8pm_on_the_first_six_days_of_the_months_from_january_until_july():
    cron = ecf.Cron('+00:00 20:00 00:30', days_of_month=[1, 2, 3, 4, 5, 6], months=[1, 2, 3, 4, 5, 6])

    assert cron.time() == ecf.TimeSeries(ecf.TimeSlot(0, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), True)
    assert cron.days_of_month == [1, 2, 3, 4, 5, 6]
    assert cron.months == [1, 2, 3, 4, 5, 6]


def can_create_cron_that_runs_relative_to_suite_start_time_or_task_requeue():
    cron = ecf.Cron('+00:15 23:00 00:30', days_of_week=[0, 1, 2], days_of_month=[4, 5, 6], months=[1, 2, 3])

    assert cron.time() == ecf.TimeSeries(ecf.TimeSlot(0, 15), ecf.TimeSlot(23, 0), ecf.TimeSlot(0, 30), True)
    assert cron.week_days == [0, 1, 2]
    assert cron.days_of_month == [4, 5, 6]
    assert cron.months == [1, 2, 3]

def can_create_cron_based_on_time_series():
    bgn = ecf.TimeSlot(0 , 0)
    end = ecf.TimeSlot(23, 0)
    inc = ecf.TimeSlot(0, 30)
    ts = ecf.TimeSeries(bgn, end, inc, True)

    cron = ecf.Cron(ts, days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6], months=[1,2])

    assert cron.time() == ts
    assert cron.week_days == [0,1,2,3,4,5,6]
    assert cron.days_of_month == [1,2,3,4,5,6]
    assert cron.months == [1,2]

def can_create_cron_with_defaults_and_set_time_series():
    cron = ecf.Cron()
    cron.set_time_series(1, 30, True)  # same as cron +01:30

    assert cron.time() == ecf.TimeSeries(1, 30, True)

def can_create_cron_that_runs_on_the_last_day_of_the_month():
    cron = ecf.Cron()
    cron.set_last_day_of_the_month()

    assert cron.last_day_of_the_month() == True

def can_create_cron_that_runs_on_the_last_weekdays_of_the_month():
    cron = ecf.Cron()
    cron.set_last_week_days_of_the_month([3, 4, 5])

    assert cron.last_week_days_of_the_month == [3, 4, 5]


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    can_create_cron_from_default_parameters()
    can_create_cron_that_runs_every_day_at_2pm()
    can_create_cron_that_runs_every_30_minutes_between_0am_and_8pm_on_the_first_six_days_of_the_months_from_january_until_july()
    can_create_cron_that_runs_relative_to_suite_start_time_or_task_requeue()
    can_create_cron_based_on_time_series()
    can_create_cron_with_defaults_and_set_time_series()
    can_create_cron_that_runs_on_the_last_day_of_the_month()
    can_create_cron_that_runs_on_the_last_weekdays_of_the_month()

    print("All tests pass")

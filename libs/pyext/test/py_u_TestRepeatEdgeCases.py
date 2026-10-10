# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Pins what the Python API reports for Repeats at the edges of their range.

import sys

import pytest

import ecflow


def test_value_past_the_end_is_reported_as_the_last_valid_value():
    task = ecflow.Task("t")
    task.add_repeat(ecflow.RepeatInteger("N", 0, 2, 1))
    for _ in range(3):
        task.get_repeat().increment()

    repeat = task.get_repeat()
    assert repeat.value() == 2  # the last valid value
    assert str(repeat.current_value()) == "3"  # the value held, past the end


def test_inconsistent_integer_reports_a_value_it_never_had():
    # the sign of the step disagrees with the order of start and end
    task = ecflow.Task("t")
    task.add_repeat(ecflow.RepeatInteger("N", 0, 10, -1))

    repeat = task.get_repeat()
    assert repeat.value() == 10
    assert str(repeat.current_value()) == "0"


@pytest.mark.skipif(
    sys.platform != "darwin",
    reason="the dates parse only with the lenient std::get_time of libc++",
)
def test_datetime_from_integers_takes_the_step_in_hours():
    repeat = ecflow.RepeatDateTime("DT", 20200101, 20200102)
    assert str(repeat).strip() == "repeat datetime DT 20200101T000000 20200102T000000 01:00:00"

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that the simulator performs as expected.

import os
import pathlib

import ecflow
import ecflow_test_util as Test


def _remove_logs(prefix):
    for log in pathlib.Path(".").glob(prefix + "*.log"):
        log.unlink()


def test_simulate_defs_with_time():
    defs = ecflow.Defs()
    suite = defs.add_suite("test_time_series")
    clock = ecflow.Clock(
        1, 1, 2011, False
    )  # day, month, year, hybrid - makes test start at midnight
    suite.add_clock(clock)
    family = suite.add_family("family")
    task = family.add_task("t1")
    ts = ecflow.TimeSeries(
        ecflow.TimeSlot(0, 0), ecflow.TimeSlot(23, 0), ecflow.TimeSlot(4, 0), True
    )
    task.add_time(ecflow.Time(ts))
    task.add_verify(
        ecflow.Verify(ecflow.State.complete, 6)
    )  # expect task to complete 6 times

    result = defs.simulate()
    assert len(result) == 0, (
        "Expected simulation to return without any errors, but found:\n" + result
    )

    _remove_logs("test_time_series")


def test_simulate_deadlock():
    defs = ecflow.Defs()
    suite = defs.add_suite("dead_lock")
    fam = suite.add_family("family")
    fam.add_task("t1").add_trigger("t2 == complete")
    fam.add_task("t2").add_trigger("t1 == complete")

    result = defs.simulate()
    assert len(result) != 0, "Expected simulation to return errors, but found none"

    _remove_logs("dead_lock")
    for name in ("defs.depth", "defs.flat"):
        try:
            os.remove(name)
        except OSError:
            pass


def test_simulate_time_series():
    # Initialise real clock on a Monday, such that task should _only_ run
    # on Monday since we are using a hybrid clock
    clock = ecflow.Clock(12, 10, 2009, False)  # 12 October 2009 was a Monday

    defs = ecflow.Defs()
    suite = defs.add_suite("test_time_series")
    suite.add_clock(clock)

    fam = suite.add_family("family")
    task = fam.add_task("t")

    task.add_time("00:30 23:59 04:00")
    task.add_verify(
        ecflow.Verify(ecflow.State.complete, 6)
    )  # expect task to complete 6 times

    result = defs.simulate()
    assert len(result) == 0, (
        "Expected simulation to return without any errors, but found:\n" + result
    )

    _remove_logs("test_time_series")


def test_simulate_all_good_defs():
    """Iterate over all good .def files from the simulator test data directory."""
    workspace_dir = ecflow.File.source_dir()
    csim_test_data = (
        pathlib.Path(workspace_dir)
        / "libs"
        / "test"
        / "simulator"
        / "test"
        / "data"
        / "good_defs"
    )
    assert (
        csim_test_data.exists()
    ), f"Simulator test data directory not found: {csim_test_data}"

    for path in Test.all_files(str(csim_test_data), "*.def"):
        defs = ecflow.Defs(path)
        result = defs.simulate()
        assert (
            len(result) == 0
        ), f"Expected simulation of {path} to return without errors, but found:\n{result}"

    for path in pathlib.Path(".").glob("*.log"):
        path.unlink()

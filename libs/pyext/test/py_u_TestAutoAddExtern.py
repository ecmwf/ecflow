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

from ecflow import Defs


@pytest.fixture
def defs():
    return Defs()


def _expect_check_fails_then_passes(defs):
    error_msg = defs.check()
    assert error_msg, "Expect error in checks\n" + str(defs)

    defs.auto_add_externs(True)
    error_msg = defs.check()
    assert not error_msg, (
        "Expect check to pass after auto add extern\n" + error_msg + "\n" + str(defs)
    )


def test_empty_defs_passes_check(defs):
    error_msg = defs.check()
    assert not error_msg, "Expect empty defs to pass check"


def test_auto_add_extern_for_trigger_path(defs):
    defs.add_suite("ext").add_family("f1").add_task("t").add_trigger(
        "/a/b/c/d == complete"
    )
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_event_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_task("t").add_trigger("/a/b/c/d == complete")
    suite.add_family("f2").add_task("t1").add_trigger("/a/b/c/d/e:event == set")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_bare_event_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_task("t").add_trigger("/a/b/c/d == complete")
    suite.add_family("f2").add_task("t1").add_trigger("/a/b/c/d/e:event == set")
    suite.add_family("f3").add_task("t2").add_trigger("/a/b/c/d/x:event")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_meter_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_task("t").add_trigger("/a/b/c/d == complete")
    suite.add_family("f2").add_task("t1").add_trigger("/a/b/c/d/e:event == set")
    suite.add_family("f3").add_task("t2").add_trigger("/a/b/c/d/x:event")
    suite.add_family("f4").add_task("t3").add_trigger("/a/b/c/d/y:meter le 30")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_flag_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_task("t").add_trigger("/a/b/c/d == complete")
    suite.add_family("f2").add_task("t1").add_trigger("/a/b/c/d/e:event == set")
    suite.add_family("f3").add_task("t2").add_trigger("/a/b/c/d/x:event")
    suite.add_family("f4").add_task("t3").add_trigger("/a/b/c/d/y:meter le 30")
    suite.add_family("f5").add_task("t4").add_trigger("/a/b/c/d/z<flag>late")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_family_inlimit(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_inlimit("hpcd", "limits")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_task_inlimit(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_inlimit("hpcd", "limits")
    suite.add_family("f2").add_task("t").add_inlimit("sg1", "/suiteName")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_task_inlimit_with_family_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_inlimit("hpcd", "limits")
    suite.add_family("f2").add_task("t").add_inlimit("sg1", "/suiteName")
    suite.add_family("f3").add_task("t1").add_inlimit("hpcd", "/obs/limits")
    _expect_check_fails_then_passes(defs)


def test_auto_add_extern_for_task_inlimit_with_suite_path(defs):
    suite = defs.add_suite("ext")
    suite.add_family("f1").add_inlimit("hpcd", "limits")
    suite.add_family("f2").add_task("t").add_inlimit("sg1", "/suiteName")
    suite.add_family("f3").add_task("t1").add_inlimit("hpcd", "/obs/limits")
    suite.add_family("f4").add_task("t2").add_inlimit("c1a", "/limits")
    _expect_check_fails_then_passes(defs)

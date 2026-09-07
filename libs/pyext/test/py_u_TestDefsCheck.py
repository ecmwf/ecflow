#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

from ecflow import Defs, Limit, InLimit


def test_trigger_with_mismatched_brackets_produces_check_error():
    defs = Defs()
    suite = defs.add_suite("s1")
    suite.add_task("t1").add_trigger("t2 == active)")
    error = defs.check()
    assert (
        error
    ), "Expected Error: triggers fail parse, miss-matched brackets in expression."


def test_inlimit_value_exceeds_limit_value_produces_check_error():
    defs = Defs()
    suite = defs.add_suite("s1")
    suite.add_limit(Limit("disk", 50))
    family = suite.add_family("anon")
    family.add_inlimit(InLimit("disk", "/s1", 100))
    family.add_task("t1")
    error = defs.check()
    assert (
        error
    ), "Expected Error: since inlimit value('100') is greater than the LIMIT value('50')"


def test_inlimit_with_missing_limit_path_produces_check_warning():
    defs = Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("anon")
    family.add_inlimit(InLimit("disk", "/s1", 100))
    family.add_task("t1")
    error = defs.check()
    assert (
        error
    ), "Expected warning: since inlimit PATH(/s1:disk) reference a limit('disk') that does not exist"


def test_inlimit_with_extern_resolves_warning():
    defs = Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("anon")
    family.add_inlimit(InLimit("disk", "/s1", 100))
    family.add_task("t1")
    assert defs.check(), "Expected warning before adding extern"
    defs.add_extern("/s1:disk")
    assert not defs.check(), (
        "Expected no warnings, since extern specified: " + defs.check()
    )


def test_inlimit_without_path_produces_check_warning():
    defs = Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("anon")
    family.add_inlimit(InLimit("disk", "", 100))
    family.add_task("t1")
    error = defs.check()
    assert (
        error
    ), "Expected warning: since inlimit SHOULD reference a limit('disk') somewhere UP parent hierarchy"


def test_inlimit_without_path_with_extern_resolves_warning():
    defs = Defs()
    suite = defs.add_suite("s1")
    family = suite.add_family("anon")
    family.add_inlimit(InLimit("disk", "", 100))
    family.add_task("t1")
    assert defs.check(), "Expected warning before adding extern"
    defs.add_extern("disk")
    assert not defs.check(), "Expected no warnings, since extern specified"

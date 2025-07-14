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
from ecflow import Defs, Limit, InLimit, Client, debug_build
import ecflow_test_util as Test

if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    defs = Defs()
    suite = defs.add_suite("s1");
    suite.add_task("t1").add_trigger("t2 == active)")
    theCheckValue = defs.check();
    print("Message: '" + theCheckValue + "'")
    assert len(theCheckValue) != 0, "Expected Error: triggers fail parse, miss-matched brackets in expression."

    # The number of tokens specified on the in-limit must be less than or equal to those specified on the LIMIT
    defs = Defs()
    suite = defs.add_suite("s1");
    suite.add_limit(Limit("disk", 50));
    family = suite.add_family("anon");
    family.add_inlimit(InLimit("disk", "/s1", 100))
    family.add_task("t1");
    theCheckValue = defs.check();
    print("Message: '" + theCheckValue + "'")
    assert len(theCheckValue) != 0, "Expected Error: since inlimit value('100') is greater than the LIMIT value('50')"

    # When a path is specified on the in-limit we search for the limit on that path, otherwise the extern's
    defs = Defs()
    suite = defs.add_suite("s1");
    family = suite.add_family("anon");
    family.add_inlimit(InLimit("disk", "/s1", 100))
    family.add_task("t1");
    theCheckValue = defs.check();
    print("Message: '" + theCheckValue + "'")
    assert len(theCheckValue) != 0, "Expected warning: since inlimit PATH(/s1:disk) reference a limit('disk') that does not exist"

    # Add as extern and repeat the check
    defs.add_extern("/s1:disk")
    assert len(defs.check()) == 0, "Expected no warnings, since extern specified: " + defs.check()

    # When no path is specified on the in-limit, we search for the limit up the node tree, otherwise the extern's
    defs = Defs()
    suite = defs.add_suite("s1");
    family = suite.add_family("anon");
    family.add_inlimit(InLimit("disk", "", 100))
    family.add_task("t1");
    theCheckValue = defs.check();
    print("Message: '" + theCheckValue + "'")
    assert len(theCheckValue) != 0, "Expected warning: since inlimit SHOULD reference a limit('disk') somewhere UP parent hierarchy"

    # Add as extern and repeat the check
    defs.add_extern("disk")
    assert len(defs.check()) == 0, "Expected no warnings, since extern specified"

    print("All Tests pass")

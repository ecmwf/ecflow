#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures the Trigger evaluation uses date arithmetic whenever
# the expression references a repeat DATE variable.
#
# Addition and subtraction for repeat date should follow date arithmetic.
# The repeat variable in the expression must be on the LHS (left-hand side)
# of the expression, otherwise integer arithmetic is used.

import os
import ecflow
import ecflow_test_util as Test


def test_repeat_arithmetic(repeat_to_add, repeat_to_add2):
    defs = ecflow.Defs()
    s1 = defs.add_suite("s1");
    t1 = s1.add_task("t1").add_repeat(repeat_to_add);
    t2 = s1.add_task("t2").add_trigger("t1:YMD ge 20100601");

    # Check trigger expressions
    assert len(defs.check()) == 0, "Expected no errors in parsing expressions."

    # Initial value of repeat is 20090101 hence trigger should fail to evaluate
    assert t2.evaluate_trigger() == False, "Expected trigger to evaluate. 20090101 >= 20100601"

    # Check end of month - 1
    t2.change_trigger("t1:YMD - 1 eq 20081231")
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101 - 1  == 20081231"

    # check addition
    t2.change_trigger("t1:YMD + 1 eq 20090102");
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101 + 1  == 20090102"

    # Check the end of each month + 1
    t1.delete_repeat();
    t1.add_repeat(repeat_to_add2)
    t2.change_trigger("t1:YMD + 1 eq 20090201");
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090131 + 1  == 20090201"


def test_repeat_datetime_arithmetic(repeat_to_add, repeat_to_add2):
    defs = ecflow.Defs()
    s1 = defs.add_suite("s1")
    t1 = s1.add_task("t1").add_repeat(repeat_to_add)
    t2 = s1.add_task("t2").add_trigger("t1:DT ge 20100601T000000")

    # Check trigger expressions
    print(f"## defs: {defs}")
    assert len(defs.check()) == 0, "Expected no errors in parsing expressions."

    # Initial value of repeat is 20090101 hence trigger should fail to evaluate
    assert t2.evaluate_trigger() == False, "Expected trigger to evaluate. 20090101T000000 >= 20100601T000000"

    # Check end of month - 1 day
    t2.change_trigger("t1:DT - 86400 eq 20081231T000000")
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101T000000 - 86400  == 20081231T000000"

    # check addition
    t2.change_trigger("t1:DT + 86400 eq 20090102T000000");
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090101T000000 + 86400  == 20090102T000000"

    # Check the end of each month + 1 day
    t1.delete_repeat();
    t1.add_repeat(repeat_to_add2)
    t2.change_trigger("t1:DT + 86400 eq 20090201T000000");
    assert t2.evaluate_trigger(), "Expected trigger to evaluate. 20090131 + 86400  == 20090201T000000"


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    test_repeat_arithmetic(ecflow.RepeatDate("YMD", 20090101, 20091231, 1),
                           ecflow.RepeatDate("YMD", 20090131, 20101231, 1))
    test_repeat_arithmetic(ecflow.RepeatDateList("YMD", [20090101, 20091231]),
                           ecflow.RepeatDateList("YMD", [20090131, 20101231]))

    test_repeat_datetime_arithmetic(ecflow.RepeatDateTime("DT", "20090101T000000", "20091231T000000", "24:00:00"),
                                    ecflow.RepeatDateTime("DT", "20090131T000000", "20101231T000000", "24:00:00"))

    print("All Tests pass")

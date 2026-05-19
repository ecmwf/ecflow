#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import ecflow as ecf
import sys
import os
import ecflow_test_util as Test

flag_values = [ecf.FlagType.force_abort,
               ecf.FlagType.user_edit,
               ecf.FlagType.task_aborted,
               ecf.FlagType.edit_failed,
               ecf.FlagType.jobcmd_failed,
               ecf.FlagType.killcmd_failed,
               ecf.FlagType.statuscmd_failed,
               ecf.FlagType.no_script,
               ecf.FlagType.killed,
               ecf.FlagType.status,
               ecf.FlagType.late,
               ecf.FlagType.message,
               ecf.FlagType.byrule,
               ecf.FlagType.queuelimit,
               ecf.FlagType.wait,
               ecf.FlagType.locked,
               ecf.FlagType.zombie,
               ecf.FlagType.no_reque,
               ecf.FlagType.archived,
               ecf.FlagType.restored,
               ecf.FlagType.threshold,
               ecf.FlagType.sigterm,
               ecf.FlagType.log_error,
               ecf.FlagType.checkpt_error,
               ecf.FlagType.remote_error]


def can_create_flag_from_default_parameters():
    flag = ecf.Flag()
    assert str(flag) == "", "expected empty string, for an empty flag"

    flag_values = flag.list()  # flag_values is of type FlagTypeVec
    print(f"Flags: {len(flag_values)}")
    for flag_value in flag_values: print(" * ", flag.type_to_string(flag_value))

    assert len(flag_values) == 25


def can_set_clear_reset_flag():
    flag = ecf.Flag()
    assert str(flag) == "", "expected empty string, for an empty flag"

    for flag_value in flag_values:
        # Set the flag
        flag.set(flag_value)
        assert flag.is_set(flag_value), "expected flag %r to be set" % flag.type_to_string(flag_value)
        # Clear the flag
        flag.clear(flag_value)
        assert not flag.is_set(flag_value), "expected flag {0} not to be set".format(flag.type_to_string(flag_value))

        # Set the flag, again
        flag.set(flag_value)
        assert flag.is_set(flag_value), "expected flag %r to be set" % flag.type_to_string(flag_value)

    # After the iteration above, all flags should be set
    assert str(flag) == "force_aborted,user_edit,task_aborted,edit_failed,ecfcmd_failed,killcmd_failed,statuscmd_failed" \
                        ",no_script,killed,status,late,message,by_rule,queue_limit,task_waiting,locked,zombie,no_reque" \
                        ",archived,restored,threshold,sigterm,log_error,checkpt_error,remote_error"

    flag.reset()
    for flag_value in flag_values:
        assert not flag.is_set(flag_value), "expected flag {0} not to be set".format(flag.type_to_string(flag_value))


def can_access_empty_flag():
    suite = ecf.Suite("s1")
    flag = suite.get_flag()  # the flag should be empty for a newly created node.
    for flag_value in flag_values:
        assert not flag.is_set(flag_value), "expected flag %r not to be set" % flag.type_to_string(flag_value)
    assert str(flag) == "", "expected empty string, for an empty flag"



if __name__ == "__main__":

    Test.print_test_start(os.path.basename(__file__))

    can_create_flag_from_default_parameters()
    can_set_clear_reset_flag()
    can_access_empty_flag()

    print("All tests pass")

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import ecflow
import sys
import os
import ecflow_test_util as Test

if __name__ == "__main__":

    Test.print_test_start(os.path.basename(__file__))

    flag = ecflow.Flag()
    assert str(flag) == "", "expected empty string, for an empty flag"

    flag_list = flag.list()  # flag_list is of type FlagTypeVec
    my_flag_list = [ecflow.FlagType.force_abort,
                    ecflow.FlagType.user_edit,
                    ecflow.FlagType.task_aborted,
                    ecflow.FlagType.edit_failed,
                    ecflow.FlagType.jobcmd_failed,
                    ecflow.FlagType.killcmd_failed,
                    ecflow.FlagType.statuscmd_failed,
                    ecflow.FlagType.no_script,
                    ecflow.FlagType.killed,
                    ecflow.FlagType.status,
                    ecflow.FlagType.late,
                    ecflow.FlagType.message,
                    ecflow.FlagType.byrule,
                    ecflow.FlagType.queuelimit,
                    ecflow.FlagType.wait,
                    ecflow.FlagType.locked,
                    ecflow.FlagType.zombie,
                    ecflow.FlagType.no_reque,
                    ecflow.FlagType.archived,
                    ecflow.FlagType.restored,
                    ecflow.FlagType.threshold,
                    ecflow.FlagType.sigterm,
                    ecflow.FlagType.log_error,
                    ecflow.FlagType.checkpt_error,
                    ecflow.FlagType.remote_error
                    ]
    print("Flag list:")
    for flg in flag_list: print("flag ", flag.type_to_string(flg))
    print("My   list:")
    for flg in my_flag_list: print("flag ", flag.type_to_string(flg))

    assert len(flag_list) == len(my_flag_list), "expected flag list have changed"
    expected_flags = "force_aborted,user_edit,task_aborted,edit_failed,ecfcmd_failed,killcmd_failed,statuscmd_failed" \
                     ",no_script,killed,status,late,message,by_rule,queue_limit,task_waiting,locked,zombie,no_reque" \
                     ",archived,restored,threshold,sigterm,log_error,checkpt_error,remote_error";

    # Set *ALL* the flags
    for flg in flag_list:
        flag.set(flg)
        assert flag.is_set(flg), "expected flag %r to be set" % flag.type_to_string(flg)

    # clear *ALL* the flags
    assert str(flag) == expected_flags, "Expected flags \n{0}\nbut found \n{1}".format(str(flag), expected_flags)
    for flg in flag_list:
        flag.clear(flg)
        assert not flag.is_set(flg), "expected flag {0} not to be set".format(flag.type_to_string(flg))

    # test reset
    for flg in flag_list:
        flag.set(flg)
    flag.reset()
    for flg in flag_list:
        assert not flag.is_set(flg), "expected flag %r not to be set" % flag.type_to_string(flg)

    # ===========================================================================
    # Setting flag type
    # ===========================================================================
    for flg in my_flag_list:
        flag.set(flg)
        print("current flag:", flag)
        assert flag.is_set(flg), "expected flag %r to be set" % flag.type_to_string(flg)

    # ===========================================================================
    # Node:  get the flag, should be empty for a newly created node.
    # ===========================================================================
    suite = ecflow.Suite("s1")
    flag = suite.get_flag()
    for flg in flag_list:
        assert not flag.is_set(flg), "expected flag %r not to be set" % flag.type_to_string(flg)
    assert str(flag) == "", "expected empty string, for an empty flag"

    # assert False
    print("All tests pass")

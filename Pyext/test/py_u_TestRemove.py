#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2017 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#  code for testing creation of defs file in python
import os
import ecflow
import copy
from ecflow import Suite, Family, Task, Defs, Clock, DState, PartExpression, Variable, Limit, InLimit, \
                   Date, Day, Event, Meter, Label, Autocancel, Days, TimeSlot, TimeSeries, Style, State, \
                   RepeatString, RepeatDate, RepeatInteger, RepeatDay, RepeatEnumerated, \
                   Verify, PrintStyle, Time, Today, Late, Cron, Client, debug_build

if __name__ == "__main__":
    print("####################################################################")
    print("Running ecflow version " + Client().version() + " debug build(" + str(debug_build()) +")")
    print("####################################################################")
    
    defs = Defs()
    s0 = defs.add_suite("s0")
    s1 = defs.add_suite("s1")
    f1 =  s1.add_family("f1")
    t1 = f1.add_task("t1")
    t2 = f1.add_task("t2")
    assert len(list(f1.nodes)) == 2 ,"Expected family to have 2 nodes"
    defs_copy = copy.copy(defs)
     
    
    t2_remove = t2.remove()
    assert len(f1) == 1 ,"Expected family to have 1 nodes"

    t1_remove = t1.remove()
    assert len(f1) == 0 ,"Expected family to have 0 nodes"

    f1_remove = f1.remove()
    assert len(s1) == 0 ,"Expected suite to have no nodes"

    node_vec = defs.get_all_nodes()
    assert len(node_vec) == 2, "Expected 2 suites only, but found " + str(len(node_vec))

    s1_remove = s1.remove()
    node_vec = defs.get_all_nodes()
    assert len(node_vec) == 1, "Expected 1 suites only, but found " + str(len(node_vec))

    s0_remove = s0.remove()
    node_vec = defs.get_all_nodes()
    assert len(node_vec) == 0, "Expected 0 suites only, but found " + str(len(node_vec))

    # =========================================================
    defs2 = Defs();
    defs2.add_suite(s0_remove)
    s = defs2.add_suite(s1_remove) # s1_remove is returned as s
    f =  s.add_family(f1_remove)   # f1_remove is returned as f
    f.add_task(t1_remove)
    f.add_task(t2_remove)
    assert defs2 == defs_copy ,"expected defs to be the same"

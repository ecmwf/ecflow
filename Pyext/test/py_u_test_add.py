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

# test generic Defs.add(list of object that can be added to a defs)
#      generic Node.add(list of object that can be added to a defs)
#
from ecflow import *
import sys
import os
import copy
import unittest # for assertItemsEqual

def add_generic_attributes(node):
    cron = Cron()
    start = TimeSlot(23 , 0)
    ts = TimeSeries(start,True)  # True means relative to suite start
    cron.set_time_series(ts)

    late = Late()
    late.submitted(TimeSlot(20, 10))
    late.active(TimeSlot(20, 10))
    late.complete(TimeSlot(20, 10), True)
    
    node.add(Variable("name","value"),
             Limit("limit",10),
             InLimit("limit","/s1",2),
             cron,
             late,
             fred = "2", bill = "x")

    node.VAR = "VALUE"
    assert len(list(node.crons)) == 1, "Expected 1 crons"
    assert len(list(node.variables)) == 4, "Expected 4 variable"    

def add_suite_attributes(node):
    assert node.get_clock() == None, "Expected no clock"
    clock = Clock(1, 1, 2010, False)     # day,month, year, hybrid
    clock.set_gain(1, 10, True)                 # True means positive gain
    node.add(clock)
    assert node.get_clock() != None, "Expected clock"

def add_node_attributes(node):
    zombie_life_time_in_server = 800
    child_list = [ChildCmdType.init,ChildCmdType.event,ChildCmdType.meter,ChildCmdType.label,ChildCmdType.wait,ChildCmdType.abort,ChildCmdType.complete]
    node.add(Variable("var","joe90"),
             Edit(fred="fred",bill="bill",jake=1),
             {"A":"a", "C":1 }, # note use of integer
             Trigger("a==1"),
             Complete("a==1"),
             Event(1),Event(1,"event"),
             Meter("meter",0,100),
             Label("label","init-value"),
             Today(1,10),
             Time(1,10),
             Date(1,1,0),
             Day(Days.sunday),
             Day("monday"),
             Limit("limit",10),
             InLimit("limit","/s1",10),
             ZombieAttr(ZombieType.ecf,child_list,ZombieUserActionType.block,zombie_life_time_in_server),
             ZombieAttr(ZombieType.user,child_list,ZombieUserActionType.block,zombie_life_time_in_server),
             ZombieAttr(ZombieType.path,child_list,ZombieUserActionType.block,zombie_life_time_in_server),
             RepeatDate("date", 20100111, 20100115, 2),
             Autocancel(1, 10, True),         # hour,minutes,relative
             keyword="arg1",keyword2="arg12"  # should be added as variables
            )
    node.VAR2 = "VALUE"

    assert len(list(node.events))  == 2, "Expected 2 Events"
    assert len(list(node.meters))  == 1, "Expected 1 Meters"
    assert len(list(node.labels))  == 1, "Expected 1 labels"
    assert len(list(node.todays))  == 1, "Expected 1 todays"
    assert len(list(node.times))   == 1, "Expected 1 times"
    assert len(list(node.dates))   == 1, "Expected 1 dates"
    assert len(list(node.days))    == 2, "Expected 2 days"
    assert len(list(node.zombies)) == 3, "Expected 3 zombie attributes but found " + str(len(list(s1.zombies)))
    assert len(list(node.limits))  == 1, "Expected 1 Limits"
    assert len(list(node.inlimits))== 1, "Expected 1 InLimits"
    assert len(list(node.variables)) == 9, "Expected 9 variables"    
    repeat = node.get_repeat(); assert not repeat.empty(), "Expected repeat"

    task = node.add_task("t1"); task.add(RepeatEnumerated("enum", ["red", "green", "blue" ]))
    repeat = task.get_repeat(); assert not repeat.empty(), "Expected repeat"

    task = node.add_task("t2"); task.add(RepeatString("string", ["a", "b", "c" ]) )
    repeat = task.get_repeat(); assert not repeat.empty(), "Expected repeat"

    task = node.add_task("t3"); task.add(RepeatDay(1))
    repeat = task.get_repeat(); assert not repeat.empty(), "Expected repeat"


if __name__ == "__main__":
    
    print("####################################################################")
    print("Running ecflow version " + Client().version()  + " debug build(" + str(debug_build()) +")")
    print("PYTHONPATH: " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    print("sys.path:   " + str(sys.path))
    print("####################################################################")
     
    defs = Defs().add(Variable("n1","v1"),{"A":"a", "B":"b"},
             Suite("s1").add(
                 Family("f1").add(Variable("n","b"),
                    Task("t1").add(Variable("n","b")))))
  
    suite = defs.add_suite("s2")
    add_suite_attributes(suite)
    add_generic_attributes(suite)
    f1 = suite.add_family("f1")
    add_node_attributes(f1)
    assert suite.f1.t1.name() == "t1"
    assert len(list(defs.suites)) == 2, "Expected 2 suites"    
    assert len(list(defs.user_variables)) == 3, "Expected 3 user variables"    

    suite.VARX = "fred"
    f1.VARX = "joe90"
    print(defs)
    print("All Tests pass")
    
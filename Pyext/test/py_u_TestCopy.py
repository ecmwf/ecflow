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

# code for testing copy
#
import ecflow
import sys
import os
import copy
import ecflow_test_util as Test

if __name__ == "__main__":
    
    Test.print_test_start(os.path.basename(__file__))

    defs = ecflow.Defs()
    defs.add_suite("a").add_family("f1").add_task("t1").add_variable("a","b").add_event(1).add_meter("meter", 0, 100).add_label("label", "v").add_time("+00:30 20:00 01:00")
    print(defs)
    
    defs_copy = copy.copy(defs)
    print(defs_copy)
    assert defs_copy == defs,"defs should be equal after copy"
    
 
    #===========================================================================
    # Defs: add and delete *USER* variables
    #===========================================================================
    defs = ecflow.Defs()
    defs.add_variable("FRED", "/tmp/")
    defs.add_variable("ECF_URL_CMD", "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'")
    defs.add_variable("ECF_URL_BASE", "http://www.ecmwf.int")
    defs.add_variable("ECF_URL", "publications/manuals/sms")
    assert len(list(defs.user_variables)) == 4, "Expected *user* 4 variable"    
    defs_copy = copy.copy(defs)
    assert len(list(defs_copy.user_variables)) == 4, "Expected *user* 4 variable"    

    defs.delete_variable("FRED");     assert len(list(defs.user_variables)) == 3, "Expected 3 variables since we just delete FRED"
    defs.delete_variable("");         assert len(list(defs.user_variables)) == 0, "Expected 0 variables since we should have deleted all"
    assert len(list(defs_copy.user_variables)) == 4, "Expected *user* 4 variable"    

    #===========================================================================
    # Suite: add and delete variables
    #===========================================================================
    suite = ecflow.Suite("s1")
    suite.add_variable(ecflow.Variable("ECF_HOME", "/tmp/"))
    suite.add_variable("ECF_URL_CMD", "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'")
    suite.add_variable("ECF_URL_BASE", "http://www.ecmwf.int")
    suite.add_variable("ECF_URL", "publications/manuals/sms")
    assert len(list(suite.variables)) == 4, "Expected 4 variable"   
    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.variables)) == 4, "Expected 4 variables in copy"   

    #===========================================================================
    # add and delete limits
    #===========================================================================
    suite.add_limit(ecflow.Limit("limitName1", 10))
    suite.add_limit(ecflow.Limit("limitName2", 10))
    suite.add_limit("limitName3", 10)
    suite.add_limit("limitName4", 10)
    assert len(list(suite.limits)) == 4, "Expected 4 Limits"
    
    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.limits)) == 4, "Expected 4 Limits in copy"
    suite_copy.delete_limit("limitName1"); assert len(list(suite_copy.limits)) == 3, "Expected 3 limits since we just deleted one limitName1" 
    suite_copy.delete_limit("");           assert len(list(suite_copy.limits)) == 0, "Expected 0 limits since we just deleted all of them"

    #===========================================================================
    # Test Limit and node paths 
    #===========================================================================
    the_limit = ecflow.Limit("limitName1", 10)
    assert the_limit.name() == "limitName1", "name not as expected"
    assert the_limit.value() == 0 ,"Expected limit value of 0"
    assert the_limit.limit() == 10 ,"Expected limit of 10"
    assert len(list(the_limit.node_paths())) == 0 ,"Expected nodes which have consumed a limit to be empty"
    
    the_limit.increment(1,"/path1") ;  assert the_limit.value() == 1 ,"Expected limit value of 1"
    the_limit.increment(1,"/path2") ;  assert the_limit.value() == 2 ,"Expected limit value of 2"
    the_limit.increment(1,"/path3") ;  assert the_limit.value() == 3 ,"Expected limit value of 3"
    the_limit.increment(1,"/path4") ;  assert the_limit.value() == 4 ,"Expected limit value of 4"
    for path in the_limit.node_paths(): print(path)
    assert len(list(the_limit.node_paths())) == 4 ,"expected 4 path"
    
    limit_copy = copy.copy(the_limit)
    assert len(list(limit_copy.node_paths())) == 4 ,"expected 4 path in copy"

    limit_copy.decrement(1,"/path1") ; assert limit_copy.value() == 3 ,"Expected limit value of 3"
    limit_copy.decrement(1,"/path2") ; assert limit_copy.value() == 2 ,"Expected limit value of 2"
    limit_copy.decrement(1,"/path3") ; assert limit_copy.value() == 1 ,"Expected limit value of 1"
    limit_copy.decrement(1,"/path4") ; assert limit_copy.value() == 0 ,"Expected limit value of 0"
    assert len(list(limit_copy.node_paths())) == 0 ,"Expected nodes which have consumed a limit to be empty"

    limit_copy.increment(1,"/path1") # add same path, should only consume one token
    limit_copy.increment(1,"/path1")
    limit_copy.increment(1,"/path1")
    limit_copy.increment(1,"/path1")
    assert len(list(limit_copy.node_paths())) == 1 ,"expected 1 path"
    assert limit_copy.value() == 1 ,"Expected limit value of 1"

    #===========================================================================
    # add and delete inlimits
    #===========================================================================
    suite.add_inlimit(ecflow.InLimit("limitName1", "/s1/f1", 2))
    suite.add_inlimit(ecflow.InLimit("limitName2", "/s1/f1", 2))
    suite.add_inlimit("limitName3", "/s1/f1", 2)
    suite.add_inlimit("limitName4", "/s1/f1", 2)
    assert len(list(suite.inlimits)) == 4, "Expected 4 inLimits"
    
    suite_copy = copy.copy(suite)
    assert len(list(suite_copy.inlimits)) == 4, "Expected 4 inLimits"
    suite_copy.delete_inlimit("limitName1"); assert len(list(suite_copy.inlimits)) == 3, "Expected 3 inlimits since we just deleted one limitName1" 
    suite_copy.delete_inlimit("");           assert len(list(suite_copy.inlimits)) == 0, "Expected 0 inlimits since we just deleted all of them"

    #===============================================================================
    # add and delete triggers and complete
    #===============================================================================
    task = ecflow.Task("task")
    task.add_trigger("t2 == active")
    task.add_complete("t2 == complete")
    
    task_copy = copy.copy(task)
    assert task_copy.get_complete(), "Expected complete"
    assert task_copy.get_trigger(), "Expected trigger"
    task_copy.delete_trigger();  assert not task_copy.get_trigger(), "Expected no trigger"
    task_copy.delete_complete(); assert not task_copy.get_complete(), "Expected no complete"
    
    #===========================================================================
    # Add triggers using expressions
    #===========================================================================
    expr = ecflow.Expression("t1 == complete")
    task = ecflow.Task("task")
    task.add_trigger(expr)
    
    task_copy = copy.copy(task)
    assert task_copy.get_trigger(), "Expected trigger"

    expr = ecflow.Expression("t1 == complete")
    expr.add(ecflow.PartExpression("t1 == complete", True))
    expr.add(ecflow.PartExpression("t2 == complete", True))
    task = ecflow.Task("task")
    task.add_trigger(expr)

    task_copy = copy.copy(task)
    assert task_copy.get_trigger(), "Expected trigger"

     
    #===========================================================================
    # add,delete,find events
    #===========================================================================
    task.add_event(ecflow.Event(1)); 
    task.add_event(2)                              
    task.add_event(ecflow.Event(10, "Eventname"))  
    task.add_event(10, "Eventname2")               
    task.add_event("fred")                         
    
    # test find
    task_copy = copy.copy(task)
    event = task_copy.find_event("EVENT")
    assert(event.empty()),"Expected to not find event"
    assert(event.name() == ""),"Expected to not to find event, number is maximum int"
    assert(event.value() == 0),"Expected to not to find event"

    event = task_copy.find_event("1"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 1), "Expected to find event 1"
    assert(event.name() == ""), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task_copy.find_event("2"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 2), "Expected to find event 1"
    assert(event.name() == ""), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task_copy.find_event("10"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 10), "Expected to find event 10"
    assert(event.name() == "Eventname"), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task_copy.find_event("fred"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.name() == "fred"), "Expected name to be empty, when name defind an not number, number is max_int"
    assert(event.value() == 0),"Expected to not to find event"

    for e in task_copy.events:     print(str(e)," # value: ",str(e.value()))
    a_dict = {}
    for e in task_copy.events:
        if e.name() != "": a_dict[e.name()] = e.value()
        else:              a_dict[e.number()] = e.value()
    print(a_dict)

    assert len(list(task_copy.events)) == 5, "Expected 5 Events"
    task_copy.delete_event("1");         assert len(list(task_copy.events)) == 4, "Expected 4 Events"
    task_copy.delete_event("Eventname"); assert len(list(task_copy.events)) == 3, "Expected 3 Events"
    task_copy.delete_event("");          assert len(list(task_copy.events)) == 0, "Expected 0 Events"


    #===========================================================================
    # add and delete meter
    #===========================================================================
    task.add_meter(ecflow.Meter("metername1", 0, 100, 50))
    task.add_meter(ecflow.Meter("metername2", 0, 100))
    task.add_meter("metername3", 0, 100, 50)
    task.add_meter("metername4", 0, 100)
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.meters)) == 4, "Expected 4 Meters"
    task_copy.delete_meter("metername1"); assert len(list(task_copy.meters)) == 3, "Expected 3 Meters"
    task_copy.delete_meter("metername4"); assert len(list(task_copy.meters)) == 2, "Expected 2 Meters"
    task_copy.delete_meter("");           assert len(list(task_copy.meters)) == 0, "Expected 0 Meters"


    #===========================================================================
    # add and delete label
    #===========================================================================
    task.add_label(ecflow.Label("label_name1", "value"))
    task.add_label(ecflow.Label("label_name2", "value"))
    task.add_label("label_name3", "value")
    task.add_label("label_name4", "value")
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.labels)) == 4, "Expected 4 labels"
    task_copy.delete_label("label_name1"); assert len(list(task_copy.labels)) == 3, "Expected 3 Labels"
    task_copy.delete_label("label_name4"); assert len(list(task_copy.labels)) == 2, "Expected 2 Labels"
    task_copy.delete_label("");            assert len(list(task_copy.labels)) == 0, "Expected 0 Labels"


    #===========================================================================
    # add delete Repeat
    #===========================================================================
    task = ecflow.Task("task")
    task.add_repeat(ecflow.RepeatInteger("integer", 0, 100, 2))
    task_copy = copy.copy(task)
    repeat = task_copy.get_repeat(); assert not repeat.empty(), "Expected repeat"
    task_copy.delete_repeat()      
    repeat = task_copy.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task = ecflow.Task("task")
    task.add_repeat(ecflow.RepeatEnumerated("enum", ["red", "green", "blue" ]))
    task_copy = copy.copy(task)
    task_copy.delete_repeat()      
    repeat = task_copy.get_repeat(); assert repeat.empty(), "Expected no repeat"

    task = ecflow.Task("task")
    task.add_repeat(ecflow.RepeatDate("date", 20100111, 20100115, 2))
    task_copy = copy.copy(task)
    task_copy.delete_repeat()      
    repeat = task_copy.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task = ecflow.Task("task")
    task.add_repeat(ecflow.RepeatString("string", ["a", "b", "c" ]))
    task_copy = copy.copy(task)
    task_copy.delete_repeat()      
    repeat = task_copy.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    
    #===========================================================================
    # create a time series, used for adding time and today
    #===========================================================================
    start = ecflow.TimeSlot(0, 0)
    finish = ecflow.TimeSlot(23, 0)
    incr = ecflow.TimeSlot(0, 30)
    time_series = ecflow.TimeSeries(start, finish, incr, True)

    # Add and delete today
    # NOTE: **********************************************************************
    # Do *NOT* delete attributes using iterator traversal
    # The iterators *are* bound to the c++ iterators hence if we delete
    # over the traversal, we can corrupt the vector, leading to undefined behavour.  
    # Hence we **can not** delete more than once over a traversal
    # Soln 1: take a copy
    # *****************************************************************************
    # task.add_today( Today( 0,10 ))
    # task.add_today( Today( 0,59, True ))
    # task.add_today( Today(TimeSlot(20,10)) )
    # task.add_today( Today(TimeSlot(20,20),False)) 
    # for today in task.todays: 
    #    task.delete_today(today) # will corrupt C++ vector
  
    task.add_today("00:30")
    task.add_today("+00:30")
    task.add_today("+00:30 20:00 01:00")
    task.add_today(ecflow.Today(time_series))
    task.add_today(ecflow.Today(0, 10))
    task.add_today(0, 59, True)
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 10)))
    task.add_today(ecflow.Today(ecflow.TimeSlot(20, 20), False)) 
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.todays)) == 8, "Expected 8 todays"
    vec_copy = []
    for today in task_copy.todays: vec_copy.append(today)
    for today in vec_copy: task_copy.delete_today(today)
    assert len(list(task_copy.todays)) == 0, "Expected 0 todays"
        
    #===========================================================================
    # add and delete time
    #===========================================================================
    task.add_time("00:30")
    task.add_time("+00:30")
    task.add_time("+00:30 20:00 01:00")
    task.add_time(ecflow.Time(time_series))
    task.add_time(ecflow.Time(0, 10))
    task.add_time(0, 59, True)
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 10)))
    task.add_time(ecflow.Time(ecflow.TimeSlot(20, 20), False)) 

    task_copy = copy.copy(task)
    assert len(list(task_copy.times)) == 8, "Expected 8 times"
    vec_copy = []
    for time in task_copy.times: vec_copy.append(time)
    for time in vec_copy: task_copy.delete_time(time)
    assert len(list(task_copy.times)) == 0, "Expected 0 todays"

    #===========================================================================
    # add and delete date
    #===========================================================================
    for i in [ 1, 2, 4, 8, 16 ] :
        task.add_date(i, 0, 0)
    task.add_date(ecflow.Date(1, 1, 2010))
    task.add_date(1, 1, 2010)       # duplicate
    task.add_date(ecflow.Date(2, 1, 2010))
    task.add_date(ecflow.Date(3, 1, 2010))
    task.add_date(ecflow.Date(4, 1, 2010)) 
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.dates)) == 10, "Expected 10 dates but found " + str(len(list(task_copy.dates)))
    vec_copy = []
    for attr in task_copy.dates: vec_copy.append(attr)
    for attr in vec_copy: task_copy.delete_date(attr)
    assert len(list(task_copy.dates)) == 0, "Expected 0 dates"

    #===========================================================================
    # add and delete day
    #===========================================================================
    task.add_day(ecflow.Day(ecflow.Days.sunday))
    task.add_day(ecflow.Days.monday)
    task.add_day(ecflow.Days.tuesday)  # duplicate ?
    task.add_day("sunday")     
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.days)) == 4, "Expected 4 days"
    vec_copy = []
    for attr in task_copy.days: vec_copy.append(attr)
    for attr in vec_copy: task_copy.delete_day(attr)
    assert len(list(task_copy.days)) == 0, "Expected 0 days"
    
    #===========================================================================
    # add and delete crons
    #===========================================================================
    cron = ecflow.Cron()
    start = ecflow.TimeSlot(23 , 0)
    ts = ecflow.TimeSeries(start,True)  # True means relative to suite start
    cron.set_time_series(ts)
    
    cron = ecflow.Cron()
    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron.set_days_of_month([1, 2, 3, 4, 5, 6 ])
    cron.set_months([1, 2, 3, 4, 5, 6])
    start = ecflow.TimeSlot(0 , 0)
    finish = ecflow.TimeSlot(23, 0)
    incr = ecflow.TimeSlot(0, 30)
    ts = ecflow.TimeSeries(start, finish, incr, True)  # True means relative to suite start
    cron.set_time_series(ts)

    cron0 = ecflow.Cron()
    cron0.set_week_days([0, 1, 2, 3, 4, 5 ])
    cron0.set_time_series(1, 30) # default relative = false, added in release 4.0.7

    cron1 = ecflow.Cron()
    cron1.set_week_days([0, 1, 2, 3, 4, 5, 6 ])
    cron1.set_time_series(1, 30, True)
    
    cron2 = ecflow.Cron()
    cron2.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron2.set_time_series("00:30 01:30 00:01")
    
    cron3 = ecflow.Cron()
    cron3.set_week_days([0, 1, 2, 3, 4, 5, 6])
    cron3.set_time_series("+00:30")

    task = ecflow.Task("task")
    task.add_cron(cron)
    task.add_cron(cron1)
    task.add_cron(cron2)
    task.add_cron(cron3)
    
    task_copy = copy.copy(task)
    assert len(list(task_copy.crons)) == 4, "Expected 4 crons"
    vec_copy = []
    for attr in task_copy.crons: vec_copy.append(attr)
    for attr in vec_copy: task_copy.delete_cron(attr)
    assert len(list(task_copy.crons)) == 0, "Expected 0 crons"  
    
    #===========================================================================
    # add autocancel
    #===========================================================================
    print("test add autoCancel")
    t1 = ecflow.Task("t1")
    assert t1.get_autocancel() == None, " Expected no autocancel"
    t1.add_autocancel(3)  # 3 days
    
    task_copy = copy.copy(t1)
    assert task_copy.get_autocancel() != None, " Expected autocancel"
    print(str(task_copy.get_autocancel()))

    #===========================================================================
    # add late  
    #===========================================================================
    late = ecflow.Late()
    late.submitted(ecflow.TimeSlot(20, 10))
    late.active(ecflow.TimeSlot(20, 10))
    late.complete(ecflow.TimeSlot(20, 10), True)
    task.add_late(late)
    task_copy = copy.copy(task)
    assert task_copy.get_late() != None, " Expected late"
    
    #===========================================================================
    # add defstatus, last one set takes effect
    #===========================================================================
    task.add_defstatus(ecflow.DState.complete)
    task_copy = copy.copy(task)
    assert task_copy.get_defstatus() == ecflow.DState.complete

    #===========================================================================
    # add clock
    #===========================================================================
    clock = ecflow.Clock(1, 1, 2010, False)     # day,month, year, hybrid
    clock.set_gain(1, 10, True)                 # True means positive gain
    suite = ecflow.Suite("suite")
    assert suite.get_clock() == None, "Expected no clock"
    suite.add_clock(clock)
    
    suite_copy = copy.copy(suite)
    assert suite_copy.get_clock() != None, "expected clock"
    
    #===========================================================================
    # Add zombie. Note we can *NOT* add two zombie attributes of the same ZombieType
    #===========================================================================
    s1 = ecflow.Task("s1")
    zombie_life_time_in_server = 800
    child_list = [ ecflow.ChildCmdType.init, ecflow.ChildCmdType.event, ecflow.ChildCmdType.meter, ecflow.ChildCmdType.label, ecflow.ChildCmdType.wait, ecflow.ChildCmdType.abort, ecflow.ChildCmdType.complete ]
    zombie_type_list = [ ecflow.ZombieType.ecf, ecflow.ZombieType.user, ecflow.ZombieType.path ]
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(zombie_type, child_list, ecflow.ZombieUserActionType.block, zombie_life_time_in_server)
        s1.add_zombie(zombie_attr)
    task_copy = copy.copy(s1)
    assert len(list(task_copy.zombies)) == 3, "Expected 3 zombie attributes but found " + str(len(list(task_copy.zombies)))
    
    # delete all the zombies
    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0, "Expected zero zombie attributes but found " + str(len(list(s1.zombies)))
    
    # add with zombie_life_time_in_server not set, this is optional
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(zombie_type, child_list, ecflow.ZombieUserActionType.block)
        s1.add_zombie(zombie_attr)
    task_copy = copy.copy(s1)
    assert len(list(task_copy.zombies)) == 3, "Expected 3 zombie attributes but found " + str(len(list(task_copy.zombies)))

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0, "Expected zero zombie attributes but found " + str(len(list(s1.zombies)))

    print("All Tests pass")
    
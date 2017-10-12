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

# code for testing addition and deletion 
#
import ecflow
import sys
import os
import copy
import unittest # for assertItemsEqual
 
if __name__ == "__main__":
    
    print("####################################################################")
    print("Running ecflow version " + ecflow.Client().version()  + " debug build(" + str(ecflow.debug_build()) +")")
    print("PYTHONPATH: " + str(os.environ['PYTHONPATH'].split(os.pathsep)))
    print("sys.path:   " + str(sys.path))
    print("####################################################################")
 
    #===========================================================================
    # Defs: add, delete and sort *USER* variables, use set(a).intersection(b) to compare lists
    #===========================================================================
    defs = ecflow.Defs()
    defs.add_variable("ZFRED", "/tmp/")
    defs.add_variable("YECF_URL_CMD", "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'")
    defs.add_variable("XECF_URL_BASE", "http://www.ecmwf.int")
    defs.add_variable("AECF_URL", "publications/manuals/sms")
    assert len(list(defs.user_variables)) == 4, "Expected *user* 4 variable"    
    
    # sort
    expected = ['AECF_URL','XECF_URL_BASE','YECF_URL_CMD','ZFRED']
    actual = []
    defs.sort_attributes("variable");
    defs.sort_attributes(ecflow.AttrType.variable);
    for v in defs.user_variables: actual.append(v.name())
    assert actual == expected,"Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    
    
    defs.delete_variable("ZFRED");assert len(list(defs.user_variables)) == 3, "Expected 3 variables since we just delete FRED"
    defs.delete_variable("");     assert len(list(defs.user_variables)) == 0, "Expected 0 variables since we should have deleted all"
    
    a_dict = { "name":"value", "name2":"value2", "name3":"value3", "name4":"value4" }
    defs.add_variable(a_dict)
    assert len(list(defs.user_variables)) == 4, "Expected 4 variable"    
    defs.delete_variable("");         assert len(list(defs.user_variables)) == 0, "Expected 0 variable since we should have deleted all"

    # add a empty dictionary
    a_dict = {}
    defs.add_variable(a_dict)
    assert len(list(defs.user_variables)) == 0, "Expected zero variables"    
    
    #===========================================================================
    # Suite: add,delete and sort variables
    #===========================================================================
    suite = ecflow.Suite("s1")
    suite.add_variable(ecflow.Variable("ECF_HOME", "/tmp/"))
    suite.add_variable("ZZZZZZ", "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'")
    suite.add_variable("YYYYY", "http://www.ecmwf.int")
    suite.add_variable("aaaa", "publications/manuals/sms")
    assert len(list(suite.variables)) == 4, "Expected 4 variable"    
    
    # sort
    expected = ['aaaa','ECF_HOME','YYYYY','ZZZZZZ']
    actual = []
    suite.sort_attributes("variable");
    suite.sort_attributes(ecflow.AttrType.variable);
    for v in suite.variables: actual.append(v.name())
    assert set(expected).intersection(actual), "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)

    suite.delete_variable("ECF_HOME"); assert len(list(suite.variables)) == 3, "Expected 3 variable since we just delete ECF_HOME"
    suite.delete_variable("");         assert len(list(suite.variables)) == 0, "Expected 0 variable since we should have deleted all"
    
    a_dict = { "name":"value", "name2":"value2", "name3":"value3", "name4":"value4" }
    suite.add_variable(a_dict)
    assert len(list(suite.variables)) == 4, "Expected 4 variable"    
    suite.delete_variable("");         assert len(list(suite.variables)) == 0, "Expected 0 variable since we should have deleted all"

    # add a empty dictionary
    a_dict = { }
    suite.add_variable(a_dict)
    assert len(list(suite.variables)) == 0, "Expected zero variables"    

    # adding dictionary items that are not strings should result in a type error
    expected_type_error = False
    try:
        a_bad_dict = { "name":"fred", "name2":14, "name3":"12", "name4":12 }
        suite.add_variable(a_bad_dict)
    except TypeError:
        expected_type_error = True
        
    assert expected_type_error, "Expected Type error"
    assert len(list(suite.variables)) == 0, "Expected 0 variable since we should have deleted all"
    
    suite.add_variable("ECF_URL_CMD", "test duplicates") 
    suite.add_variable("ECF_URL_CMD", "Expected warning")  # expect a warning message to standard out


    #===========================================================================
    # add,delete and sort limits
    #===========================================================================
    suite.add_limit(ecflow.Limit("zlimitName1", 10))
    suite.add_limit(ecflow.Limit("ylimitName2", 10))
    suite.add_limit("xlimitName3", 10)
    suite.add_limit("alimitName4", 10)
    assert len(list(suite.limits)) == 4, "Expected 4 Limits"
    
    # sort
    expected = ['alimitName4','xlimitName3','ylimitName2','zlimitName1']
    actual = []
    suite.sort_attributes(ecflow.AttrType.limit);
    suite.sort_attributes("limit");
    for v in suite.limits: actual.append(v.name())
    assert expected == actual, "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)

    suite.delete_limit("zlimitName1"); assert len(list(suite.limits)) == 3, "Expected 3 limits since we just deleted one limitName1" 
    suite.delete_limit("");            assert len(list(suite.limits)) == 0, "Expected 0 limits since we just deleted all of them"

    # The following will fail, since the iterators are essentially read only, 
    # This is because we are using C++ vector iterators, hence we can't delete the vectors items, whilst traversing 
    #    for limit in suite.limits: suite.delete_limit(limit.name())
    # We can get round this by copy the limits first
    suite.add_limit(ecflow.Limit("limitName1", 10))
    suite.add_limit(ecflow.Limit("limitName2", 10))
    suite.add_limit("limitName3", 10)
    suite.add_limit("limitName4", 10)
    limit_names = []
    for limit in suite.limits:
        limit_names.append(limit.name())
    for limit in limit_names:
        suite.delete_limit(limit)
    assert len(list(suite.limits)) == 0, "Expected 0 Limits,since we just deleted all of them, via iteration"

    #===========================================================================
    # Test Limit and node paths, ECFLOW-518
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
    the_limit.decrement(1,"/path1") ; assert the_limit.value() == 3 ,"Expected limit value of 3"
    the_limit.decrement(1,"/path2") ; assert the_limit.value() == 2 ,"Expected limit value of 2"
    the_limit.decrement(1,"/path3") ; assert the_limit.value() == 1 ,"Expected limit value of 1"
    the_limit.decrement(1,"/path4") ; assert the_limit.value() == 0 ,"Expected limit value of 0"
    assert len(list(the_limit.node_paths())) == 0 ,"Expected nodes which have consumed a limit to be empty"

    the_limit.increment(1,"/path1") # add same path, should only consume one token
    the_limit.increment(1,"/path1")
    the_limit.increment(1,"/path1")
    the_limit.increment(1,"/path1")
    assert len(list(the_limit.node_paths())) == 1 ,"expected 1 path"
    assert the_limit.value() == 1 ,"Expected limit value of 1"

    #===========================================================================
    # add and delete inlimits
    #===========================================================================
    suite.add_inlimit(ecflow.InLimit("limitName1", "/s1/f1", 2))
    suite.add_inlimit(ecflow.InLimit("limitName2", "/s1/f1", 2, True))
    suite.add_inlimit("limitName3", "/s1/f1", 2)
    suite.add_inlimit("limitName4", "/s1/f1", 2)
    suite.add_inlimit("limitNameA", "/s1/f1", 2)
    suite.add_inlimit("limitNameA", "/s1/f1/a", 2)
    suite.add_inlimit("limitNameA", "/s1/f1/b", 2)
    assert len(list(suite.inlimits)) == 7, "Expected 7 inLimits"
    suite.delete_inlimit("limitName1"); assert len(list(suite.inlimits)) == 6, "Expected 6 inlimits since we just deleted one limitName1" 
    suite.delete_inlimit("/s1/f1:limitNameA"); assert len(list(suite.inlimits)) == 5, "Expected 5 inlimits since we just deleted one limitName1" 
    suite.delete_inlimit("/s1/f1/a:limitNameA"); assert len(list(suite.inlimits)) == 4, "Expected 4 inlimits since we just deleted one limitName1" 
    suite.delete_inlimit("/s1/f1/b:limitNameA"); assert len(list(suite.inlimits)) == 3, "Expected 3 inlimits since we just deleted one limitName1" 
    suite.delete_inlimit("");           assert len(list(suite.inlimits)) == 0, "Expected 0 inlimits since we just deleted all of them"

    #===============================================================================
    # add and delete triggers and complete
    #===============================================================================
    task = ecflow.Task("task")
    task.add_trigger("t2 == active")
    task.add_complete("t2 == complete")
    assert task.get_complete(), "Expected complete"
    assert task.get_trigger(), "Expected trigger"
    task.delete_trigger();  assert not task.get_trigger(), "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"
    
    task.add_part_trigger(ecflow.PartExpression("t1 == complete"))
    task.add_part_trigger(ecflow.PartExpression("t2 == active", True))  #  for long and/or expressions, subsequent expr must be and/or
    task.add_part_complete(ecflow.PartExpression("t3 == complete"))
    task.add_part_complete(ecflow.PartExpression("t4 == active", False))  #  for long and/or expressions, subsequent expr must be and/or
    task.delete_trigger();  assert not task.get_trigger(), "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"

    task.add_part_trigger("t1 == complete")
    task.add_part_trigger("t2 == active", True)  #  for long and/or expressions, subsequent expr must be and/or
    task.add_part_complete("t3 == complete")
    task.add_part_complete("t4 == active", False)  #  for long and/or expressions, subsequent expr must be and/or
    task.delete_trigger();  assert not task.get_trigger(), "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"
   
    #===========================================================================
    # Add triggers using expressions
    #===========================================================================
    expr = ecflow.Expression("t1 == complete")
    task = ecflow.Task("task")
    task.add_trigger(expr)

    expr = ecflow.Expression("t1 == complete")
    expr.add(ecflow.PartExpression("t1 == complete", True))
    expr.add(ecflow.PartExpression("t2 == complete", True))
    task = ecflow.Task("task")
    task.add_trigger(expr)

     
    #===========================================================================
    # add,delete,find events
    #===========================================================================
    task.add_event(ecflow.Event(1)); 
    task.add_event(2)                              
    task.add_event(ecflow.Event(10, "Eventname"))  
    task.add_event(10, "Eventname2")               
    task.add_event("fred")                         
    
    # sort
    expected = ['1','2','Eventname','Eventname2','fred']
    actual = []
    task.sort_attributes("event");
    task.sort_attributes(ecflow.AttrType.event);
    for v in task.events: actual.append(v.name_or_number())
    assert expected == actual, "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
     
    # test find
    event = task.find_event("EVENT")
    assert(event.empty()),"Expected to not to find event"
    assert(event.name() == ""),"Expected to not to find event, number is maximum int"
    assert(event.value() == 0),"Expected to not to find event"

    event = task.find_event("1"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 1), "Expected to find event 1"
    assert(event.name() == ""), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task.find_event("2"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 2), "Expected to find event 1"
    assert(event.name() == ""), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task.find_event("10"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.number() == 10), "Expected to find event 10"
    assert(event.name() == "Eventname"), "Expected name to be empty"
    assert(event.value() == 0),"Expected to not to find event"

    event = task.find_event("fred"); 
    assert(not event.empty()),"Expected to find event"
    assert(event.name() == "fred"), "Expected name to be empty, when name defind an not number, number is max_int"
    assert(event.value() == 0),"Expected to not to find event"

    for e in task.events:     print(str(e)," # value: ",str(e.value()))
    a_dict = {}
    for e in task.events:
        if e.name() != "": a_dict[e.name()] = e.value()
        else:              a_dict[e.number()] = e.value()
    print(a_dict)

    assert len(list(task.events)) == 5, "Expected 5 Events"
    task.delete_event("1");         assert len(list(task.events)) == 4, "Expected 4 Events"
    task.delete_event("Eventname"); assert len(list(task.events)) == 3, "Expected 3 Events"
    task.delete_event("");          assert len(list(task.events)) == 0, "Expected 0 Events"


    #===========================================================================
    # add and delete meter
    #===========================================================================
    task.add_meter(ecflow.Meter("zzzz", 0, 100, 50))
    task.add_meter(ecflow.Meter("yyyy", 0, 100))
    task.add_meter("bbbb", 0, 100, 50)
    task.add_meter("aaaa", 0, 100)
    assert len(list(task.meters)) == 4, "Expected 4 Meters"
    
    # sort
    expected = ['aaaa','bbbb','yyyy','zzzz']
    actual = []
    task.sort_attributes(ecflow.AttrType.meter);
    task.sort_attributes("meter");
    for v in task.meters: actual.append(v.name())
    assert expected == actual, "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
 
    task.delete_meter("zzzz"); assert len(list(task.meters)) == 3, "Expected 3 Meters"
    task.delete_meter("yyyy"); assert len(list(task.meters)) == 2, "Expected 2 Meters"
    task.delete_meter("");     assert len(list(task.meters)) == 0, "Expected 0 Meters"

    #===========================================================================
    # add and delete queue
    #===========================================================================
    queue_items = [ "001", "002"]
    task.add_queue(ecflow.Queue("queue",queue_items))
    task.add_queue("queue1",queue_items)
    task.add_queue("queue2",queue_items)
    task.add_queue("queue3",queue_items)
    assert len(list(task.queues)) == 4, "Expected 4 Queues"
    task.delete_queue("queue3"); assert len(list(task.queues)) == 3, "Expected 3 Queues"
    task.delete_queue("queue2"); assert len(list(task.queues)) == 2, "Expected 2 Queues"
    task.delete_queue("");       assert len(list(task.queues)) == 0, "Expected 0 Queues"

    #===========================================================================
    # add and delete, find generic. These are arbitary attributes for future func.
    #===========================================================================
    generic_items = [ "001", "002"]
    task.add_generic(ecflow.Generic("gen",generic_items ))
    task.add_generic("gen1",generic_items )
    task.add_generic("gen2",generic_items )
    task.add_generic("gen3",generic_items )
    assert len(list(task.generics)) == 4, "Expected 4 Generics"
    gen = task.find_generic("gen3")
    assert not gen.empty()," find failed"
    assert gen.name() == "gen3", "expected for find gen3"
    
    task.delete_generic("gen1"); assert len(list(task.generics)) == 3, "Expected 3 Generics"
    task.delete_generic("gen2"); assert len(list(task.generics)) == 2, "Expected 2 Generics"
    task.delete_generic("");     assert len(list(task.generics)) == 0, "Expected 0 Generics"

    #===========================================================================
    # add and delete label
    #===========================================================================
    task.add_label(ecflow.Label("labela", "value"))
    task.add_label(ecflow.Label("labelb", "value"))
    task.add_label("labelc", "value")
    task.add_label("labeld", "value")
    assert len(list(task.labels)) == 4, "Expected 4 labels"
    
    # sort
    expected = ['labela','labelb','labelc','labeld']
    actual = []
    task.sort_attributes("label");
    task.sort_attributes(ecflow.AttrType.label);
    for v in task.labels: actual.append(v.name())
    assert expected == actual, "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    
    task.delete_label("labela"); assert len(list(task.labels)) == 3, "Expected 3 Labels"
    task.delete_label("labelb"); assert len(list(task.labels)) == 2, "Expected 2 Labels"
    task.delete_label("");       assert len(list(task.labels)) == 0, "Expected 0 Labels"

    #===========================================================================
    # add delete Repeat
    #===========================================================================
    task.add_repeat(ecflow.RepeatInteger("integer", 0, 100, 2))
    repeat = task.get_repeat(); assert not repeat.empty(), "Expected repeat"
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task.add_repeat(ecflow.RepeatEnumerated("enum", ["red", "green", "blue" ]))
    print(task)
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"

    task.add_repeat(ecflow.RepeatDate("date", 20100111, 20100115, 2))
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task.add_repeat(ecflow.RepeatString("string", ["a", "b", "c" ]))
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    
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
    assert len(list(task.todays)) == 8, "Expected 8 todays"
    vec_copy = []
    for today in task.todays: vec_copy.append(today)
    for today in vec_copy: task.delete_today(today)
    assert len(list(task.todays)) == 0, "Expected 0 todays"
        
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
    assert len(list(task.times)) == 8, "Expected 8 times"
    vec_copy = []
    for time in task.times: vec_copy.append(time)
    for time in vec_copy: task.delete_time(time)
    assert len(list(task.todays)) == 0, "Expected 0 todays"

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
    assert len(list(task.dates)) == 10, "Expected 10 dates but found " + str(len(list(task.dates)))
    vec_copy = []
    for attr in task.dates: vec_copy.append(attr)
    for attr in vec_copy: task.delete_date(attr)
    assert len(list(task.dates)) == 0, "Expected 0 dates"

    #===========================================================================
    # add and delete day
    #===========================================================================
    task.add_day(ecflow.Day(ecflow.Days.sunday))
    task.add_day(ecflow.Days.monday)
    task.add_day(ecflow.Days.tuesday)  # duplicate ?
    task.add_day("sunday")     
    assert len(list(task.days)) == 4, "Expected 4 days"
    vec_copy = []
    for attr in task.days: vec_copy.append(attr)
    for attr in vec_copy: task.delete_day(attr)
    assert len(list(task.days)) == 0, "Expected 0 days"
    
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

    task.add_cron(cron)
    task.add_cron(cron1)
    task.add_cron(cron2)
    task.add_cron(cron3)
    assert len(list(task.crons)) == 4, "Expected 4 crons"
    vec_copy = []
    for attr in task.crons: vec_copy.append(attr)
    for attr in vec_copy: task.delete_cron(attr)
    assert len(list(task.crons)) == 0, "Expected 0 crons"  
    
    #===========================================================================
    # add autocancel
    #===========================================================================
    print("test add autoCancel")
    t1 = ecflow.Task("t1")
    assert t1.get_autocancel() == None, " Expected no autocancel"
    t1.add_autocancel(3)                       # 3 days
    assert t1.get_autocancel() != None, " Expected autocancel"
    print(str(t1.get_autocancel()))

    t3 = ecflow.Task("t3")
    t3.add_autocancel(20, 10, True)              # hour,minutes,relative
    print(str(t3.get_autocancel()))
    
    t4 = ecflow.Task("t4")
    t4.add_autocancel(ecflow.TimeSlot(10, 10), True)    # hour,minutes,relative
    print(str(t4.get_autocancel()))

    t5 = ecflow.Task("t5")
    t5.add_autocancel(ecflow.Autocancel(1, 10, True))   # hour,minutes,relative
    print(str(t5.get_autocancel()))
    
    #===========================================================================
    # add autoarchive
    #===========================================================================
    print("test add autoarchive")
    f1 = ecflow.Family("f1")
    assert f1.get_autoarchive() == None, " Expected no autoarchive"
    f1.add_autoarchive(3)                       # 3 days
    assert f1.get_autoarchive() != None, " Expected autoarchive"
    print(str(f1.get_autoarchive()))

    f3 = ecflow.Family("f3")
    f3.add_autoarchive(20, 10, True)              # hour,minutes,relative
    print(str(f3.get_autoarchive()))
    
    f4 = ecflow.Family("f4")
    f4.add_autoarchive(ecflow.TimeSlot(10, 10), True)    # hour,minutes,relative
    print(str(t4.get_autoarchive()))

    f5 = ecflow.Family("f5")
    f5.add_autocancel(ecflow.Autocancel(1, 10, True))   # hour,minutes,relative
    print(str(f5.get_autoarchive()))
    
    #===========================================================================
    # add autorestore
    #===========================================================================
    print("test add autorestore")
    t1 = ecflow.Task("f1")
    assert t1.get_autorestore() == None, " Expected no autorestore"
    t1.add_autorestore(["/s1/f2"])                       
    assert t1.get_autorestore() != None, " Expected autorestore"
    print(str(t1.get_autorestore()))

    f5 = ecflow.Family("f5")
    assert f5.get_autorestore() == None, " Expected no autorestore"
    f5.add_autorestore(["/s1/f2","/s2","/s3"])                       
    assert f5.get_autorestore() != None, " Expected autorestore"
    print(str(f5.get_autoarchive()))
    
    f6 = ecflow.Family("f6")
    assert f6.get_autorestore() == None, " Expected no autorestore"
    f6.add_autorestore(ecflow.Autorestore(["/s1/f2","/s2","/s3"]))                       
    assert f6.get_autorestore() != None, " Expected autorestore"
    print(str(f6.get_autoarchive()))
    
    #===========================================================================
    # add late
    #===========================================================================
    late = ecflow.Late()
    late.submitted(ecflow.TimeSlot(20, 10))
    late.active(ecflow.TimeSlot(20, 10))
    late.complete(ecflow.TimeSlot(20, 10), True)
    task.add_late(late)
    
    late = ecflow.Late()
    late.submitted(20, 10)
    late.active(20, 10)
    late.complete(20, 10, True)
    t1.add_late(late)
    
    
    #===========================================================================
    # add defstatus, last one set takes effect
    #===========================================================================
    task.add_defstatus(ecflow.DState.complete)
    assert task.get_defstatus() == ecflow.DState.complete
    task.add_defstatus(ecflow.DState.queued)
    assert task.get_defstatus() == ecflow.DState.queued
    task.add_defstatus(ecflow.DState.aborted)
    assert task.get_defstatus() == ecflow.DState.aborted
    task.add_defstatus(ecflow.DState.submitted)
    assert task.get_defstatus() == ecflow.DState.submitted
    task.add_defstatus(ecflow.DState.suspended)
    assert task.get_defstatus() == ecflow.DState.suspended
    task.add_defstatus(ecflow.DState.active)
    assert task.get_defstatus() == ecflow.DState.active

    # the state should be unknown until the suite is begun
    assert task.get_state() == ecflow.State.unknown
    assert task.get_dstate() == ecflow.DState.unknown
 
    #===========================================================================
    # add clock
    #===========================================================================
    clock = ecflow.Clock(1, 1, 2010, False)     # day,month, year, hybrid
    clock.set_gain(1, 10, True)                 # True means positive gain
    suite = ecflow.Suite("suite")
    suite.add_clock(clock)
    
    clock = ecflow.Clock(1, 1, 2011, True)       # day,month, year, hybrid
    clock.set_gain_in_seconds(12, True)
    s1 = ecflow.Suite("s1")
    s1.add_clock(clock)
    
    print("#===========================================================================")
    print("# get clock")
    print("#===========================================================================")
    s0 = ecflow.Suite("s0")
    assert s0.get_clock() == None, "Expected no clock"
    
    s0.add_clock(ecflow.Clock(1, 1, 2010, False))
    assert s0.get_clock() != None, "Expected clock"

    #===========================================================================
    # end clock, used in simulator only, not persisted
    #===========================================================================
    clock = ecflow.Clock(1, 1, 2010, False)     # day,month, year, hybrid
    clock.set_gain(1, 10, True)                 # True means positive gain
    suite = ecflow.Suite("suite")
    suite.add_end_clock(clock)
    
    clock = ecflow.Clock(1, 1, 2011, True)       # day,month, year, hybrid
    clock.set_gain_in_seconds(12, True)
    s1 = ecflow.Suite("s1")
    s1.add_end_clock(clock)
    
    print("#===========================================================================")
    print("# get end clock")
    print("#===========================================================================")
    s0 = ecflow.Suite("s0")
    assert s0.get_end_clock() == None, "Expected no end clock"
    
    s0.add_end_clock(ecflow.Clock(1, 1, 2010, False))
    assert s0.get_end_clock() != None, "Expected end clock"
    #===========================================================================
    # Add zombie. Note we can *NOT* add two zombie attributes of the same ZombieType
    #===========================================================================
    zombie_life_time_in_server = 800
    child_list = [ ecflow.ChildCmdType.init, ecflow.ChildCmdType.event, ecflow.ChildCmdType.meter, ecflow.ChildCmdType.label, ecflow.ChildCmdType.wait, ecflow.ChildCmdType.abort, ecflow.ChildCmdType.complete ]
    zombie_type_list = [ ecflow.ZombieType.ecf, ecflow.ZombieType.ecf_pid, ecflow.ZombieType.ecf_pid_passwd, ecflow.ZombieType.ecf_passwd, ecflow.ZombieType.user, ecflow.ZombieType.path ]
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(zombie_type, child_list, ecflow.ZombieUserActionType.block, zombie_life_time_in_server)
        s1.add_zombie(zombie_attr)
    assert len(list(s1.zombies)) == 6, "Expected 6 zombie attributes but found " + str(len(list(s1.zombies)))
    
    # delete all the zombies
    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0, "Expected zero zombie attributes but found " + str(len(list(s1.zombies)))
    
    # add with zombie_life_time_in_server not set, this is optional
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(zombie_type, child_list, ecflow.ZombieUserActionType.block)
        s1.add_zombie(zombie_attr)
    assert len(list(s1.zombies)) == 6, "Expected 6 zombie attributes but found " + str(len(list(s1.zombies)))

    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0, "Expected zero zombie attributes but found " + str(len(list(s1.zombies)))

    # repeat the the test with empty child list. Empty child list means apply to all child commands
    child_list = []
    for zombie_type in zombie_type_list:
        zombie_attr = ecflow.ZombieAttr(zombie_type, child_list, ecflow.ZombieUserActionType.block, zombie_life_time_in_server)
        s1.add_zombie(zombie_attr)
        
        # test delete of specific zombie attribute
        assert len(list(s1.zombies)) == 1, "Expected 1 zombie attributes but found " + str(len(list(s1.zombies)))
        s1.delete_zombie(zombie_type)
        assert len(list(s1.zombies)) == 0, "Expected 0 zombie attributes but found " + str(len(list(s1.zombies)))


    print("All Tests pass")
    
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

import os
from ecflow import Defs, Suite, Variable, Limit, InLimit, Task, PartExpression, \
                   Event, Meter, Label, RepeatInteger, RepeatEnumerated, RepeatDate, RepeatString, \
                   TimeSlot, TimeSeries, Today, Time, Date, Day, Days, Cron, Autocancel, Late, \
                   DState, Clock, ChildCmdType, ZombieType, ZombieAttr, ZombieUserActionType, Client, debug_build
import ecflow_test_util as Test

if __name__ == "__main__":

    Test.print_test_start(os.path.basename(__file__))
    
    #
    # Add Nodes functional way
    #
    defs = Defs()
    defs.add_suite("s1").add_task("t1").add_variable("var","v")
    defs.add_suite("s2").add_family("f1").add_task("t1").add_variable("var","v")
    defs.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable("var","v")
    assert len(defs) == 3,"Expected 3 suites"    


    # add and delete variables
    a_dict = { "name":"value", "name2":"value2", "name3":"value3", "name4":"value4" }
    suite = Suite("s1")
    suite.add_variable(Variable("ECF_HOME","'/tmp/'")) \
         .add_variable("Fred", 1)\
         .add_variable("ECF_URL_BASE",'http://www.ecmwf.int')\
         .add_variable("ECF_URL",'"publications/manuals/sms"')\
         .add_variable(a_dict)
    print(suite)
    assert len(list(suite.variables)) == 8,"Expected 8 variable"    
    suite.delete_variable("");         assert len(list(suite.variables)) == 0,"Expected 0 variable since we should have deleted all"
    
    
    # add and delete limits
    the_limit = Limit("limitName1", 10)
    assert len(list(the_limit.node_paths())) == 0 ,"Expected nodes which have consumed a limit to be empty"
    suite.add_limit(  Limit("limitName1", 10) )\
        .add_limit(  Limit("limitName2", 10) )\
        .add_limit(  "limitName3", 10 )\
        .add_limit(  "limitName4", 10 )
    assert len(list(suite.limits)) == 4,"Expected 4 Limits"
    suite.delete_limit("");           assert len(list(suite.limits)) == 0,"Expected 0 limits since we just deleted all of them"


    # add and delete inlimits
    suite.add_inlimit( InLimit("limitName1","/s1/f1",2) )\
        .add_inlimit( InLimit("limitName2","/s1/f1",2))\
        .add_inlimit( "limitName3","/s1/f1",2)\
        .add_inlimit( "limitName4","/s1/f1",2)
    assert len(list(suite.inlimits)) == 4,"Expected 4 inLimits"
    suite.delete_inlimit("limitName1"); assert len(list(suite.inlimits)) == 3,"Expected 3 inlimits since we just deleted one limitName1" 
    suite.delete_inlimit("");           assert len(list(suite.inlimits)) == 0,"Expected 0 inlimits since we just deleted all of them"


    # add and delete triggers and complete
    task = Task("task")
    task.add_trigger( "t2 == active" ).add_complete( "t2 == complete" )
    assert task.get_complete(), "Expected complete"
    assert task.get_trigger(),  "Expected trigger"
    task.delete_trigger();  assert not task.get_trigger(),  "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"
    
    task.add_part_trigger(  PartExpression("t1 == complete") )\
        .add_part_complete( PartExpression("t3 == complete") ) \
        .add_part_complete( PartExpression("t4 == active",False) )   
    task.delete_trigger();  assert not task.get_trigger(),  "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"

    task.add_part_trigger(  "t1 == complete" ) \
        .add_part_complete( "t3 == complete" ) \
        .add_part_complete( "t4 == active",False)  #  for long and/or expressions, subsequent expr must be and/or
    task.delete_trigger();  assert not task.get_trigger(),  "Expected no trigger"
    task.delete_complete(); assert not task.get_complete(), "Expected no complete"
    
    
    # add and delete events
    task.add_event( Event(1) ).add_event( 2 ).add_event( Event(10, "Eventname") ).add_event( 10, "Eventname2" ).add_event( "fred" )
    assert len(list(task.events)) == 5,"Expected 5 Events"
    task.delete_event("");          assert len(list(task.events)) == 0,"Expected 0 Events"


    # add and delete meter
    task.add_meter( Meter("metername1", 0, 100, 50) )\
        .add_meter( Meter("metername2", 0, 100) )\
        .add_meter( "metername3", 0, 100, 50 )\
        .add_meter( "metername4", 0, 100 )
    assert len(list(task.meters)) == 4,"Expected 4 Meters"
    task.delete_meter("");           assert len(list(task.meters)) == 0,"Expected 0 Meters"


    # add and delete label
    task.add_label( Label("label_name1", "value") )\
        .add_label( Label("label_name2", "value") )\
        .add_label( "label_name3", "value" )\
        .add_label( "label_name4", "value" )
    assert len(list(task.labels)) == 4,"Expected 4 labels"
    task.delete_label("");            assert len(list(task.labels)) == 0,"Expected 0 Labels"


    # add delete Repeat
    task.add_repeat( RepeatInteger("integer", 0, 100, 2) ).add_variable("fred","j")
    repeat = task.get_repeat(); assert not repeat.empty(), "Expected repeat"
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task.add_repeat( RepeatEnumerated("enum", ["red", "green", "blue" ]) ).add_variable("Q","j")
    print(task)
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"

    task.add_repeat( RepeatDate("date", 20100111, 20100115,2) ).add_variable("W","j")
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    task.add_repeat( RepeatString("string", ["a", "b", "c" ] ) ).add_variable("E","j")
    task.delete_repeat()      
    repeat = task.get_repeat(); assert repeat.empty(), "Expected no repeat"
    
    
    # create a time series, used for adding time and today
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    time_series = TimeSeries( start, finish, incr, True)

    # Add and delete today
    # NOTE: **********************************************************************
    # Do *NOT* delete attributes using iterator traversal
    # The iterators *are* bound to the c++ iterators hence if we delete
    # over the traversal, we can corrupt the vector, leading to undefined behaviour.  
    # Hence we **can not** delete more than once over a traversal
    # Soln 1: take a copy
    # *****************************************************************************
    # task.add_today( Today( 0,10 ))
    # task.add_today( Today( 0,59, True ))
    # task.add_today( Today(TimeSlot(20,10)) )
    # task.add_today( Today(TimeSlot(20,20),False)) 
    # for today in task.todays: 
    #    task.delete_today(today) # will corrupt C++ vector
  
    task.add_today( "00:30" ).add_today( "+00:30" ).add_today( "+00:30 20:00 01:00" )\
        .add_today( Today( time_series) )\
        .add_today( Today( 0,10 )).add_today( 0, 59, True )\
        .add_today( Today(TimeSlot(20,10)) )\
        .add_today( Today(TimeSlot(20,20),False)) 
    assert len(list(task.todays)) == 8,"Expected 8 todays"
    vec_copy = []
    for today in task.todays: vec_copy.append(today)
    for today in vec_copy: task.delete_today(today)
    assert len(list(task.todays)) == 0,"Expected 0 todays"
        
             
    # add and delete time
    task.add_time( "00:30" ).add_time( "+00:30" ).add_time( "+00:30 20:00 01:00" )\
        .add_time( Time(time_series ))\
        .add_time( Time( 0,10 ))\
        .add_time( 0, 59, True)\
        .add_time( Time(TimeSlot(20,10)) )\
        .add_time( Time(TimeSlot(20,20),False)) 
    assert len(list(task.times)) == 8,"Expected 8 times"
    vec_copy = []
    for time in task.times: vec_copy.append(time)
    for time in vec_copy: task.delete_time(time)
    assert len(list(task.todays)) == 0,"Expected 0 todays"


    # add and delete date
    for i in [ 1, 2, 4, 8, 16 ] :
        task.add_date( i, 0, 0)
    task.add_date( Date(1, 1, 2010))\
        .add_date( Date(2, 1, 2010))\
        .add_date( Date(3, 1, 2010))\
        .add_date( Date(4, 1, 2010))\
        .add_date( 1, 1, 2010)
    assert len(list(task.dates)) == 10,"Expected 10 dates but found " + str(len(list(task.dates)))
    vec_copy = []
    for attr in task.dates: vec_copy.append(attr)
    for attr in vec_copy: task.delete_date(attr)
    assert len(list(task.dates)) == 0,"Expected 0 dates"

    # add and delete day
    task.add_day( Day(Days.sunday))\
        .add_day( Days.monday)\
        .add_day( Days.tuesday)\
        .add_day( "sunday")     
    assert len(list(task.days)) == 4,"Expected 4 days"
    vec_copy = []
    for attr in task.days: vec_copy.append(attr)
    for attr in vec_copy: task.delete_day(attr)
    assert len(list(task.days)) == 0,"Expected 0 days"
    
    # add and delete crons
    cron = Cron()
    cron.set_week_days( [0, 1, 2, 3, 4, 5, 6]  )
    cron.set_days_of_month( [1, 2, 3, 4, 5, 6 ] )
    cron.set_months(  [1, 2, 3, 4, 5, 6]  )
    start = TimeSlot(0 ,0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries( start, finish, incr, True)  # True means relative to suite start
    cron.set_time_series( ts )

    cron1 = Cron()
    cron1.set_week_days( [0, 1, 2, 3, 4, 5, 6 ] )
    cron1.set_time_series( 1, 30,  True )
    
    cron2 = Cron()
    cron2.set_week_days( [0, 1, 2, 3, 4, 5, 6] )
    cron2.set_time_series( "00:30 01:30 00:01" )
    
    cron3 = Cron()
    cron3.set_week_days( [0, 1, 2, 3, 4, 5, 6] )
    cron3.set_time_series( "+00:30" )

    task.add_cron( cron ) \
        .add_cron( cron1 ) \
        .add_cron( cron2 ) \
        .add_cron( cron3 )
    assert len(list(task.crons)) == 4,"Expected 4 crons"
    vec_copy = []
    for attr in task.crons: vec_copy.append(attr)
    for attr in vec_copy: task.delete_cron(attr)
    assert len(list(task.crons)) == 0,"Expected 0 crons"  
    
    
    # add autocancel
    t1 = Task("t1")
    t1.add_autocancel( 3 ).add_variable("A","j")                       
    t3 = Task("t3")
    t3.add_autocancel( 20, 10, True ).add_variable("B","j") 
    t4 = Task("t4")
    t4.add_autocancel( TimeSlot(10, 10), True ).add_variable("C","j") 
    t5 = Task("t5")
    t5.add_autocancel( Autocancel(1, 10, True) ).add_variable("D","j") 
    
    
    # add late
    late = Late()
    late.submitted( TimeSlot(20, 10) )
    late.active(    TimeSlot(20, 10))
    late.complete(  TimeSlot(20, 10), True)
    task.add_late(late ).add_variable("FRED33","j") 
        
    
    # add defstatus, last one set takes effect
    task.add_defstatus( DState.complete )\
        .add_defstatus( DState.queued )\
        .add_defstatus( DState.aborted )\
        .add_defstatus( DState.submitted )\
        .add_defstatus( DState.suspended )\
        .add_defstatus( DState.active )
    
    #
    # add clock
    #
    clock = Clock(1, 1, 2010, False)     # day,month, year, hybrid
    clock.set_gain(1, 10, True)         # True means positive gain
    suite =  Suite("suite")
    suite.add_clock(clock).add_variable("fred1","j") 
    
    end_clock = Clock(1, 1, 2017, False)     # day,month, year, hybrid
    suite.add_end_clock(end_clock).add_variable("fred2","j") 
    
    clock = Clock(1, 1, 2011, True)       # day,month, year, hybrid
    clock.set_gain_in_seconds(12, True)
    s1 =  Suite("s1")
    s1.add_clock(clock).add_variable("fred3","j") 
    s1.add_end_clock(end_clock).add_variable("fred4","j") 
    
    #
    # Add zombie. Note we can *NOT* add two zombie attributes of the same ZombieType
    #
    zombie_life_time_in_server = 800
    child_list = [ ChildCmdType.init, ChildCmdType.event, ChildCmdType.meter, ChildCmdType.label, ChildCmdType.wait, ChildCmdType.abort, ChildCmdType.complete ]
    zombie_type_list = [ ZombieType.ecf, ZombieType.user, ZombieType.path ]
    count = 1;
    for zombie_type in zombie_type_list:
        zombie_attr = ZombieAttr(zombie_type, child_list, ZombieUserActionType.block, zombie_life_time_in_server)
        s1.add_zombie(zombie_attr).add_variable("afred" + str(count),"j") 
        count += 1
    assert len(list(s1.zombies)) == 3,"Expected 3 zombie attributes but found " + str(len(list(s1.zombies)))
    
    # delete all the zombies
    s1.delete_zombie("")
    assert len(list(s1.zombies)) == 0,"Expected zero zombie attributes but found " + str(len(list(s1.zombies)))

    # repeat the the test with empty child list. Empty child list means apply to all child commands
    # + don't specify zombie_life_time_in_server as this is optional
    child_list = [ ]
    zombie_type_list = [ ZombieType.ecf, ZombieType.user, ZombieType.path ]
    for zombie_type in zombie_type_list:
        zombie_attr = ZombieAttr(zombie_type, child_list, ZombieUserActionType.block)
        s1.add_zombie(zombie_attr).add_variable("bfred" + str(count),"j") 
        
        # test delete of specific zombie attribute
        assert len(list(s1.zombies)) == 1,"Expected 1 zombie attributes but found " + str(len(list(s1.zombies)))
        s1.delete_zombie(zombie_type)
        assert len(list(s1.zombies)) == 0,"Expected 0 zombie attributes but found " + str(len(list(s1.zombies)))
        count += 1;

    print("All Tests pass")
    
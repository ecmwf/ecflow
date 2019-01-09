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

print(ecflow.__doc__)
print(ecflow.Defs.__doc__)
    
from ecflow import Suite, Family, Task, Defs, Clock, DState, PartExpression, Variable, Limit, InLimit, \
                   Date, Day, Event, Meter, Label, Autocancel, Days, TimeSlot, TimeSeries, Style, State, \
                   RepeatString, RepeatDate, RepeatInteger, RepeatDay, RepeatEnumerated, \
                   Verify, PrintStyle, Time, Today, Late, Cron, Client, debug_build,Ecf
import ecflow_test_util as Test

if __name__ == "__main__":
    
    Test.print_test_start(os.path.basename(__file__))

    suite = Suite("s1")
    assert (isinstance(suite, ecflow.Suite)), "Expected suite"
    assert (not isinstance(suite, ecflow.Family)), "Expected suite"
    assert (not isinstance(suite, ecflow.Task)), "Expected suite"
 
    family = Family("f1")
    assert (isinstance(family, ecflow.Family)), "Expected Family"
    assert (not isinstance(family, ecflow.Suite)), "Expected Family"
    assert (not isinstance(family, ecflow.Task)), "Expected Family"
    suite.add_family(family)

    task = Task("f1")
    assert (isinstance(task, ecflow.Task)), "Expected Task"
    assert (not isinstance(task, ecflow.Suite)), "Expected Task"
    assert (not isinstance(task, ecflow.Family)), "Expected Task"
    family.add_task(task)

    
    defs = Defs()
    defs2 = Defs()
    assert defs == defs2, "Empty defs should be equal"
 
    assert defs.get_state() == ecflow.State.unknown, "Expected default state to be unknown"
    defs.add_suite(suite);  # add existing suite to defs
 
    suite = defs.add_suite("s2");
    suite.add_variable(Variable("ECF_HOME","/tmp/"))
    suite.add_variable("ECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%")
    suite.add_variable("ECF_URL_BASE","http://www.ecmwf.int")
    suite.add_variable("ECF_URL","publications/manuals/sms")
    suite.add_limit( Limit("limitName", 10) )
    suite.add_limit( "limitName_2", 10 )
    assert suite.begun() == False, "Suite should not have begun"

    clock = Clock(1,1,2010,False);
    clock.set_gain(1,10,True);
    suite.add_clock(clock);

    assert defs != defs2, "Defs should no longer compare as they are different"
    
    defs.add_extern("/path/to/task");
    defs.add_extern("/fred/bill:event_name");
    
    family = suite.add_family("f1")
    family.add_defstatus( DState.active );

    task = family.add_task("t1")
    task.add_trigger( "t2 == active" )
    task.add_complete( "t2 == complete" )
    task.add_autocancel( Autocancel(3) );

    task2 = family.add_task("t2")
    task2.add_defstatus(  DState.complete );
    task2.add_part_trigger(  PartExpression("t1 == complete") )
    task2.add_part_trigger(  PartExpression("t1 == active",True) )  #  for long and/or expressions, subsequent expr must be and/or
    task2.add_part_complete( PartExpression("t1 == complete") )
    task2.add_part_complete( PartExpression("t1 == active",False) )  #  for long and/or expressions, subsequent expr must be and/or
    task2.add_label(      Label("labelname", "label1") )
    task2.add_label(      "labelname2", "label1")
    task2.add_inlimit(    InLimit("limitName","/s1/f1",2) )
    task2.add_inlimit(    InLimit("limitName1","/s1/f1",2,True) )
    task2.add_inlimit(    "limitName2" )
    task2.add_inlimit(    "limitName3","path" )
    task2.add_inlimit(    "limitName4","path",1 )
    task2.add_inlimit(    "limitName5","path",1,True )
    task2.add_event(      10 )
    task2.add_event(      Event(10,"Eventname") )
    task2.add_meter(      Meter("metername",0,100,50) )
    task2.add_meter(      "metername2",0,100,50 )
    task2.add_date(       Date(1,1,2010) )
    task2.add_date(       1,2,2010 )
    task2.add_day(        Day(Days.sunday) )
    task2.add_day(        Days.monday )
    task2.add_autocancel( Autocancel(TimeSlot(20,10),True) )

    task2.add_today(      0,10 )
    task2.add_today(      Today( 0,59, True ))
    task2.add_today(      Today(TimeSlot(20,10)) )
    task2.add_today(      Today(TimeSlot(20,20),False)) 
    start = TimeSlot(0,0)
    finish = TimeSlot(23,0)
    incr = TimeSlot(0,30)
    ts = TimeSeries( start, finish, incr, True);
    task2.add_today(      Today(ts) )

    task2.add_time(       20,10 )
    task2.add_time(       Time(20,12,True) )
    task2.add_time(       Time(TimeSlot(20,12))) 
    task2.add_time(       Time(TimeSlot(20,20),True)) 
    task2.add_time(       Time(ts) )
    
    late = Late()
    late.submitted( 20,10 )
    late.active(TimeSlot(20,10))
    late.complete(TimeSlot(20,10),True)
    task2.add_late(  late )
        
    cron = Cron()
    cron.set_week_days( [0,1,2,3,4,5,6] )
    cron.set_days_of_month(  [1,2,3,4,5,6] )
    cron.set_months( [1,2,3,4,5,6] )
    cron.set_time_series( ts )
    task2.add_cron( cron );
    
    task3 = family.add_task("t3")
    task3.add_repeat( RepeatDate("testDate",20100111,20100115,2) )
    task3.add_defstatus( DState.queued );
    task3.add_verify( Verify(State.complete, 1) )
    task3.add_autocancel( Autocancel(20,10,False) )
    task3.add_label( Label("label_name","label_value") )
    
    task3_1 = family.add_task("t3_1")
    task3_1.add_repeat( RepeatDate("testDate",20100111,20100115) )
    

    task4 = family.add_task("t4")
    task4.add_repeat( RepeatInteger("testInteger",0,100,2) )
    task4.add_defstatus( DState.aborted );
    
    task4_1 = family.add_task("t4_1")
    task4_1.add_repeat( RepeatInteger("testInteger",0,100) )
 
    
    task5 = family.add_task("t5")
    task5.add_repeat( RepeatEnumerated("testInteger", ["red", "green", "blue" ] ) )
    task5.add_defstatus( DState.submitted );
        
    task6 = family.add_task("t6")
    task6.add_repeat( RepeatString("test_string",  ["a", "b", "c" ] ) )
    task6.add_defstatus( DState.suspended );
    
    task7 = family.add_task("t7")
    task7.add_repeat( RepeatDay(2) )
    task7.add_defstatus( DState.active );
    
    task8 = family.add_task("t8")
    task8.add_repeat( RepeatDay() )
    print("task8.get_try_no()    :string:",task8.get_try_no())
    print("task8.get_int_try_no():int   :",task8.get_int_try_no())
    
    the_defs = task8.get_defs()
 
          
    # Test find      
    assert defs.find_suite("s1") != None, "Expected to find suite of name s1"
    assert defs.find_suite("sffff1") == None, "Should not be able to find suite of name sfffff1"
        
    # save the defs file as a check point file and restore it again
    checkpt_file = "py_u_TestDefs.check"
    defs_file = "py_u_TestDefs.def"
    if debug_build() :
        checkpt_file = "py_u_TestDefs_debug.check"
        defs_file = "py_u_TestDefs_debug.def"
    defs.save_as_checkpt(checkpt_file);
    
    restored_checkpt_defs = Defs()
    restored_checkpt_defs.restore_from_checkpt(checkpt_file)
    
    print("Save and restore as Defs ******************************************************")
    defs.save_as_defs(defs_file)           # default is to save as DEFS
    restored_from_defs = Defs( defs_file ) # restore the defs

    defs.save_as_defs(defs_file,Style.DEFS)
    restored_from_defs = Defs( defs_file ) # restore the defs
            
    print("Save and restore using STATE ******************************************************")
    defs.save_as_defs(defs_file,Style.STATE)
    restored_from_defs = Defs( defs_file ) # restore the defs

    print("Save and restore using MIGRATE ******************************************************")
    defs.save_as_defs(defs_file,Style.MIGRATE)
    restored_from_defs = Defs( defs_file ) # restore the defs
    assert restored_checkpt_defs == restored_from_defs ,"File should be the same"

        
    print("Print in DEFS style ******************************************************")
    PrintStyle.set_style(Style.DEFS)
    the_string = str(restored_from_defs)
    assert the_string.find("defs_state") == -1, "Print in DEFS style failed"
    print(the_string)
    
    print("Print in STATE style *****************************************************")
    PrintStyle.set_style(Style.STATE)
    the_string = str(restored_from_defs)
    assert the_string.find("defs_state STATE") != -1, "Print in STATE style failed"
    print(the_string)
    
    print("Print in MIGRATE style *****************************************************")
    PrintStyle.set_style(Style.MIGRATE)
    the_string = str(restored_from_defs)
    assert the_string.find("defs_state MIGRATE") != -1, "Print in MIGRATE style failed"
    print(the_string)
     
    # Comment this out if you want to see what the file looked like
    os.remove(checkpt_file)
    os.remove(defs_file)
    
    # Test job creation failure
    msg = defs.check_job_creation()
    assert len(msg) != 0 ,"Expected job creation to fail"
    
    expected_exeption = False;
    try:
        defs.check_job_creation(throw_on_error=True) 
    except RuntimeError as e:
        expected_exeption = True
    assert expected_exeption,"expected exception"  
 
    print("All tests pass")
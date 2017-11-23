#!/usr/bin/env python2.7
import os
import ecflow  

def create_family_f1():
    f1 = ecflow.Family("f1")
    f1.add_variable("SLEEP", 20)
    f1.add_task("t1").add_meter("progress", 1, 100, 90)
    
    t2 = f1.add_task("t2")  
    t2.add_trigger("t1 eq complete") 
    t2.add_event("a")
    t2.add_event("b")
    
    f1.add_task("t3").add_trigger("t2:a")  
    f1.add_task("t4").add_trigger("t2 eq complete").add_complete("t2:b")  
    f1.add_task("t5").add_trigger("t1:progress ge 30")  
    f1.add_task("t6").add_trigger("t1:progress ge 60")  
    f1.add_task("t7").add_trigger("t1:progress ge 90") 
    return f1 

def create_family_f2():
    f2 = ecflow.Family("f2")
    f2.add_variable("SLEEP", 20)
    f2.add_task("t1").add_time( "00:30 23:30 00:30" ) # start(hh:mm) end(hh:mm) increment(hh:mm)
    f2.add_task("t2").add_day( "sunday" )
   
    # for add_date(): day,month,year; here 0 means every month, and every year
    t3 = f2.add_task("t3")
    t3.add_date(1, 0, 0)           # day month year, first of every month or every year
    t3.add_time( 12, 0 )           # hour, minutes  at 12 o'clock
   
    f2.add_task("t4").add_time( 0, 2, True ) # hour, minutes, relative to suite start
                                             # 2 minutes after family f2 start
    f2.add_task("t5").add_time( 0, 2 )       # hour, minutes   suite site
                                             # 2 minutes past midnight
    return f2
            
print "Creating suite definition"   
defs = ecflow.Defs()
suite = defs.add_suite("test")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")

suite.add_family( create_family_f1() )
suite.add_family( create_family_f2() )
print defs

print "Checking job creation: .ecf -> .job0"   
print defs.check_job_creation()

print "Checking trigger expressions"
print defs.check()

print "Saving definition to file 'test.def'"
defs.save_as_defs("test.def")
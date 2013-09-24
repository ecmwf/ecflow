#!/usr/bin/env python2.7
import os
import ecflow  
import sys

version = sys.version_info;
if  version[1] < 7 : 
    print "This example requires python version 2.7, but found : " + str(version)
    exit(0)

print "Creating suite definition"  
with ecflow.Defs() as defs: 
    
    with defs.add_suite("test") as suite:
        
        suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
        suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")
        
        with suite.add_family("f1") as f1:
            f1.add_variable("SLEEP", 20)
            f1.add_task("t1").add_meter("progress", 1, 100, 90)
            f1.add_task("t2").add_trigger("t1 eq complete").add_event("a").add_event("b")
            f1.add_task("t3").add_trigger("t2:a")  
            f1.add_task("t4").add_trigger("t2 eq complete").add_complete("t2:b")  
            f1.add_task("t5").add_trigger("t1:progress ge 30")  
            f1.add_task("t6").add_trigger("t1:progress ge 60")  
            f1.add_task("t7").add_trigger("t1:progress ge 90") 
    
        with suite.add_family("f2") as f2:        
            f2.add_variable("SLEEP", 20)
            f2.add_task("t1").add_time( "00:30 23:30 00:30" )
            f2.add_task("t2").add_day( "sunday" )
            f2.add_task("t3").add_date(1, 0, 0).add_time( 12, 0 )
            f2.add_task("t4").add_time( 0, 2, True )
            f2.add_task("t5").add_time( 0, 2 )
            
    print defs

    print "Checking job creation: .ecf -> .job0"   
    print defs.check_job_creation()

    print "Checking trigger expressions"
    print defs.check()

    print "Saving definition to file 'test.def'"
    defs.save_as_defs("test.def")
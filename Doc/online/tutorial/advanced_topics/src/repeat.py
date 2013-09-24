#!/usr/bin/env python2.7
import os
import ecflow  
    
def create_family_f4():
    f4 = ecflow.Family("f4")
    f4.add_variable("SLEEP", 2)
    f4.add_repeat( ecflow.RepeatString("NAME", ["a", "b", "c", "d", "e", "f" ] ) )
   
    f5 = f4.add_family("f5")
    f5.add_repeat( ecflow.RepeatInteger("VALUE", 1, 10) )
   
    t1 = f5.add_task("t1")
    t1.add_repeat( ecflow.RepeatDate("DATE", 20101230, 20110105) )
    t1.add_label("info", "")
    return f4
    
print "Creating suite definition"   
defs = ecflow.Defs()
suite = defs.add_suite("test")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")

suite.add_family( create_family_f4() )
print defs

print "Checking job creation: .ecf -> .job0"   
print defs.check_job_creation()

print "Saving definition to file 'test.def'"
defs.save_as_defs("test.def")
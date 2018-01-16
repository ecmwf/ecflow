#!/usr/bin/env python2.7
import os
import ecflow

def create_family_f5() :
    f5 = ecflow.Family("f5")
    f5.add_inlimit("l1")
    f5.add_variable("HOST", "??????")
    f5.add_variable("ECF_JOB_CMD", "rsh %HOST% '%ECF_JOB% > %ECF_JOBOUT% 2>&1 &'")
    f5.add_variable("SLEEP", 20)
    for i in range(1, 10):
        f5.add_task( "t" + str(i) )
    return f5
          
print "Creating suite definition"   
defs = ecflow.Defs()
suite = defs.add_suite("test")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")

suite.add_limit("l1", 2)
suite.add_family( create_family_f5() )
print defs

print "Checking job creation: .ecf -> .job0"   
print defs.check_job_creation()

print "Saving definition to file 'test.def'"
defs.save_as_defs("test.def")

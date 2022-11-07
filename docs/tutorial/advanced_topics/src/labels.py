#!/usr/bin/env python2.7
import os
import ecflow 
 
print "Creating suite definition"   
defs = ecflow.Defs()
suite = defs.add_suite("test")
suite.add_variable("ECF_INCLUDE", os.getenv("HOME") + "/course")
suite.add_variable("ECF_HOME",    os.getenv("HOME") + "/course")

suite.add_family("f3").add_task("t1").add_label("info","") 
print defs

print "Checking job creation: .ecf -> .job0"   
print defs.check_job_creation()

print "Saving definition to file 'test.def'"
defs.save_as_defs("test.def") 
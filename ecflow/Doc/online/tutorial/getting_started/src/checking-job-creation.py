#!/usr/bin/env python2.7
import os
import ecflow 
   
print "Creating suite definition"   
defs = ecflow.Defs()
suite = defs.add_suite("test")
suite.add_variable("ECF_HOME", os.getenv("HOME") + "/course")
suite.add_task("t1")
print defs

print "Checking job creation: .ecf -> .job0"   
print defs.check_job_creation()

# We can assert, so that we only progress, once all job creation works
# assert len(defs.check_job_creation()) == 0, "Job generation failed"

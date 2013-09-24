#!/usr/bin/env python2.7
import ecflow 
   
try:
    print "Loading definition in 'test.def' into the server"
    ci = ecflow.Client();   
    ci.load("test.def")      # read definition from disk and load into the server
except RuntimeError, e:
    print "Failed: " + str(e); 
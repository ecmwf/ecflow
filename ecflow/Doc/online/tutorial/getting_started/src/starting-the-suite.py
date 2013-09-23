#!/usr/bin/env python2.7
import ecflow 
   
try:
    print "Loading definition in 'test.def' into the server"
    ci = ecflow.Client();   
    ci.load("test.def")      
    
    print "Restarting the server. This starts job scheduling"
    ci.restart_server()      
    
    print "Begin the suite named 'test'"     
    ci.begin_suite("test")   
    
except RuntimeError, e:
    print "Failed: " + str(e); 
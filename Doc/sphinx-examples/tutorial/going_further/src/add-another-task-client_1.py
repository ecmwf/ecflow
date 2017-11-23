#!/usr/bin/env python2.7
import ecflow 
   
print "Client -> Server: replacing suite '/test' in the server, with a new definition"   
try:
    ci = ecflow.Client()
    ci.suspend("/test")              # so that we can resume manually in ecflowview
    ci.replace("/test", "test.def")     
except RuntimeError, e:
    print "Failed: " + str(e)
    
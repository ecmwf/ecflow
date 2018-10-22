# Used to build ecflow, incrementally and nightly
import os
import ecflow 

# =====================================================================================
print "delete the definition in the server"
# ======================================================================================
ci = ecflow.Client("localhost:4141")
try:
    ci.restart_server();
    ci.delete_all(True)           # clear out the server
    #ci.load(generated_defs_file)  # load the definition into the server, from disk
    #ci.load(defs)                # load the definition into the server, from memory
    #ci.replace("/suite",generated_defs_file,True,True)     
    ci.begin_all_suites()         # need only begin once 
 
except RuntimeError, e:
    print "failed: " + str(e)


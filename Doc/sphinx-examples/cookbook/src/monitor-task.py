#!/usr/bin/env python2.7
import ecflow
import time

def monitor_critical_task(ci, path_to_task):
    
    # Query the server for any changes
    if ci.news_local():
            
        # get the incremental changes, and merge with defs stored on the Client 
        ci.sync_local()
        
        # check to see if definition exists in the server
        defs = ci.get_defs()
        if defs == None :
            exit(0) # return
            
        # find the task we are interested in  
        critical_task = defs.find_abs_node(path_to_task)
        if critical_task == None:
            # No such task
            exit(0) # return
             
        # Check to see if task was aborted, if it was email me the job output
        if critical_task.get_state() == ecflow.State.aborted:
                
            # Get the job output
            the_aborted_task_output = ci.get_file(path_to_task,'jobout')  
            # email(the_aborted_task_output)
            exit(0)
                
try:
    # Create the client. This will read the default environment variables
    ci = ecflow.Client("localhost", "4143")

    # Continually monitor the suite
    while 1:

        monitor_critical_task(ci, "/suite/critical_node")
                
        # Sleep for 5 minutes. 
        # To avoid overloading server ensure sleep is > 60 seconds 
        time.sleep(300)
        
except RuntimeError, e:
    print str(e)

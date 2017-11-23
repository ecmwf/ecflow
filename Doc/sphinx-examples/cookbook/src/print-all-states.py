#!/usr/bin/env python2.7
import ecflow

try:
    # Create the client
    ci = ecflow.Client("localhost", "4143")
    
    # Get the node tree suite definition as stored in the server
    # The definition is retrieved and stored on the variable 'ci'
    ci.sync_local()

    # access the definition retrieved from the server
    defs = ci.get_defs()
    
    if defs == None :
        print "The server has no definition"
        exit(1)
    
    # get the tasks, *alternatively* could use defs.get_all_nodes()  
    # to include suites, families and tasks.
    task_vec = defs.get_all_tasks()
 
    # iterate over tasks and print path and state
    for task in task_vec:
        print task.get_abs_node_path()  + " "  + str(task.get_state())
        
except RuntimeError, e:
    print "Failed: " + str(e)
    
#Migration
#
#STEP 1:  
#   o first shutdown the old server
#   o suspend all the suites
#   o record all the currently aborted tasks, via a python script & set them to unknown state,
#   o kill all the active tasks, and wait for them to abort.
#   o halt the server
#   o Used --migrate to dump the state & structure to a file.
#   o terminate the server
#
#STEP 2: manual step
#   o move the check point file, to stop server from loading them
#   o start the new server using the same port number
#
#STEP 3: 
#   o Load the migration file.
#   o restart the server, and resume all suspended suites
#   o Re-queue all the aborted tasks
#   o re-set tasks that were previously aborted, back to abort state


#!/usr/bin/env python2.7
#///////////////////////////////////////////////////////////////////////
#// STEP 1 migrate_part1.py
#///////////////////////////////////////////////////////////////////////
import ecflow

try:
    # ==================================================================
    # Create the client
    # ==================================================================
    ci = ecflow.Client("localhost", "4143")
    
    # ==================================================================
    # shutdown the server 
    # ==================================================================
    ci.shutdown_server()

    # ==================================================================
    # suspend all the suites    
    #    get the definition from the server
    # ==================================================================
    ci.sync_local()
    defs = ci.get_defs()
    if defs == None :
        print "The server has no definition"
        exit(1)
    for suite in defs.suites:
       ci.suspend(suite.get_abs_node_path())    
    
    # ==================================================================
    # iterate over tasks and record aborted states
    # *Required* so that we can distinguish between those active task we
    # kill and hence abort, and those already aborted before migration starts
    # ==================================================================
    # We open for append to allow this to be rerun multiple times
    task_vec = defs.get_all_tasks()
    list_of_task_paths = {}
    with open("aborted_tasks.txt",'a') as file:
        for task in task_vec:
            if task.get_state() == State.aborted:
                file.writeln( task.get_abs_node_path() )
                list_of_task_paths.append(task.get_abs_node_path())

    # ======================================================================
    # set the aborted states to unknown. i,e we do not want to re-queue these    
    # =======================================================================
    if len(list_of_task_paths) != 0:
       try:
           ci.force_state(list_of_task_paths,State.unknown)
       except RuntimeError, e:
           print "Could not force to the unknown state: " + str(e)

    # ==============================================================================
    # Kill all active and submitted tasks, (these we want to re-queue in new server)
    # ===============================================================================
    for suite in ci.get_defs().suites:
       ci.kill(suite.get_abs_node_path())    
 
    # ==================================================================
    # Wait 5 minutes, for all active tasks to die
    # ==================================================================
    count = 0
    while 1:
        count += 1
        ci.sync_local() # get the changes, synced with local defs
        
        task_vec = ci.get_defs().get_all_tasks()
        for task in task_vec:
            if task.get_state() == State.active:
               continue;   
                    
        time.sleep(60)
        if count > 5:
            assert False, "test_client_run aborted after " + str(count) + " loops:\n" + str(ci.get_defs())         

    # ==================================================================
    # halt the server 
    # ==================================================================
    ci.halt_server()

    # ==================================================================
    # dump the state & structure to a file, in format that a new release 
    # can read
    # ==================================================================
    ci.sync_local()  
    ci.get_defs().save_as_defs("migrate.defs",Style.MIGRATE);
    
    # ==================================================================
    # terminate the server 
    # ==================================================================
    ci.terminate_server()
    
except RuntimeError, e:
    print "Failed: " + str(e)
    
-------------------------------------------------------------------------------------    

   
#!/usr/bin/env python2.7
# ======================================================================
# STEP 2: migrate_part2.py
# ======================================================================
import ecflow
try:
    # ==================================================================
    # Create the client
    # ==================================================================
    ci = ecflow.Client("localhost", "4143")
    
    # ==================================================================
    # Load the file
    # ==================================================================
    ci.load("migrate.defs")
    
    # ==================================================================
    # restart the server 
    # ==================================================================
    ci.restart_server()    

    # ==================================================================
    # Re-queue all the aborted tasks
    # ==================================================================
    ci.sync_local()
    defs = ci.get_defs()
    if defs == None :
        print "The server has no definition"
        exit(1)
    
    for suite in ci.get_defs().suites:
       ci.requeue(suite.get_abs_node_path(),"aborted")    
    
    # ==================================================================
    # re-set tasks that were previously aborted, back to abort state
    # Open the file that recorded the list of aborted tasks
    # ==================================================================
    list_of_aborted_tasks = {}
    with open("aborted_tasks.txt",'r') as file:
       for line in file:
           list_of_aborted_tasks.append(line.rstrip(('\n'))  # remove the trailing new line
    
    # ==================================================================
    # set these tasks to state aborted
    # ==================================================================
    if len(list_of_aborted_tasks) != 0:
       try:
           ci.force_state(list_of_aborted_tasks,State.aborted)
       except RuntimeError, e:
           print "Could not force to the aborted state:\n" + str(e)
  
except RuntimeError, e:
    print "Failed: " + str(e)
    

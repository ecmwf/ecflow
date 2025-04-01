
.. _force_cli:

force
/////

::

   
   force
   -----
   
   Force a node to a given state, or set its event.
   When a task is set to complete, it may be automatically re-queued if it has
   multiple future time dependencies. However each time we force a complete it will
   expire any time based attribute on that node. When the last time based attribute
   expires, the node will stay in a complete state.
   This behaviour allow Repeat values to be incremented interactively.
   A repeat attribute is incremented when all the child nodes are complete
   in this case the child nodes are automatically re-queued.
     arg1 = [ unknown | complete | queued | submitted | active | aborted | clear | set ]
     arg2 = (optional) recursive
            Applies state to node and recursively to all its children
     arg3 = (optional) full
            Set repeat variables to last value, only works in conjunction
            with recursive option
     arg4 = path_to_node or path_to_node:<event>: paths must begin with '/'
   Usage:
     --force=complete /suite/t1 /suite/t2   # Set task t1 & t2 to complete
     --force=clear /suite/task:ev           # Clear the event 'ev' on task /suite/task
     --force=complete recursive /suite/f1   # Recursively set complete all children of /suite/f1
   Effect:
     Consider the effect of forcing complete when the current time is at 09:00
     suite s1
        task t1; time 12:00             # will complete straight away
        task t2; time 10:00 13:00 01:00 # will complete on fourth attempt
   
     --force=complete /s1/t1 /s1/t2
     When we have a time range(i.e as shown with task t2), it is re-queued and the
     next time slot is incremented for each complete, until it expires, and the task completes.
     Use the Why command, to show next run time (i.e. next time slot)
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable secure communication between client and server.            |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   

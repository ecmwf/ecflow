
.. _run_cli:

run
///

::

   
   run
   ---
   
   Ignore triggers, limits, time or date dependencies, just run the Task.
   When a job completes, it may be automatically re-queued if it has a cron
   or multiple time dependencies. If we have multiple time based attributes,
   then each run, will expire the time.
   When we run before the time, we want to avoid re-running the task then
   a flag is set, so that it is not automatically re-queued.
   A repeat attribute is incremented when all the child nodes are complete
   in this case the child nodes are automatically re-queued.
   Hence this command can be aid, in allowing a Repeat attribute to be incremented
     arg1 = (optional)force
            Forcibly run, even if there are nodes that are active or submitted
            This can result in zombie creation
     arg2 = node path(s). The paths must begin with a leading '/' character.
            If the path is /suite/family will recursively run all tasks
            When providing multiple paths avoid running the same task twice
   Example:
     --run=/suite/t1                    # run task t1
   Effect:
        task t1; time 12:00             # will complete if run manually
        task t2; time 10:00 13:00 01:00 # will run 4 times before completing
    When we have a time range(i.e as shown with task t2), then next time slot
    is incremented for each run, until it expires, and the task completes.
    Use the Why command, to show next run time (i.e. next time slot)
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable encrypted comms with SSL enabled server.                   |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   

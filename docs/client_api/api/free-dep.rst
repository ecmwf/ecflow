
.. _free-dep_cli:

free-dep
////////

::

   
   free-dep
   --------
   
   Free dependencies for a node. Defaults to triggers
   After freeing the time related dependencies (i.e time,today,cron)
   the next time slot will be missed.
     arg1 = (optional) trigger
     arg2 = (optional) all
            Free trigger, date and all time dependencies
     arg3 = (optional) date
            Free date dependencies
     arg4 = (optional) time
            Free all time dependencies i.e time, day, today, cron
     arg5 = List of paths. At least one required. Must start with a leading '/'
   Usage:
     --free-dep=/s1/t1 /s2/t2   # free trigger dependencies for task's t1,t2
     --free-dep=all /s1/f1/t1   # free all dependencies of /s1/f1/t1
     --free-dep=date /s1/f1     # free holding date dependencies of /s1/f1
   
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
   

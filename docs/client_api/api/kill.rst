
.. _kill_cli:

kill
////

::

   
   kill
   ----
   
   Kills the job associated with the node.
   If a family or suite is selected, will kill hierarchically.
   Kill uses the ECF_KILL_CMD variable. After variable substitution it is invoked as a command.
   The command should be written in such a way that the output is written to %ECF_JOB%.kill
   as this allow the --file command to report the output: .e.e.
    /home/ma/emos/bin/ecfkill %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1::
   Usage::
      --kill=/s1/f1/t1 /s1/f2/t2 # kill the jobs for tasks t1 and t2
      --file=/s1/f1/t1 kill      # write to standard out the '.kill' file for task /s1/f1/t1
   
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
   

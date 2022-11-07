
.. _init_cli:

init
////

::

   
   init
   ----
   
   Mark task as started(active). For use in the '.ecf' script file *only*
   Hence the context is supplied via environment variables.
     arg1(string)         = process_or_remote_id The process id of the job or remote_id
                            Using remote id allows the jobs to be killed
     arg2(--add)(optional)= add/update variables as name value pairs
   
   If this child command is a zombie, then the default action will be to *block*.
   The default can be overridden by using zombie attributes.
   Otherwise the blocking period is defined by ECF_TIMEOUT.
   
   Usage:
     ecflow_client --init=$$
     ecflow_client --init=$$ --add name=value name2=value2 # add/update variables to task
   
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
   
   The following environment variables are specific to child commands.
   The scripts should export the mandatory variables. Typically defined in the head/tail includes files
   
   |--------------|----------|-----------|---------------------------------------------------------------|
   | Name         |  Type    | Required  | Description                                                   |
   |--------------|----------|-----------|---------------------------------------------------------------|
   | ECF_NAME     | <string> | Mandatory | Full path name to the task                                    |
   | ECF_PASS     | <string> | Mandatory | The jobs password, allocated by server, then used by server to|
   |              |          |           | authenticate client request                                   |
   | ECF_TRYNO    |  <int>   | Mandatory | The number of times the job has run. This is allocated by the |
   |              |          |           | server, and used in job/output file name generation.          |
   | ECF_RID      | <string> | Mandatory | The process identifier. Helps zombies identification and      |
   |              |          |           | automated killing of running jobs                             |
   | ECF_TIMEOUT  |  <int>   | optional  | Max time in *seconds* for client to deliver message to main   |
   |              |          |           | server. The default is 24 hours                               |
   | ECF_HOSTFILE | <string> | optional  | File that lists alternate hosts to try, if connection to main |
   |              |          |           | host fails                                                    |
   | ECF_DENIED   |  <any>   | optional  | Provides a way for child to exit with an error, if server     |
   |              |          |           | denies connection. Avoids 24hr wait. Note: when you have      |
   |              |          |           | hundreds of tasks, using this approach requires a lot of      |
   |              |          |           | manual intervention to determine job status                   |
   | NO_ECF       |  <any>   | optional  | If set exit's ecflow_client immediately with success. This    |
   |              |          |           | allows the scripts to be tested independent of the server     |
   |--------------|----------|-----------|---------------------------------------------------------------|
   

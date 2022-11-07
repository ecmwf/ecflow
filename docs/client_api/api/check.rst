
.. _check_cli:

check
/////

::

   
   check
   -----
   
   Checks the expression and limits in the server. Will also check trigger references.
   Trigger expressions that reference paths that don't exist, will be reported as errors.
   (Note: On the client side unresolved paths in trigger expressions must
   have an associated 'extern' specified)
     arg = [ _all_ | / | list of node paths ]
   Usage:
     --check=_all_           # Checks all the suites
     --check=/               # Checks all the suites
     --check=/s1 /s2/f1/t1   # Check suite /s1 and task t1
   
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
   

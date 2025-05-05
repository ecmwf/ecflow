
.. _ch_rem_cli:

ch_rem
//////

::

   
   ch_rem
   ------
   
   Remove a set of suites, from an existing handle.
      arg1 = handle(integer)   # The handle must be an integer that is > 0
      arg2 = names             # should be a list of suite names, names not in the definition are ignored
   Usage:
      --ch_rem=10 s2 s3 s4     # remove suites s2 s3,s4 from handle 10
   The handle is created with --ch_register command
   To list all suites and handles use --ch_suites
   
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
   

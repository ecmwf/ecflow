
.. _archive_cli:

archive
///////

::

   
   archive
   -------
   
   Archives suite or family nodes *IF* they have child nodes(otherwise does nothing).
   Saves the suite/family nodes to disk, and then removes the child nodes from the definition.
   This saves memory in the server, when dealing with huge definitions that are not needed.
   It improves time taken to checkpoint and reduces network bandwidth.
   If the node is re-queued or begun, the child nodes are automatically restored.
   Use --restore to reload the archived nodes manually
   Care must be taken if you have trigger reference to the archived nodes
   The nodes are saved to the file ECF_HOME/<hostname>.<port>.<ECF_NAME>.check,
   where '/' has been replaced with ':' in ECF_NAME
   
   Nodes like (family and suites) can also to automatically archived by using,
   the 'autoarchive' attribute. The attribute is only applied once the node is complete
   
   suite autoarchive
    family f1
       autoarchive +01:00 # archive one hour after complete
       task t1
    endfamily
    family f2
        autoarchive 01:00 # archive at 1 am in morning after complete
       task t1
    endfamily
    family f3
       autoarchive 10     # archive 10 days after complete
       task t1
    endfamily
    family f4
       autoarchive 0      # archive immediately (upto 60s) after complete
       task t1
     endfamily
   endsuite
   
   Usage::
      --archive=/s1           # archive suite s1
      --archive=/s1/f1 /s2    # archive family /s1/f1 and suite /s2
      --archive=force /s1 /s2 # archive suites /s1,/s2 even if they have active tasks
   
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
   

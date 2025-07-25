
.. _plug_cli:

plug
////







.. rubric:: Output of :code:`--help=plug`



The following help text is generated by :code:`ecflow_client --help=plug`

::

   
   plug
   ----
   
   Plug command is used to move nodes.
   The destination node can be on another server In which case the destination
   path should be of the form '<host>:<port>/suite/family/task
     arg1 = path to source node
     arg2 = path to the destination node
   This command can fail because:
   - Source node is in a 'active' or 'submitted' state
   - Another user already has an lock
   - source/destination paths do not exist on the corresponding servers
   - If the destination node path is empty, i.e. only host:port is specified,
     then the source node must correspond to a suite.
   - If the source node is added as a child, then its name must be unique
     amongst its peers
   Usage:
     --plug=/suite macX:3141  # move the suite to ecFlow server on host(macX) and port(3141)
   
   The client considers, for both user and child commands, the following environment variables:
   
     ECF_HOST <string> [mandatory*]
       The main server hostname; default value is 'localhost'
     ECF_PORT <int> [mandatory*]
       The main server port; default value is '3141'
     ECF_SSL <any> [optional*]
       Enable secure communication between client and server.
     ECF_HOSTFILE <string> [optional]
       File that lists alternate hosts to try, if connection to main host fails
     ECF_HOSTFILE_POLICY <string> [optional]
       The policy ('task' or 'all') to define which commands consider using alternate hosts.
   
   The options marked with (*) must be specified in order for the client to communicate
   with the server, either by setting the environment variables or by specifying the
   command line options.
   


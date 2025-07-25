
.. _zombie_block_cli:

zombie_block
////////////







.. rubric:: Output of :code:`--help=zombie_block`



The following help text is generated by :code:`ecflow_client --help=zombie_block`

::

   
   zombie_block
   ------------
   
   Locates the task in the servers list of zombies, and sets flags to block it.
   This is default behaviour of the child commands(init,abort,complete,wait,queue)
   when the server cannot match the passwords. Each child commands will continue
   attempting to connect to the server for 24 hours, and will then return an error.
   The connection timeout can be configured with environment ECF_TIMEOUT
     args = list of task paths, at least one expected
     --zombie_block=/path/to/task  /path/to/task2
   
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
   


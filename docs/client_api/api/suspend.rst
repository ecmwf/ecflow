
.. _suspend_cli:

suspend
///////







.. rubric:: Output of :code:`--help=suspend`



The following help text is generated by :code:`ecflow_client --help=suspend`

::

   
   suspend
   -------
   
   Suspend the given node. This prevents job generation for the given node, or any child node.
   Usage::
      --suspend=/s1/f1/t1   # suspend task s1/f1/t1
      --suspend=/s1 /s2     # suspend suites /s1 and /s2
   
   
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
   


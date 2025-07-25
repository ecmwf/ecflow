
.. _requeue_cli:

requeue
///////







.. rubric:: Output of :code:`--help=requeue`



The following help text is generated by :code:`ecflow_client --help=requeue`

::

   
   requeue
   -------
   
   Re queues the specified node(s)
     If any child of the specified node(s) is in a suspended state, this state is cleared
   Repeats are reset to their starting values, relative time attributes are reset.
     arg1 = (optional) [ abort | force ]
            abort  = re-queue only aborted tasks below node
            force  = Force the re-queueing even if there are nodes that are active or submitted
            <null> = Checks if any tasks are in submitted or active states below the node
                     if so does nothing. Otherwise re-queues the node.
     arg2 = list of node paths. The node paths must begin with a leading '/' character
   
   Usage:
     --requeue=abort /suite/f1  # re-queue all aborted tasks of /suite/f1
     --requeue=force /suite/f1  # forcibly re-queue /suite/f1 and all its children.May cause zombies.
     --requeue=/s1/f1/t1 /s1/t2 # Re-queue node '/suite/f1/t1' and '/s1/t2'
   
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
   


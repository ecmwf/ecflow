
.. _free-dep_cli:

free-dep
////////







.. rubric:: Output of :code:`--help=free-dep`



The following help text is generated by :code:`ecflow_client --help=free-dep`

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
   


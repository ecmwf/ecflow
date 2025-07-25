
.. _group_cli:

group
/////







.. rubric:: Output of :code:`--help=group`



The following help text is generated by :code:`ecflow_client --help=group`

::

   
   group
   -----
   
   Allows a series of ';' separated commands to be grouped and executed as one.
   Some commands like halt, shutdown and terminate will prompt the user. To bypass the prompt
   provide 'yes' as an additional parameter. See example below.
     arg = string
   Usage:
      --group="halt=yes; reloadwsfile; restart;"
                                    # halt server,bypass the confirmation prompt,
                                    # reload white list file, restart server
      --group="get; show"           # get server defs, and write to standard output
      --group="get=/s1; show state" # get suite 's1', and write state to standard output
   
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
   



.. _restore_cli:

restore
///////







.. rubric:: Output of :code:`--help=restore`



The following help text is generated by :code:`ecflow_client --help=restore`

::

   
   restore
   -------
   
   Manually restore archived nodes.
   Restore will fail if:
     - Node has not been archived
     - Node has children, (since archived nodes have no children)
     - If the file ECF_HOME/<host>.<port>.<ECF_NAME>.check does not exist
   Nodes can be restored manually(as in this command) but also automatically
   
   Automatic restore is done using the 'autorestore' attribute.
   Once the node containing the 'autorestore' completes, the attribute is applied
   
    suite s
      family farchive_now
        autoarchive 0      # archive immediately after complete
        task tx
      endfamily
      family frestore_from_task
        task t1
           trigger ../farchive_now<flag>archived
           autorestore ../farchive_now  # call autorestore when t1 completes
      endfamily
    endsuite
   
   In this example task '/s/frestore_from_task/t1' is only triggered if 'farchive_now'
   is archived, then when t1 completes it will restore family 'farchive_now'
   Usage::
      --restore=/s1/f1   # restore family /s1/f1
      --restore=/s1 /s2  # restore suites /s1 and /s2
   
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
   


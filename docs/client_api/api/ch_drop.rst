
.. _ch_drop_cli:

ch_drop
///////







.. rubric:: Output of :code:`--help=ch_drop`



The following help text is generated by :code:`ecflow_client --help=ch_drop`

::

   
   ch_drop
   -------
   
   Drop/de-register the client handle.
   Un-used handle should be dropped otherwise they will stay, in the server.
      arg1 = handle(integer)  # The handle must be an integer that is > 0
   Usage:
      --ch_drop=10            # drop the client handle 10
   An error is returned if the handle had not previously been registered
   The handle stored on the local client is set to zero
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
   


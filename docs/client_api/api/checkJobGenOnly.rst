
.. _checkJobGenOnly_cli:

checkJobGenOnly
///////////////

::

   
   checkJobGenOnly
   ---------------
   
   Test hierarchical Job generation only, for chosen Node.
   The jobs are generated independent of the dependencies
   This will generate the jobs *only*, i.e. no job submission. Used for checking job generation only
     arg = node path | arg = NULL
        If no node path specified generates for all Tasks in the definition. For Test only
   
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
   

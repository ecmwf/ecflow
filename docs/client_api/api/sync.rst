
.. _sync_cli:

sync
////

::

   
   sync
   ----
   
   Incrementally synchronise the local definition with the one in the server.
   *Important* for use with c++/python interface only.
   Preference should be given to this method as only the changes are returned.
   This reduces the network bandwidth required to keep in sync with the server
   Requires a client handle, change and modify number, to get the incremental changes from server.
   The change in server state is then and merged with the client definition.
   
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
   

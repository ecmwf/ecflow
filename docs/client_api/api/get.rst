
.. _get_cli:

get
///

::

   
   get
   ---
   
   Get the suite definition or node tree in form that is re-parse able
   Get all suite node tree's from the server and write to standard out.
   The output is parse-able, and can be used to re-load the definition
     arg = NULL | arg = node path
   Usage:
     --get     # gets the definition from the server,and writes to standard out
     --get=/s1 # gets the suite from the server,and writes to standard out
   
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
   

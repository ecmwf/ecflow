
.. _job_gen_cli:

job_gen
///////

::

   
   job_gen
   -------
   
   Job submission for chosen Node *based* on dependencies.
   The server traverses the node tree every 60 seconds, and if the dependencies are free
   does job generation and submission. Sometimes the user may free time/date dependencies
   to avoid waiting for the server poll, this commands allows early job generation
     arg = node path | arg = NULL
        If no node path specified generates for full definition.
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable encrypted comms with SSL enabled server.                   |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   

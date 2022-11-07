
.. _why_cli:

why
///

::

   
   why
   ---
   
   Show the reason why a node is not running.
   Can only be used with the group command. The group command must include a 
   'get' command(i.e returns the server defs)
   The why command take a optional string argument representing a node path
   Will return reason why the node is holding and for all its children.
   If no arguments supplied will report on all nodes
     arg = node path | arg = NULL
   Usage:
     --group="get; why"               # returns why for all holding nodes
     --group="get; why=/suite/family" # returns why for a specific node
   
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
   

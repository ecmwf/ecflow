
.. _zombie_block_cli:

zombie_block
////////////

::

   
   zombie_block
   ------------
   
   Locates the task in the servers list of zombies, and sets flags to block it.
   This is default behaviour of the child commands(init,abort,complete,wait,queue)
   when the server cannot match the passwords. Each child commands will continue
   attempting to connect to the server for 24 hours, and will then return an error.
   The connection timeout can be configured with environment ECF_TIMEOUT
     args = list of task paths, at least one expected
     --zombie_block=/path/to/task  /path/to/task2
   
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
   

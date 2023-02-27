
.. _zombie_adopt_cli:

zombie_adopt
////////////

::

   
   zombie_adopt
   ------------
   
   Locates the task in the servers list of zombies, and sets to adopt.
   Next time a child command (init,event,meter,label,abort,complete,wait queue) communicates with the server, the password on the zombie is adopted by the task.
   This is only allowed if the process id matches, otherwise an error is issued.
   The zombie reference stored in the server is then deleted.
     args = list of task paths, at least one expected
     --zombie_adopt=/path/to/task  /path/to/task2
   
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
   

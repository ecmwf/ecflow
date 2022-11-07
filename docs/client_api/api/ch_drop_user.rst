
.. _ch_drop_user_cli:

ch_drop_user
////////////

::

   
   ch_drop_user
   ------------
   
   Drop/de-register all handles associated with the given user.
   If no user provided will drop for current user. Client must ensure un-used handle are dropped
   otherwise they will stay, in the server.
      arg1 = user           # The user to be drooped, if left empty drop current user 
   Usage:
      --ch_drop_user=ma0    # drop all handles associated with user ma0
      --ch_drop_user        # drop all handles associated with current user
   An error is returned if no registered handles
   To list all suites and handles use --ch_suites
   
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
   

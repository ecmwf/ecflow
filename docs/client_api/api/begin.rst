
.. _begin_cli:

begin
/////

::

   
   begin
   -----
   
   Begin playing the definition in the server.
   Expects zero or a single quoted string.
     arg1 = suite-name | Nothing | force
            play the chosen suite, if no arg specified, play all suites, in the definition
            force means reset the begin status on the suites and bypass checks.
            This is only required if suite-name is provide as the first argument
            Using force can cause the creation of zombies
   Usage:
   --begin                     # will begin all suites
   --begin="--force"         # reset and then begin all suites, bypassing any checks. Note: string must be quoted
   --begin="mySuite"         # begin playing suite of name 'mySuite'
   --begin="mySuite --force" # reset and begin playing suite 'mySuite', bypass check
   
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
   

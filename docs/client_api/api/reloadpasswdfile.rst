
.. _reloadpasswdfile_cli:

reloadpasswdfile
////////////////

::

   
   reloadpasswdfile
   ----------------
   
   Reload the server password file. To be used when ALL users have a password
   Although the password file can be reloaded(i.e to add/remove users), its location can't be changed
   The password file is located by the ECF_PASSWD environment variable, both for the client and server
   On the server the default file name is <host>.<port>.ecf.passwd
   On the client the default file name is ecf.passwd
   The format of the file is same for client and server:
   
   4.5.0
   # comment
   <user> <host> <port> <passwd> # comment
   
   i.e
   4.5.0 # the version
   fred machine1 3142 xxyyyd
   fred machine2 3133 xxyyyd # comment
   bill machine2 3133 xxyggyyd
   
   The same user may appear multiple times. i.e with different host/port. This allows the password file
   to be used for multiple servers
   For the password authentication to work. It must be:
     - Defined for the client and server
     - Creating an empty password file,(i.e with just the version) will mean, no client can reload it.
       Hence at least the server administrator needs to be added to the file
     - The password file permission's must be set for reading by the user only
   Usage:
    --reloadpasswdfile
   
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
   

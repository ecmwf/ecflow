
.. _reloadwsfile_cli:

reloadwsfile
////////////

::

   
   reloadwsfile
   ------------
   
   Reload the white list file.
   The white list file is used to authenticate 'user' commands.
   File path is specified by ECF_LISTS environment, read by the server on *startup*.
   Hence the contents of the file can be changed but not the location
   If ECF_LISTS is not specified, or is specified and is 'ecf.lists' then by default
   it will open <host>.<port>.ecf.lists.If a path like /var/tmp/ecf.lists was specified
   for ECF_LISTS, then this is the path used for reloading the white list file
   On startup, if the file is not present or is present but is empty (i.e just contains the version number)
   then all users have read/write access
   However on reload it will raises an error if file does not exist, or fails to parse
   Expected format for this file is:
   
   # comment
   4.4.14  # version number, this must be present, even if no users specified
   
   # Users with read/write access
   user1   # comment
   user2   # comment
   
   *       # use this form if you want all users to have read/write access
   
   # Users with read  access, must have - before user name
   -user3  # comment
   -user4
   
   -*      # use this form if you want all users to have read access
   
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
   

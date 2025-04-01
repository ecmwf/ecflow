
.. _delete_cli:

delete
//////

::

   
   delete
   ------
   
   Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.
     arg1 = [ force | yes ](optional)  # Use this parameter to bypass checks, i.e. for active or submitted tasks
     arg2 = yes(optional)              # Use 'yes' to bypass the confirmation prompt
     arg3 = node paths | _all_         # _all_ means delete all suites
                                       # node paths must start with a leading '/'
   Usage:
     --delete=_all_                    # Delete all suites in server. Use with care.
     --delete=/suite/f1/t1             # Delete node at /suite/f1/t1. This will prompt
     --delete=force /suite/f1/t1       # Delete node at /suite/f1/t1 even if active or submitted
     --delete=force yes /s1 /s2        # Delete suites s1,s2 even if active or submitted, bypassing prompt
   
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
   

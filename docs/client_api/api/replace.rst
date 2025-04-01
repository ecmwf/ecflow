
.. _replace_cli:

replace
///////

::

   
   replace
   -------
   
   Replaces a node in the server, with the given path
   Can also be used to add nodes in the server
     arg1 = path to node
            must exist in the client defs(arg2). This is also the node we want to
            replace in the server
     arg2 = path to client definition file
            provides the definition of the new node
     arg3 = (optional) [ parent | false ] (default = parent)
            create parent families or suite as needed, when arg1 does not
            exist in the server
     arg4 = (optional) force (default = false) 
            Force the replacement even if it causes zombies to be created
   Replace can fail if:
   - The node path(arg1) does not exist in the provided client definition(arg2)
   - The client definition(arg2) must be free of errors
   - If the third argument is not provided, then node path(arg1) must exist in the server
   - Nodes to be replaced are in active/submitted state, in which case arg4(force) can be used
   
   Replace will preserve the suspended status, if this is not required please re-queue first
   After replace is done, we check trigger expressions. These are reported to standard output.
   It is up to the user to correct invalid trigger expressions, otherwise the tasks will *not* run.
   Please note, you can use --check to check trigger expression and limits in the server.
   For more information use --help check.
   
   Usage:
     --replace=/suite/f1/t1 /tmp/client.def  parent      # Add/replace node tree /suite/f1/t1
     --replace=/suite/f1/t1 /tmp/client.def  false force # replace t1 even if its active or submitted
   
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
   

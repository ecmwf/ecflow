
.. _show_cli:

show
////

::

   
   show
   ----
   
   Used to print state of the definition returned from the server to standard output.
   This command can *only* be used in a group command, and will only work if it is
   preceded with a get command. See examples below.
      arg1 = [ defs | state | migrate ] 
   The output of show has several options: i.e
     o no arguments: With no arguments, print the definition structure to standard output
       Extern's are automatically added, allowing the output to be reloaded into the server
       i.e --group="get ; show"
     o state:
       This will output definition structure along with all the state information.
       This will include the trigger expressions, abstract syntax tree as comments.
       Excludes the edit history
     o migrate:
       This will output definition structure along with all the state information.
       The node state is shown in the comments.
       This format allows the definition to be migrated to future version of ecflow.
       The output includes edit history but excludes externs.
       When the definition is reloaded *NO* checking is done.
   
   The following shows a summary of the features associated with each choice
                           DEFS          STATE      MIGRATE
   Auto generate externs   Yes           Yes        No
   Checking on reload      Yes           Yes        No
   Edit History            No            No         Yes
   trigger AST             No            Yes        No
   
   Usage:
       --group="get ; show"
       --group="get ; show defs"    # same as the previous example
       --group="get ; show state"   # Show all state for the node tree
       --group="get ; show migrate" # Shows state and allows migration
       --group="get=/s1; show"      # show state for the node only
       --group="get=/s1; show state"
   
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
   

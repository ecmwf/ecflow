
.. _log_cli:

log
///

::

   
   log
   ---
   
   Get,clear,flush or create a new log file.
   The user must ensure that a valid path is specified.
   Specifying '--log=get' with a large number of lines from the server,
   can consume a lot of **memory**. The log file can be a very large file,
   hence we use a default of 100 lines, optionally the number of lines can be specified.
    arg1 = [ get | clear | flush | new | path ]
     get -   Outputs the log file to standard out.
             defaults to return the last 100 lines
             The second argument can specify how many lines to return
     clear - Clear the log file of its contents.
     flush - Flush and close the log file. (only temporary) next time
             server writes to log, it will be opened again. Hence it best
             to halt the server first
     new -   Flush and close the existing log file, and start using the
             the path defined for ECF_LOG. By changing this variable
             a new log file path can be used
             Alternatively an explicit path can also be provided
             in which case ECF_LOG is also updated
     path -  Returns the path name to the existing log file
    arg2 = [ new_path | optional last n lines ]
            if get specified can specify lines to get. Value must be convertible to an integer
            Otherwise if arg1 is 'new' then the second argument must be a path
   Usage:
     --log=get                        # Write the last 100 lines of the log file to standard out
     --log=get 200                    # Write the last 200 lines of the log file to standard out
     --log=clear                      # Clear the log file. The log is now empty
     --log=flush                      # Flush and close log file, next request will re-open log file
     --log=new /path/to/new/log/file  # Close and flush log file, and create a new log file, updates ECF_LOG
     --log=new                        # Close and flush log file, and create a new log file using ECF_LOG variable
   
   
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
   

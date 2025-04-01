
.. _server_load_cli:

server_load
///////////

::

   
   server_load
   -----------
   
   Generates gnuplot files that show the server load graphically.
   This is done by parsing the log file. If no log file is provided,
   then the log file path is obtained from the server. If the returned
   log file path is not accessible an error is returned
   This command produces a three files in the CWD.
       o <host>.<port>.gnuplot.dat
       o <host>.<port>.gnuplot.script
       o <host>.<port>.png
   
   The generated script can be manually changed, to see different rendering
   effects. i.e. just run 'gnuplot <host>.<port>.gnuplot.script'
   
     arg1 = <optional> path to log file
   
   If the path to log file is known, it is *preferable* to use this,
   rather than requesting the log path from the server.
   
   Usage:
      --server_load=/path/to_log_file  # Parses log and generate gnuplot files
      --server_load                    # Log file path is requested from server
                                       # which is then used to generate gnuplot files
                                       # *AVOID* if log file path is accessible
   
   Now use any png viewer to see the output i.e
   
   > display   <host>.<port>.png
   > feh       <host>.<port>.png
   > eog       <host>.<port>.png
   > xdg-open  <host>.<port>.png
   > w3m       <host>.<port>.png
   
   
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
   

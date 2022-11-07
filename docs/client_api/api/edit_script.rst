
.. _edit_script_cli:

edit_script
///////////

::

   
   edit_script
   -----------
   
   Allows user to edit, pre-process and submit the script.
   Will allow pre-processing of arbitrary file with 'pre_process_file' option
    arg1 = path to task  # The path to the task/alias
    arg2 = [ edit | pre_process | submit | pre_process_file | submit_file ]
       edit : will return the script file to standard out. The script will
              include used variables enclosed between %comment/%end at the
              start of the file
       pre_process: Will return the script file to standard out.The script will
                    include used variables enclosed between %comment/%end at the
                    start of the file and with all %include expanded
       submit: Will extract the used variables from the supplied file, i.e
               between the %comment/%end and use these them to generate the
               job using the ecf file accessible from the server
       pre_process_file: Will pre process the user supplied file.
                         Will expand includes,variable substitution,
                         remove manual & comment sections.
       submit_file: Like submit, but the supplied file, is submitted by the server
                    The last 2 options allow complete freedom to debug the script file
    arg3 = [ path_to_script_file ]
             needed for option [  pre_process_file | submit_file ]
    arg4 = create_alias (optional) default value is false, for use with 'submit_file' option
    arg5 = no_run (optional) default value is false, i.e immediately run the alias
           is no_run is specified the alias in only created
   Usage:
   --edit_script=/path/to/task edit > script_file
      server returns script with the used variables to standard out
      The user can choose to edit this file
   
   --edit_script=/path/to/task pre_process > pre_processed_script_file
     server will pre process the ecf file accessible from the server
     (i.e expand all %includes) and return the file to standard out
   
   --edit_script=/path/to/task submit script_file
     Will extract the used variables in the 'script_file' and will uses these
     variables during variable substitution of the ecf file accessible by the
     server. This is then submitted as a job
   
   --edit_script=/path/to/task pre_process_file file_to_pre_process
     The server will pre-process the user supplied file and return the contents
     to standard out. This pre-processing is the same as job file processing,
     but on a arbitrary file
   
   --edit_script=/path/to/task submit_file file_to_submit
     Will extract the used variables in the 'file_to_submit' and will uses these
     variables during variable substitution, the file is then submitted for job
     generation by the server
   
   --edit_script=/path/to/task submit_file file_to_submit create_alias
     Like the the previous example but will create and run as an alias
   
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
   


.. _status_cli:

status
//////

::

   
   status
   ------
   
   Shows the status of a job associated with a task, in %ECF_JOB%.stat file
   If a family or suite is selected, will invoke status command hierarchically.
   Status uses the ECF_STATUS_CMD variable. After variable substitution it is invoked as a command.
   The command should be written in such a way that the output is written to %ECF_JOB%.stat
   This will allow the output of status command to be shown by the --file command
   i.e /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1::
   If the process id cannot be found on the remote system, then the status command can also
   arrange for the task to be aborted
   The status command can fail for the following reasons:
    - ECF_STATUS_CMD not found
    - variable substitution fails
    - state is active but it can't find process id, i.e. ECF_RID
    - the status command does not exit cleanly
   When this happens a flag is set, STATUSCMD_FAILED, which is visible in the GUI
   Usage::
      --status=/s1/f1/t1     # ECF_STATUS_CMD should output to %ECF_JOB%.stat
      --file=/s1/f1/t1 stat  # Return contents of %ECF_JOB%.stat file
   
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
   

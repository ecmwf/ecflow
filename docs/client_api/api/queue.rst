
.. _queue_cli:

queue
/////

::

   
   queue
   -----
   
   QueueCmd. For use in the '.ecf' script file *only*
   Hence the context is supplied via environment variables
     arg1(string) = queue-name:
     arg2(string) = action: [active | aborted | complete | no_of_aborted | reset ]
        active: returns the first queued/aborted step, the return string is the queue  value from the definition
        no_of_aborted: returns number of aborted steps as a string, i.e 10
        reset: sets the index to the first queued/aborted step. Allows steps to be reprocessed for errors
     arg2(string) = step:  value returned from. step=$(ecflow_client --queue=queue_name active)
                   This is only valid for complete and aborted steps
     arg4(string) = path: (optional). The path where the queue is defined.
                    By default we search for the queue up the node tree.
   
   If this child command is a zombie, then the default action will be to *block*,
   The default can be overridden by using zombie attributes.If the path to the queue is not defined, then this command will
   search for the queue up the node hierarchy. If no queue found, command fails
   
   Usage:
   step=""
   QNAME="my_queue_name"
   while [1 == 1 ] ; do
      # this return the first queued/aborted step, then increments to next step, return <NULL> when all steps processed
      step=$(ecflow_client --queue=$QNAME active) # of the form string  i.e "003". this step is now active
      if [[ $step == "<NULL>" ]] ; then
           break;
      fi
      ...
      ecflow_client --queue=$QNAME complete $step   # tell ecflow this step completed
   done
   
   trap() { ecflow_client --queue=$QNAME aborted $step # tell ecflow this step failed }
   
   
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
   
   The following environment variables are specific to child commands.
   The scripts should export the mandatory variables. Typically defined in the head/tail includes files
   
   |--------------|----------|-----------|---------------------------------------------------------------|
   | Name         |  Type    | Required  | Description                                                   |
   |--------------|----------|-----------|---------------------------------------------------------------|
   | ECF_NAME     | <string> | Mandatory | Full path name to the task                                    |
   | ECF_PASS     | <string> | Mandatory | The jobs password, allocated by server, then used by server to|
   |              |          |           | authenticate client request                                   |
   | ECF_TRYNO    |  <int>   | Mandatory | The number of times the job has run. This is allocated by the |
   |              |          |           | server, and used in job/output file name generation.          |
   | ECF_RID      | <string> | Mandatory | The process identifier. Helps zombies identification and      |
   |              |          |           | automated killing of running jobs                             |
   | ECF_TIMEOUT  |  <int>   | optional  | Max time in *seconds* for client to deliver message to main   |
   |              |          |           | server. The default is 24 hours                               |
   | ECF_HOSTFILE | <string> | optional  | File that lists alternate hosts to try, if connection to main |
   |              |          |           | host fails                                                    |
   | ECF_DENIED   |  <any>   | optional  | Provides a way for child to exit with an error, if server     |
   |              |          |           | denies connection. Avoids 24hr wait. Note: when you have      |
   |              |          |           | hundreds of tasks, using this approach requires a lot of      |
   |              |          |           | manual intervention to determine job status                   |
   | NO_ECF       |  <any>   | optional  | If set exit's ecflow_client immediately with success. This    |
   |              |          |           | allows the scripts to be tested independent of the server     |
   |--------------|----------|-----------|---------------------------------------------------------------|
   

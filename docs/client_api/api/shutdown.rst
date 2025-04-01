
.. _shutdown_cli:

shutdown
////////

::

   
   shutdown
   --------
   
   Stop server from scheduling new jobs.
     arg1 = yes(optional) # use to bypass confirmation prompt,i.e
     --shutdown=yes
   The following table shows server behaviour in the different states.
   |----------------------------------------------------------------------------------|
   | Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |
   |--------------|--------------|--------------|---------------|---------------------|
   |     RUNNING  |    yes       |      yes     |      yes      |      yes            |
   |    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |
   |      HALTED  |    yes       |      no      |      no       |      no             |
   |--------------|--------------|--------------|---------------|---------------------|
   
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
   

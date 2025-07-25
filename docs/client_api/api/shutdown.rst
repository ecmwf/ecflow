
.. _shutdown_cli:

shutdown
////////







.. rubric:: Output of :code:`--help=shutdown`



The following help text is generated by :code:`ecflow_client --help=shutdown`

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
   
   The client considers, for both user and child commands, the following environment variables:
   
     ECF_HOST <string> [mandatory*]
       The main server hostname; default value is 'localhost'
     ECF_PORT <int> [mandatory*]
       The main server port; default value is '3141'
     ECF_SSL <any> [optional*]
       Enable secure communication between client and server.
     ECF_HOSTFILE <string> [optional]
       File that lists alternate hosts to try, if connection to main host fails
     ECF_HOSTFILE_POLICY <string> [optional]
       The policy ('task' or 'all') to define which commands consider using alternate hosts.
   
   The options marked with (*) must be specified in order for the client to communicate
   with the server, either by setting the environment variables or by specifying the
   command line options.
   


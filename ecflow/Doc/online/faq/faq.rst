
.. index::
   single: faq
   
.. _faq:
   
=======
**FAQ**
=======
 
* How do I check a :term:`suite definition` without loading it into the :term:`ecflow_server` ?

  * If the :term:`suite definition` is in a text file::
  
      > ecflow_client --load=test.def check_only
  
  * If the :term:`suite definition` is built using :ref:`suite_definition_python_api`::
  
      import ecflow
      
      defs = ecflow.Defs()
      suite = defs.add_suite("s1");
      suite.add_task("t1").add_trigger("t2 == active)")   
      theCheckValue = defs.check();
      print "Message: '" + theCheckValue + "'"
      assert len(theCheckValue) != 0,  "Expected Error: mis-matched brackets in expression."
  
* I have loaded my :term:`suite definition` but I cannot see it on :term:`ecflowview`?
   | If you can see the :term:`suite` using the suites or get command in CLI::
   
   ::
   
         > ecflow_client --suites   # list suites, this is faster than using --get
         > ecflow_client --get       
   
   
   | but not in :term:`ecflowview` you should right click on the “server” button, select “suites” 
   | and select your suite.
  
* My task fails when I submit it and I do not see any job output?
   | Generally it is always a good idea to look at the output of the :term:`ecflow_server` log directly 
   | or in :term:`ecflowview` by right clicking on the server and selecting “History”.
   | Also look at whether a job is created. Also check that server is not :term:`halted`.
   | If you click on the script button you get a “file read error” message then :term:`ecflow_server` cannot 
   | find your script. 
   | Check the file permissions or location given in variable ECF_SCRIPT (using variables 
   | tag in :term:`ecflowview`).  This is normally based on the ECF_FILES variable
   | If you right click on the node and click the edit button and get the error message
   | “send failed for the node” you may not be able to access your include files.  
   | Check the location of the include files and how they are defined 
   | (see ecFlow Pre-processor symbols ).
  
* My task stays in a :term:`submitted` state when I submit a job?
   | This could be caused by the job being unable to submit because the queuing system 
   | used cannot schedule the task at the time or because the task is failing before 
   | the :term:`child command`
   
   ::
  
      > ecflow_client --init   # command is sent
   
   | Check that the :term:`ecflow_server` is not :term:`halted`
   | Test, running the job from the command line or check the status in the queuing 
   | system used, e.g. llq for loadleveller, qstat for PBS etc.
   | Run the submission command from the command line.  
   | This is based on how the ECF_JOB_CMD variable is set. 
   | The script could be failing before the ecflow_client --init  command is sent.
   
   | If you are using ECF_OUT to define the output directory. Then make
   | sure that the directory exists, including the directories corresponding
   | to suite/family nodes.
   
* My task stays in a :term:`active` state when I submit a job?
   | This can be caused if the job is unable to send an :term:`ecflow_client` --complete 
   | :term:`child command` to the :term:`ecflow_server`.  
   | This can be caused by an error that is not trapped, such as the job being 
   | killed with a -9 option or the host system crashing. 
   | Check that the job is running and if not rerun or syncronise the 
   | task :term:`status` in ecFlow as appropriate.
  
* How can I check the status of an :term:`ecflow_server`?
   | Invoke
   
   ::

      > ecflow_client --stats   
   
   | This will display some standard information regarding the :term:`ecflow_server` including 
   | the version number,node information, status, security information, usage, load,
   | setup and up time.
  
* How can I check the load on the :term:`ecflow_server`?
   Invoke::
  
      > ecflow_client --server_load   
      
   | This relies on gnuplot. If you know the location of your log path and it’s accessible, 
   | to avoid overloading server call
   
   ::
  
      > ecflow_client --server_load=/path/to/log/file

* Jobs run locally remain submitted when the variable ECF_OUT is used?
   | ECF_OUT variable should be used in situations where job output, is not 
   | located in the same directory as job files. This is necessary for remote job submission
   | when local and remote hosts do not share a common file system.

   | By using ECF_OUT, the user is then responsible to create the directory
   | structure (all directories) in *advance*, so that output files can be created. 

   | It is enough to copy-paste the directory path for the variable ECF_JOBOUT, and use
   | it to execute the command mkdir -p on the remote host.

   | ecFlow server is "target agnostic" and does not know on its own how to log
   | appropriately on the remote machine ; it keeps the suite designer responsible for 
   | directory creation.

* A message 'locale::facet::_S_create_c_locale name not valid' is displayed, when the
   | ecflow_server or ecflow_client command are run. How to prevent it ?
  
   | To see the list of locale's on your system use 
   
   ::
  
    > locale -a
   
   | Then set the LANG environment variable. i.e.
   | LANG=en_GB.UTF-8 (ksh: export LANG=en_GB.UTF-8)
 
* How can I logically or time dependencies of different types ?
 
   | It is important to understand how time :term:`dependencies` work first.
   | When we have multiple time :term:`dependencies` of the same type, they are 'or'ed. i.e
   |    time 10.00
   |    time 12:00             # Task will run when time is 10:00 OR 12:00
   | 
   | Likewise if we have:
   |    date 1.12.2012
   |    date 2.12.2012         # Task is free to run only on first or second of December.
   | 
   | When *different* types of time :term:`dependencies` are added, then the task is only 
   | free to run when both are satisfied:
   |    time 10:00
   |    date 1.12.2015         # task is only free to run at 10 am on the first of December
   | This effectively means that time :term:`dependencies` are logically 'and'ed.
   |
   | Now suppose we wanted to run the task, at 10.00  *OR* when the date is 1.12.2015.
   | This can be done by adding a dummy task.
   |
   |    task dummy_time_trigger
   |       edit ECF_DUMMY_TASK ""   # Tell server & checking not to expect .ecf file
   |       time 10:00
   |    task dummy_date_trigger
   |       edit ECF_DUMMY_TASK ""   # Tell server & checking not to expect .ecf file
   |       date 1.12.2015
   |    task time_or_date
   |       trigger dummy_time_trigger == complete or dummy_date_trigger == complete
   |
   | By using a combination of a dummy task and trigger, we can achieve the effect
   | of 'OR' ing time :term:`dependencies` of different types. This technique will work
   | for any complex dependency and has the added advantage of allowing us to 
   | manually free the :term:`dependencies` via the GUI.
   
* In the python API what's the difference between sync_local(),get_server_defs(), get_defs() ?

  | First it is important to understand that the 'ecflow.Client' class **stores**
  | the suite definition returned from the server. The suite definition can be retrieved
  | from the server using 'sync_local()' or 'get_server_defs()'
  | While 'ecflow.Client' exists the suite definition is retained.
  
  **ecflow.Client.get_defs()**
  
  | Returns the defs stored on the client. Hence either sync_local() or get_server_defs() should
  | be called first, otherwise a Null object is returned.
  
  **ecflow.Client.sync_local()**
  
  #. The very *first* call always retrieves the *full* suite definition
  #. The second and subsequent calls *may* return delta/incremental *or* less typically the full suite definition.
     If there there only event, meter,label and state changes in the server, then calling
     sync_local(), will retrieve these *small* incremental changes and syncronise them with the
     suite definition held in the ecflow.Client() object. Typically these changes are a very
     small fraction, when compared with the full suite. This is the normal scenario.
     The incremental sync reduces the network bandwidth and hence improves speed.
     
     If however the user make large scale changes, i.e by deleting or adding nodes, then
     sync_local() will return the full suite definition.
     
     Hence if your python code needs to continually poll the server, please use the
     same ecflow.Client() object and *always* use sync_local().

   ::
   
     try:
         ci = Client()                       # use default host(ECF_NODE) & port(ECF_PORT)
         ci.sync_local()                     # Very first call gets the full Defs
         client_defs = ci.get_defs()         # End user access to the returned Defs
             ... after a period of time
         ci.sync_local()                     # Subsequent calls retrieve incremental or full suite, but typically incremental
         if ci.in_sync():                    # returns true if server changed and changes applied to client
            print 'Client is now in sync with server'
         client_defs = ci.get_defs()         # End user access to the returned Defs
      except RuntimeError, e:
         print str(e)
       
   
  **ecflow.Client.get_server_defs()**
   
   | This *always* returns the full suite definition. For single use of suite
   | definition in the python code there is no difference between sync_local()
   | and get_server_defs(). *HOWEVER* if you wish to monitor the server 
   | in python then you *MUST* uses sync_local() as it will be considerably faster.
   
   ::
   
      try:
         ci = Client()         # use default host(ECF_NODE) & port(ECF_PORT)
         ci.get_server_defs()  # retrieve definition from the server and store on 'ci'
         print ci.get_defs()   # print out definition stored in the client
       except RuntimeError, e:
         print str(e) 
   
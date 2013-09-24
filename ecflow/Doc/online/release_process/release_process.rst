
.. index::
   single: release_process
   
.. _release_process:
   
===================
**Release process**
===================

This page documents the release process for ecFlow.

* All new releases made to /usr/local/apps/ecflow are *candidate/test*
  releases until they are made official.
  
  This is required because:
    * Allows user support to test build ecflow on all the platforms used by the member states.
    * Allow the module functionality to be setup for AIX
    * Enable user support to validate/test any customer related issues
    * Allow testing
    * Allow general feedback.
    
  Before a release is made it will be announced in advance.  
  The test release is accessed by using::
  
       export ECFLOW_VERSION=test
       use ecflow
       
  On AIX(power6/power7) the test release can be accessed using 
  the module functionality::
  
      module avail ecflow
      module unload ecflow 
      module load ecflow/3.0.1
       
* The latest official release can be accessed by using::

      use ecflow
      
  and on AIX(power6/power7)::

      module avail ecflow
      module unload ecflow 
      module load ecflow/3.0.1
   
* ecFlow is used in scenario's where the server will live for
   | several months/years, hence it is *highly* recommended that 
   | users specify the version of the software they wish to use. 
  
   | This protects the user from changes to the latest release
   | and allows them to decide when to migrate to a newer release.
   | It also allows the developers to make new release at any time.
   | (any other solution including use of test release, means that the
   | current release can never be updated until everyone is ready
   | to move).
  
  To use a specific version::
  
     export ECFLOW_VERSION=3.0.1
     use ecflow
 
  | Additionally references to ecflow_client in the scripts(.ecf) should use
  | the server variable %ECF_VERSION% to locate the executable, this ensures
  | that client version is in sync with the server
  
  ::

     /usr/local/apps/ecflow/%ECF_VERSION%/bin/ecflow_client --complete
   
* If issues get raised with test release, then another release is made. 
   | This is announced each time it is required.
   | Individual test release candidates can be accessed using.
   
   ::
   
     export ECFLOW_VERSION=3.0.0rc2  # i.e release candidate 2
     use ecflow   
  
* If no further issues are raised, the release is made official.
   | In this case
   
   ::
  
      use ecflow
      
   | will now point to the newly released version.
   | A email will be sent to notify users.
   | This release will then be uploaded on the web(Confluence) for all users.
     
 
Version numbering scheme
========================

Ecflow version number has 3 numbers, i.e 2.0.30

  <release-number><major><minor>
   
* If a release is made where the only the minor number is changed.
   | 3.0.0--> 3.0.1
   | Then the client/server are compatible and can be intermixed.
     
* If a release is made where the only the major number is changed.
   | 3.0.0--> 3.1.0
   | Then the user commands have changed, the child commands remain
   | unchanged. i.e old child commands can talk to the newer server,
   | check point files can still be used between the releases.
     
* If the release number is changed.
   | Both user and child command have changed, and check point files
   | are not compatible. This requires migration.
   
As ecFlow achieves greater maturity I expect most of the changes
will be to the minor number. 


Migration
=========
At the simplest migration involves running::

   ecflow_client --migrate > migrate.def  # run on old server
   
| migrate.def is like a normal definition file where the
| state is encoded as comments.
   
| Then run the following with the new client/server

::

   ecflow_client --load migrate.def      # run on new server
   
| This will load the file into the new server preserving 
| all state information.

.. warning::
   
   The --migrate functionality is only available from release 3.0.0.

| If the backup servers functionality is used, then then backup servers
| should also be migrated at the same time.
   
| The following notes provides more detail guidance on the migration process.
| This assumes you are migrating half way through running some experiments,
| and want to continue where you left. This can be done in 3 steps,
| where step 1 and 3 can be automated.

STEP 1:  
   * first shutdown the old server
   * suspend all the suites
   * record all the currently aborted tasks, via a python script & set them to unknown state,
   * kill all the active tasks, and wait for them to abort.
   * halt the server
   * Used --migrate to dump the state & structure to a file.
   * terminate the server

STEP 2: manual step
   * move the check point file, to stop server from loading them
   * start the new server using the same port number

STEP 3: 
   * Load the migration file.
   * restart the server, and resume all suspended suites
   * Re-queue all the aborted tasks
   * re-set tasks that were previously aborted, back to abort state

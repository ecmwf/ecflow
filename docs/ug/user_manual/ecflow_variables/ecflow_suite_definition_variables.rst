.. _ecflow_suite_definition_variables:

Suite definition variables
/////////////////////////////////

The suite definition variables are created like:

.. code-block:: shell

    edit VAR 'the name of the variable'                                

https://confluence.ecmwf.int/display/ECFLOW/Adding+Variables can also be created via the :ref:`python_api`.

Any user-created variable takes precedence over the suite definition
variable of the same name.

These suite definition variables control the execution of ECF.
Defining these variables you can, for example, control how a job is
run, how ecFlow files are located, or where the job output should go. 

The table below shows a list of ecFlow variables.


.. list-table::
   :header-rows: 1
   :widths: 8 57 5 30
 
   * - Variable name
     - Explanation
     - Default
     - Example
   * - ECF_URLCMD
     - Command to be executed to allow the user to view related web pages
     - No
     - .. code-block:: shell
          
          ${BROWSER:=firefox} -remote 'openURL(%ECF_URLBASE%/%ECF_URL%)'
          
       Where ECF_URLBASE is the base web address and ECF_URL the specific page.
   * - ECF_TRIES
     - The number of times a job should rerun if it aborts. If more than one and the job aborts, the job is automatically re-run by ECF. Useful when jobs are run in unreliable environments.
       
       For example, using commands like ftp(1) in a job can fail easily, but re-running the job will often work. 
     - Yes
     - 2
   * - ECF_STATUS_CMD
     - Command to be used to check the status of a submitted or running job. Can use the generated variable %ECF_JOB%.stat for storing command output
     - No
     - .. code-block:: shell
        
         'rsh %SCHOST% qstat -f %ECF_RID%.%SCHOST% %ECF_JOB% 2>&1' "ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% bash -c 'ps -elf %ECF_RID% | grep \" %USER% \"' >>%ECF_JOB%.stat"

   * - ECF_OUT
     - Alternate location for job and cmd output files. If this variable exists it is used as a base for ECF_JOBOUT but it is also used to search for the output by ecFlow when asked by ecflow_ui/CLI. 
     
       If the output is in ECF_HOME/ECF_NODE.ECF_TRYNO it is returned, otherwise ECF_OUT/ECF_NODE.ECF_TRYNO that is ECF_JOBOUT is used. 
       
       The job may continue to use ECF_JOBOUT (as in a QSUB directive) but should copy its own output file back into ECF_HOME/ECF_NODE.ECF_TRYNO at the end of their run.
     - No
     - /scratch/ECF/
   * - ECF_MICRO
     - ecFlow pre-process character to be used by ecFlow pre-processor for variable substitution and including files.
     - Yes
     - %
   * - ECF_KILL_CMD
     - Method to kill a running task. Depends on how the task was submitted via ECF_JOB_CMD. ecFlow must know the value of remote-id (ECF_RID). Variable enables kill(CLI) command to be used. Can use the generated variable %ECF_JOB%.kill for storing command output
     - No
     - .. code-block:: shell 
          
          rsh %SCHOST% qdel -2 %ECF_RID% > %ECF_JOB% 2>&1 "ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% kill -9 %ECF_RID%"

   * - ECF_JOB_CMD
     - Command to be executed to submit a job. May involve using a queuing system, like NQS, or may run the job in the background.
     - Yes
     - .. code-block:: shell 
         
          %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 "mkdir -p $(dirname %ECF_JOBOUT%) && ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% ksh -s <%ECF_JOB% >%ECF_JOBOUT% 2>&1 &"

   * - ECF_INCLUDE
     - Path for the include files.
     - No
     - /home/user/ECF/$SUITE/include
   * - ECF_HOME
     - The default location for ecFlow files if ECF_FILES is not used.
     - Yes
     - /tmp/ECF/$SUITE
   * - ECF_FILES
     - Alternate location for ecFlow files
     - No
     - /home/user/ECF/$SUITE
   * - ECF_EXTN
     - Overrides the default script extension
     - Yes
     - .sms (default is .ecf)
   * - ECF_DUMMY_TASK
     - Some tasks have no associated '.ecf' file. The addition of this variable stops job generation checking from raising errors.
     - No
     - Any value is sufficient
   * - ???
     - The location for generated files. These are the job-files and the job-output. Setting this variable to a different directory to ECF_FILES enables you to clean up all the files produced by running ECF.
     - ???
     - .. code-block:: shell  
        
         %SCHOST%submit %ECF_JOB%
 

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
   * - :code:`ECF_DUMMY_TASK`
     - Some tasks have no associated '.ecf' file. Setting this variable disables job generation checking, thus avoiding related errors.
     - No
     - Any value is sufficient
   * - :code:`ECF_EXTN`
     - Setting this variable allows the customisation of the default task script extension.
       If not set, the default extension is '.ecf'.
     - Yes
     - :code:`.sms`
   * - :code:`ECF_FILES`
     - This directory path serves as anchor for locating task scripts. It is used by ecFlow only to find the task scripts.

       This variable can be defined, in conjunction with :code:`ECF_HOME`, and this allows separating the generated files from the task scripts.
     - No
     - /path/to/workspace/$SUITE/scripts
   * - :code:`ECF_HOME`
     - This directory path serves as anchor for locating all ecFlow files, both while finding task scripts and storing generated ecFlow files (such as the job files and the job output).

       In order to customise task script location, consider overriding :code:`ECF_FILES`, as this allows separating the task scripts from the files generated during workflow execution.
     - Yes
     - /path/to/workspace/$SUITE
   * - :code:`ECF_INCLUDE`
     - This directory path is used by ecFlow to find included files.
     - No
     - /path/to/workspace/$SUITE/include
   * - :code:`ECF_JOB_CMD`
     - The command executed to submit a job.

       The command itself is generic and, in practice, may involve using a queuing system (e.g Slurm) or running the job in the background.
     - Yes
     - .. code-block:: shell

          # Example 1: Submit the job locally, running it in background
          %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &

          # Example 2: Job submitted as a remote process over an ssh connection
          ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% ksh -s < %ECF_JOB% > %ECF_JOBOUT% 2>&1 &

          # Example 3: Job submitted to a queuing system
          %SCHOST% submit %ECF_JOB%

   * - :code:`ECF_KILL_CMD`
     - The command executed to kill a running job.

       In practice, this command is highly linked with how the task was submitted via :code:`ECF_JOB_CMD`.

       The command definition can use the value of job remote-id (i.e. :code:`ECF_RID`),
       and can leverage on the Unix command :code:`kill` to kill a local job,
       or perform remote operations in the cases where the tasks are submitted to a remote host.

       Consider defining this command such that it stores the kill command output
       (e.g. by redirecting the output to a file, such as :code:`%ECF_JOB%.kill`)
     - No
     - .. code-block:: shell

          # Example 1: Kill a local jobq
          kill -9 %ECF_RID%

          # Example 2: Kill a remote job over an ssh connection
          ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% kill -9 %ECF_RID%

          # Example 3: Kill a job submitted to a queuing system
          rsh %SCHOST% qdel -2 %ECF_RID% > %ECF_JOB% 2>&1

   * - :code:`ECF_MICRO`
     - The ecFlow pre-processor character, is used when generating job scripts
       (i.e. when pre-processing a '.ecf' file + include files, and performing variable substitution).

       The default value is the percent sign (:code:`%`), but it can be set to any character.
     - Yes
     - :code:`%`
   * - :code:`ECF_OUT`
     - Alternate location for job and cmd output files. If this variable exists it is used as a base for ECF_JOBOUT but it is also used to search for the output by ecFlow when asked by ecflow_ui/CLI.

       If the output is in ECF_HOME/ECF_NODE.ECF_TRYNO it is returned, otherwise ECF_OUT/ECF_NODE.ECF_TRYNO that is ECF_JOBOUT is used.

       The job may continue to use ECF_JOBOUT (as in a QSUB directive) but should copy its own output file back into ECF_HOME/ECF_NODE.ECF_TRYNO at the end of their run.
     - No
     - /scratch/ECF/
   * - :code:`ECF_STATUS_CMD`
     - The command executed to retrieve the status of a submitted or running job.

       Consider defining this command such that it stores the status command output
       (e.g. by redirecting the output to a file, such as :code:`%ECF_JOB%.stat`)
     - No
     - .. code-block:: shell

         'rsh %SCHOST% qstat -f %ECF_RID%.%SCHOST% %ECF_JOB% 2>&1' "ssh -v -o StrictHostKeyChecking=no %USER%@%REMOTE_HOST% bash -c 'ps -elf %ECF_RID% | grep \" %USER% \"' >>%ECF_JOB%.stat"

   * - :code:`ECF_TRIES`
     - The number of times a job should automatically rerun if it aborts.

       When this variable is set to a value higher than 1, any job that terminates by aborting will be automatically
       re-run by ECF, until the number of retries reaches the configured value.

       This variable is useful when jobs execute in unreliable environments or, perhaps, run network dependent commands
       (e.g. upload data over FTP).
     - Yes
     - 2
   * - :code:`ECF_URL_CMD`
     - The command executed to view related web pages.
     - No
     - .. code-block:: shell

          ${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'

       Where ECF_URL_BASE is the base web address and ECF_URL the specific page.
       Notice that `ECF_URL_BASE` and `ECF_URL` themselves are not suite definition variables, but can be defined in the suite definition file.

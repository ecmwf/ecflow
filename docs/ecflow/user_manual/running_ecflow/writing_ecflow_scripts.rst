.. _writing_ecflow_scripts:

Writing ecFlow scripts
//////////////////////


The **ecFlow** **script** describing the **task t1**, as defined in
the previous section, refers to a '.ecf' file. This is similar to a
UNIX shell script. The differences, however, include the addition of
"C" like pre-processing directives and ecFlow variables.

By default, the ecFlow pre-processing directives are specified using
the % character. A simple example task file is given below.

.. code-block:: shell

    %include <head.h>                                                                                                                   
    echo "I am testing a ecFlow script in %ECF_HOME%"                                                                                      
    %include <tail.h>                                                  

Before submitting the task, ecFlow will parse the script for ecFlow
directives and substitute relevant strings.

In the example above the %include command will be substituted with the
content of the file head.h and **%ECF_HOME%** will be substituted with
the ecFlow variable **ECF_HOME**

ecFlow scripts communicate with ecFlow server via child commands, the
head.h file can be used to send relevant child commands to inform
ecFlow of the job status, set up error trapping (ecflow_client
--abort) and define variables relating to the job environment. The
tail.h file can contain related child commands (ecflow_client
--complete) and information on how to clean up after the task.

Operationally at ECMWF we also include a number of header files that
also setup relevant information for job scheduling via external
queuing systems (such as loadleveler on IBM systems) and suite
configuration.

Guidelines when Writing operational scripts
===========================================

The following are a few guidelines we use when writing operational
scripts:

-  All operational tasks should be rerunnable, or at the very least
   include instructions in the **manual page** on how to restart the
   task (e.g. by running another task first).

-  Tasks should be able to run independently of the server (again the
   manual page should have instructions on how to restart on a different
   server).

-  The critical parts of a suite should be independent as far as
   possible from the less critical parts. For instance, ECMWF keeps its
   operational archiving tasks in a separate family from the
   time-critical tasks.

-  You should be consistent in your use of scripts.

-  You should turn on error failing so ecFlow can trap failures, e.g.
   using set -e and trap in ksh.

-  You should not use UNIX aliases.

-  You should not use shell functions, as these can cause problems
   trapping errors. (or explicitly repeat the trapping to ensure
   portability)

-  Exported variables should be UPPERCASE.

-  Defined ecFlow variables should not start with generated ECF\_ to
   avoid confusion.

-  All variables should be set (using default values if necessary).

-  Try to avoid using NFS mounted file systems in the critical path.

-  Use files or file-systems owned by one single "operational" user

-  Always clean up. i.e. Job output, server logs, job scripts.

-  Tasks should run in a reasonable time.

-  Keep the ecFlow scripts manual page up to date, including details of
   how to handle failures and who is responsible for the script.

-  Keep your task output - at the end of each day, we keep the output of
   all tasks, tar them up and store them on tape. This allows us to
   review suites at a later date and is useful in indicating when
   problems may have started.

-  Avoid duplication of scripts. Scripts can easily be made configurable
   and shareable.

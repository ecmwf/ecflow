.. _debugging_ecflow_scripts:

Debugging ecFlow scripts
////////////////////////

When debugging scripts you need to consider the process used to submit
the job. First, a script is accessed and expanded to create the job
file. This job file is then submitted to the relevant system where it is
run, possibly via a queuing system. Errors can occur at any of these
steps.

Location of ecFlow script
=========================

The first thing to check is whether the job file can be created. In the
GUI you should be able to view your script. If not, you should receive a
pop-up window indicating a file read error. This indicates that ecFlow
cannot find your script as either it does not exist or ecFlow cannot
find it. Look at the value of the **ECF_SCRIPT** variable to see where
ecFlow expects to find the file. The location can be modified using the
variable **ECF_FILES** as described later.

Creation of ecFlow job file
===========================

The next thing ecFlow will do is create the job file by adding all
include files and substituting ecFlow variables (see the next chapter on
"The ecFlow Pre-processor" for more details). To test if ecFlow can find
all the include files and variables click the edit tab in ecflow_ui,
info panel. If you get the error message "send failed for the node"
ecFlow may not be able to access the include files or some ecFlow
variables have not been set. More details will be given in the ecFlow
log files.

Using Python API to check, Location of ecFlow scripts and Creation of job file
==============================================================================

Both of the above steps can be done using ecFlow python API. This should
be done on the server-side, where the scripts are accessible. For more
details see: `Checking Job
Creation <https://confluence.ecmwf.int/display/ECFLOW/Checking+Job+Generation>`__

.. code-block:: python
    :caption: Check job creation

    import os
    from ecflow import Defs,Suite,Task,Edit
        
    home = os.path.join(os.getenv("HOME"),  "course")
    defs = Defs(
            Suite('test',
                Edit(ECF_HOME=home),
                Task('t1')))
    
    print("Checking job creation: .ecf -> .job0")
    print(defs.check_job_creation())


Submission of ecflow job file
=============================

The next stage is to submit the ecFlow job file. The best way to debug
this is to try the submission of the job file on the command line as
described by your ECF_JOB_CMD variable. This will usually show up any
problems in the job submission process. The script we use to submit our
job files also makes visible the job submission output in the ECF_JOBOUT
directory.

Job file debugging
==================

You can debug your job file stand-alone, by running it outside of
ecflow. To bypass the ecflow_client command in the job file, set the
environment variable NO_ECF.

.. code-block:: shell

    export NO_ECF=1 # this will cause ecflow_client to always return success.
    sh -x .....task.job1
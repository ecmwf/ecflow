.. _running_tasks_on_remote_systems:

Running tasks on remote systems
///////////////////////////////

To start a job, ecFlow uses the content of the :term:`ECF_JOB_CMD` variable.
By modifying this variable, it is possible to control where and how the
job will run. The command should use the ecFlow variables :term:`ECF_JOB`
and :term:`ECF_JOBOUT`. :term:`ECF_JOB` contains the name of the job file and
:term:`ECF_JOBOUT` contains the name of the file that should contain the
output. The default value of :term:`ECF_JOB_CMD` runs the job using a
**bash** login shell with the following command:

.. code-block:: shell

    %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &

To run the tasks on a remote machine consider using the UNIX command **ssh**,
and defining an ecFlow variable called :code:`HOST`. Assuming that the job files are
visible on ecFlow and remote hosts (e.g. by mounting the same file-system on both hosts),
the :term:`ECF_JOB_CMD` can be defined as follows:

.. code-block:: shell

    edit ECF_JOB_CMD "ssh %HOST% '%ECF_JOB% > %ECF_JOBOUT% 2>&1 &'"    

ecFlow uses standard UNIX permissions, so ensure that :code:`$HOME/.ssh` contents contain
the right settings. Access to this location is needed to allow ecFlow to successfully run
the **ssh** command.

.. note::

    The examples in this section assume the use of **bash**, but other login shells can be used
    with the necessary adaptations. For example, if your login shell is **csh**, you can define
    :term:`ECF_JOB_CMD` as

    .. code-block:: shell

        edit ECF_JOB_CMD "ssh %HOST% '%ECF_JOB% >& %ECF_JOBOUT%'"

:term:`ECF_JOB_CMD` can also submit tasks directly to a queuing system on the target machine.
For example, at ECMWF, the UNIX :code:`ecf_submit` script is used to submit tasks to multiple
systems and multiple queuing systems, as follows:

.. code-block:: shell

    edit ECF_JOB_CMD "ecf_submit %USER% %SCHOST% %ECF_JOB%             
    %ECF_JOBOUT%"                                                      

Alongside this, the task 'ecf' scripts include a generic script
header that contains typical queuing commands (such as wall clock time and
priority), e.g. contents of sample :code:`qsub.h`:

.. code-block:: shell

    # QSUB -q %QUEUE%                                                                                                            
    # QSUB -u %USER%                                                                                                                     
    # QSUB -s /bin/ksh                                                                                                                       
    # QSUB -r %TASK%_%FAMILY1:NOT_DEF%                                                                                                       
    # QSUB -o %LOGDIR%%ECF_NAME%.%ECF_TRYNO%                                                                                                
    # QSUB -lh %THREADS:1%                                             

The :code:`ecf_submit` script can replace these generic queuing commands with
the relevant commands for the host to which the task is submitted and
submit the task-relevant way, e.g. for a PBS system it replaces the QSUB
commands with the equivalent PBS commands.

Similarly to running a task remotely, to kill a task remotely you need
to either send a signal 2 (i.e. :code:`kill -2`) to the task or issue the relevant
queuing system command. Again we have included all this information into
a script called :code:`ecf_kill` that issues the correct command depending on
the host. Another example scripts are :code:`ecf_status` (which shows the status of
tasks) and :code:`ecfurl` (which opens a web link for a task) are included in the
latest releases of ecFlow.

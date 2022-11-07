.. _running_tasks_on_remote_systems:

Running tasks on remote systems
///////////////////////////////

To start a job, ecFlow uses the content of the **ECF_JOB_CMD** variable.
By modifying this variable, it is possible to control where and how the
job will run. The command should use the ecFlow variables **ECF_JOB**
and **ECF_JOBOUT**. **ECF_JOB** contains the name of the job file and
**ECF_JOBOUT** contains the name of the file that should contain the
output. For a ksh or bash UNIX script the default command is:

.. code-block:: shell

    %ECF_JOB_CMD% 1> %ECF_JOBOUT% 2>&1 &                               

To run the tasks on a remote machine you can use the UNIX command
**rsh** (or **ssh**). We would like the name of the host to be defined
by an ecFlow variable called **HOST**. We assume that all the files are
visible on all the hosts, using NFS. You can then redefine the
ECF_JOB_CMD as follows for **ksh**:

.. code-block:: shell

    edit ECF_JOB_CMD "ssh %HOST% '%ECF_JOB% > %ECF_JOBOUT% 2>&1 &'"    

As ecFlow makes use of standard UNIX permissions you may experience problems using **ssh**. Make sure that the file $HOME/.ssh contains the right settings.

If your login shell is **csh**, you can define **ECF_JOB_CMD** as:

.. code-block:: shell

    edit ECF_JOB_CMD "ssh %HOST% '%ECF_JOB% >& %ECF_JOBOUT%'"          

You can also submit tasks directly to the relevant queuing system on the
target machine. In fact, at ECMWF, we have written a UNIX script to
submit tasks to multiple systems and multiple queuing systems
(ecf_submit):

.. code-block:: shell

    edit ECF_JOB_CMD "ecf_submit %USER% %SCHOST% %ECF_JOB%             
    %ECF_JOBOUT%"                                                      

Alongside this, we include into our 'ecf' scripts a generic script
header containing typical queuing commands (such as wall clock time and
priority), e.g. contents of sample qsub.h:

.. code-block:: shell

    # QSUB -q %QUEUE%                                                                                                            
    # QSUB -u %USER%                                                                                                                     
    # QSUB -s /bin/ksh                                                                                                                       
    # QSUB -r %TASK%_%FAMILY1:NOT_DEF%                                                                                                       
    # QSUB -o %LOGDIR%%ECF_NAME%.%ECF_TRYNO%                                                                                                
    # QSUB -lh %THREADS:1%                                             

The ecf_submit script can replace these generic queuing commands with
the relevant commands for the host to which the task is submitted and
submit the task-relevant way, e.g. for a PBS system it replaces the QSUB
commands with the equivalent PBS commands.

Similarly to running a task remotely, to kill a task remotely you need
to either send a signal 2 (kill -2) to the task or issue the relevant
queuing system command. Again we have included all this information into
a script called "ecf_kill" that issue the correct command depending on
the host. This and other example scripts "ecf_status" (show status of
tasks) and "ecfurl" (open a web link for a task) are included in the
latest releases of ecFlow.

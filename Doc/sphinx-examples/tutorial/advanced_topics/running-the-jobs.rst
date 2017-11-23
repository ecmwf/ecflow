.. index::
   single: jobs
   single: ECF_JOB
   single: ECF_JOB_CMD
   single: ECF_JOBOUT
   
.. running-the-jobs:

Running the jobs
================

| To start a job, :term:`ecflow_server` uses the content of the ECF_JOB_CMD :term:`variable`. 
| By modifying this variable, it is possible to control where and how a :term:`job file` will run. 
| The command should be used in conjunctions with the :term:`variable` ECF_JOB and ECF_JOBOUT. 
| The ECF_JOB variable contains the :term:`job file` path and ECF_JOBOUT contains
| the path of a file where the output of the job will be written.

.. note::
   The default command ECF_JOB_CMD = %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &

| Let us run the tasks on a remote machine. For that we could use the unix command rsh. 
| We would like the name of the host to be defined by an :term:`variable` called HOST. 
| We assume that all the files are visible on all the hosts, i.e. using NFS.


In the examples below replace the string ?????? with a host name of your choice.

.. note:: 

   | The environment of a task running on a remote host is different from that of a task running locally.
   | This depends on how your system is set up. Here we need to set PATH, to allow :term:`child command`'s to be used. 
   | So add the following line into your :ref:`head_h` file before the call to :term:`ecflow_client` --init

   > export PATH=$PATH:/usr/local/apps/ecflow/current/bin

| You may experience other problems using rsh, caused by standard unix issues. 
| Make sure that the file $HOME/.rhosts contains a line with your user ID and 
| the machine where your server is running.

Modify the :term:`family` f5 so that all its tasks will run on another machine in the classroom

Text
----
   
::

   # Definition of the suite test
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    limit l1 2
 
    family f5
        edit HOST ?????? 
        edit ECF_JOB_CMD "rsh %HOST% '%ECF_JOB% > %ECF_JOBOUT% 2>&1 &'" 
        inlimit l1
        edit SLEEP 20
        task t1
        task t2
        task t3
        task t4
        task t5
        task t6
        task t7
        task t8
        task t9
    endfamily
   endsuite
   
If your login shell is csh, you should define ECF_JOB_CMD as::

   edit ECF_JOB_CMD "rsh %HOST% '%ECF_JOB% >& %ECF_JOBOUT%'"

Python
------

In python modify the function create_family_f5() created in the earlier page:

.. literalinclude:: src/running-the-jobs.py

            
**What to do:**

#. Modify PATH environment variable in head.h
#. Change the :term:`suite definition`
#. Replace the :term:`suite definition`
#. It may not work immediately. Have a look in the file :file:`$HOME/course/{host}.{port}.ecf.log` to see why.
#. What do you need to do in order to have the task **/test/f5/t9** run on another machine? Try your solution. 

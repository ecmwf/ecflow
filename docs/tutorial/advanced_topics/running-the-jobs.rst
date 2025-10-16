.. index::
   single: jobs (tutorial)
   single: ECF_JOB (tutorial)
   single: ECF_JOB_CMD (tutorial)
   single: ECF_JOBOUT (tutorial)
   
.. _tutorial-running-the-jobs:

Running (remote) jobs
=========================

To start a job, :term:`ecflow_server` uses the content of the ECF_JOB_CMD :term:`variable`.  By modifying this variable, it is possible to control where and how a :term:`job file` will run. The command should be used in conjunctions with the :term:`variable` ECF_JOB and ECF_JOBOUT. The ECF_JOB variable contains the :term:`job file` path and ECF_JOBOUT contains the path of a file where the output of the job will be written.

.. code-block:: bash

   ECF_JOB_CMD = %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &

Let us run the tasks on a remote machine. For that we could use the unix command rsh. We would like the name of the host to be defined by an :term:`variable` called HOST. We assume that all the files are visible on all the hosts, i.e. using NFS.

In the examples below replace the string ?????? with a host name of your choice.

.. note:: 

   The environment of a task running on a remote host is different from that of a task running locally. This depends on how your system is set up. Here we need to set PATH, to allow :term:`child command`\ s to be used.
   So add the following line into your :code:`head.h` file before the call to :term:`ecflow_client` --init ::

    export PATH=$PATH:/usr/local/apps/ecflow/%ECF_VERSION%/bin

To use ssh requires your public key to be available on the destination machine.
Check if you can log on to the remote machine through ssh without a password check.
If you need to enter a password you will need to add your public key on the destination machine. To do this issue the following commands:

.. code-block:: bash
   :caption: no password for ssh connection

   REMOTE_HOST=??????  # change me
   ssh $USER@$REMOTE_HOST mkdir -p \$HOME/.ssh      # if you are prompted for a password use your Training password that was provided
   cat $HOME/.ssh/id_rsa.pub || ssh-keygen -t rsa -b 2048
   cat $HOME/.ssh/id_rsa.pub | ssh $USER@$REMOTE_HOST 'cat &gt;&gt; $HOME/.ssh/authorized_keys'


Modify the :term:`family` f5 so that all its tasks will run on another machine in the classroom

Text
----

.. code-block:: shell

   # Definition of the suite test
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    limit l1 2

    family f5
        edit HOST ??????
        edit ECF_OUT /tmp/$USER
        edit ECF_JOB_CMD "ssh %HOST% 'mkdir -p %ECF_OUT%/%SUITE%/%FAMILY% &amp;&amp; %ECF_JOB% &gt; %ECF_JOBOUT% 2&gt;&amp;1 &amp;'"
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

   
If your login shell is csh, you should define ECF_JOB_CMD as:

.. code-block:: shell

   edit ECF_JOB_CMD "ssh %HOST% 'mkdir -p %ECF_OUT%/%SUITE%/%FAMILY%; %ECF_JOB% &gt;&amp; %ECF_JOBOUT%'"


Python
------

In python modify the function create_family_f5() created in the earlier page, to add HOST,ECF_OUT,ECF_LOGHOST,ECF_LOGPORT, and ECF_JOB_CMD:

.. literalinclude:: src/running-the-jobs.py
   :language: python
   :caption: $HOME/course/test.py

Logserver
-----------

We can view the output on the remote machine (class??) by using a log server. This assumes you have defined variables ECF_LOGHOST and ECF_LOGPORT in your definition. Launch the log server on a remote machine:

.. code-block:: bash

   ssh $USER@class01 /usr/local/apps/ecflow/5.5.1/bin/ecflow_logserver.sh -d /tmp/$USER -m /tmp/$USER:/tmp/$USER


**What to do**

#. Modify PATH environment variable in head.h
#. Change the :term:`suite definition`
#. Replace the :term:`suite definition`
#. It may not work immediately. Have a look in the file :file:`$HOME/course/{host}.{port}.ecf.log` to see why.
#. Add a ``uname -n`` to your ECF script to see what machine the task is running on.
#. What do you need to do in order to have the task **/test/f5/t9** run on another machine? Try your solution. 
#. Create a log server, to access the remote output

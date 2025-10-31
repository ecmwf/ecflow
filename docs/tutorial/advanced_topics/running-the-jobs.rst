.. index::
   single: jobs (tutorial)
   single: ECF_JOB (tutorial)
   single: ECF_JOB_CMD (tutorial)
   single: ECF_JOBOUT (tutorial)
   
.. _tutorial-running-the-jobs:

Running (remote) jobs
======================

To start a job, the :term:`ecflow_server` uses the content of the :code:`ECF_JOB_CMD` :term:`variable`.
By modifying this variable, it is possible to control where and how a :term:`job file` will run.
The command should be used in conjunctions with the :term:`variable` :code:`ECF_JOB` and :code:`ECF_JOBOUT`.
The :code:`ECF_JOB` variable specifies the :term:`job file` path and the :code:`ECF_JOBOUT` defines the path of a file where the output of the job is stored.

.. code-block:: bash
   :caption: A typical :code:`ECF_JOB_CMD`, spawns the job on the local machine in the background

   ECF_JOB_CMD = %ECF_JOB% 1> %ECF_JOBOUT% 2>&1 &

Updating :code:`ECF_JOB_CMD` allows to run the tasks on a remote machine, taking advantage of the unix command rsh.


In the following examples, consider a variable :code:`HOST` that defines the name of the remote host, and assume that all the files are visible on all the hosts (i.e. using NFS).
replace the string :code:`<REMOTE-HOSTNAME>` with a host name of your choice.

.. note:: 

   The environment of a task running on a remote host is different from that of a task running locally.
   This depends on how the local and remote systems are set up.

   On the remote system, it is likely that the environment variablel :code:`PATH` needs to be adjusted to allow using :term:`task commands <child command>`.

   Consider adding the following line to the :code:`head.h` file, before calling :code:`ecflow_client --init ...`.

   .. code-block:: bash

      export PATH=$PATH:/usr/local/apps/ecflow/%ECF_VERSION%/bin

For the following setup ensure an Ssh connection, based on private/public key, is available to the remote machine.

Attempt to access to the remote machine through ssh without a password.
In case a password is requested, consider adding the public key on the remote machine, with the following commands:

.. code-block:: bash
   :caption: no password for ssh connection

   REMOTE_HOST=<REMOTE-HOSTNAME>  # change this to the remote host name
   ssh $USER@$REMOTE_HOST mkdir -p \$HOME/.ssh
   cat $HOME/.ssh/id_rsa.pub || ssh-keygen -t rsa -b 2048
   cat $HOME/.ssh/id_rsa.pub | ssh $USER@$REMOTE_HOST 'cat &gt;&gt; $HOME/.ssh/authorized_keys'

Suite Definition
----------------

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file as follows:

        .. code-block:: shell

            # Definition of the suite test
            suite test
             edit ECF_INCLUDE "$HOME/course"
             edit ECF_HOME    "$HOME/course"
             limit l1 2

             family f5
                 edit HOST <REMOTE-HOSTNAME>
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

        When using csh as login shell, define :code:`ECF_JOB_CMD` as:

        .. code-block:: shell

            edit ECF_JOB_CMD "ssh %HOST% 'mkdir -p %ECF_OUT%/%SUITE%/%FAMILY%; %ECF_JOB% &gt;&amp; %ECF_JOBOUT%'"


    .. tab:: Python


        Modify the function :code:`create_family_f5()` created earlier, to add :code:`HOST`, :code:`ECF_OUT`, :code:`ECF_LOGHOST`, :code:`ECF_LOGPORT`, and :code:`ECF_JOB_CMD`.

        .. literalinclude:: src/running-the-jobs.py
           :language: python
           :caption: $HOME/course/test.py

Logserver
-----------

The job output generated on the remote machine can be inspected by using a *log server*.
This assumes that variables :code:`ECF_LOGHOST` and :code:`ECF_LOGPORT` are present in the :term:`suite definition`.

Launch the log server on a remote machine:

.. code-block:: bash

   ssh $USER@<REMOTE-HOSTNAME> /path/to/ecflow/%ECF_VERSION%/bin/ecflow_logserver.sh -d /tmp/$USER -m /tmp/$USER:/tmp/$USER


**What to do**

#. Adjust the :code:`PATH` environment variable in :file:`head.h`
#. Apply the changes to :term:`suite definition`.
#. In the :term:`ecflow_ui`, execute the suite.
#. In case of errors, inspect the ecflow server log file (i.e. :file:`{host}.{port}.ecf.log`) and determine what is the cause of the error.
#. Add :code:`uname -n` to the :term:`task script <ecf script>` to determine in which machine the task is running.
#. Launch the log server, and access the remote job output using the :term:`ecflow_ui`.

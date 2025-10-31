.. index::
   single: autoarchive (tutorial)
   single: autoarestore (tutorial)

.. _tutorial-autoarchive-and-autorestore:

Autoarchive and Autorestore
==============================

In the real world, suites can have several thousand tasks, where some of these tasks are not effectively required permanently.

This is where :code:`autoarchive` becomes useful, as having a server with an extremely large number of tasks can cause performance issues:

- The server writes to the checkpoint file periodically, meaning that disk i/o can interfere with job scheduling when dealing with an excessively large number of tasks.
- Clients like GUI(ecflow_ui), are also adversely affected by the memory requirements and may slow interactive experience.
- Network traffic is heavily affected

.. code-block:: shell
   :caption: :code:`autoarchive` example

   autoarchive +01:00 # archive one hour after complete
   autoarchive 01:00  # archive at 1 am in morning after complete
   autoarchive 10     # archive 10 days after complete
   autoarchive 0      # archive immediately after complete (can take up to a minute)

In practive, :code:`autoarchive` writes a portion of the definition to disk, considering the following:

- suite or family nodes are archived *IF* they have child nodes (otherwise, no changes are applied).
- suite or family nodes are saved to disk, and then the in-memory child nodes are removed from the definition.
- when an archived node is requeued or begun, the child nodes are automatically restored.
- nodes are saved to :code:`ECF_HOME/<host>.<port>.ECF_NAME.check` (where :code`/` has been replaced with :code`:` in :code:`ECF_NAME`)
- care must be taken if you have trigger reference to the archived nodes, as these will not be available until restored.
- this results in reduced time taken to checkpoint, and improved network bandwidth.

The command :code:`ecflow_client --archive` can be used to archive **manually** nodes.

.. code-block:: shell

   ecflow_client --archive=/s1           # archive suite s1
   ecflow_client --archive=/s1/f1 /s2    # archive family /s1/f1 and suite /s2
   ecflow_client --archive=force /s1 /s2 # archive suites /s1,/s2 even if they have active tasks

The :code:`autorestore` can also be done automatically, but is **only** applied when a node **completes**.

To **manually** restore archived nodes use the command :code:`ecflow_client --restore`.

.. code-block:: shell

   ecflow_client --restore=/s1/f1  # restore family /s1/f1
   ecflow_client --restore=/s1 /s2 # restore suites /s1 and /s2

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file as follows (n.b. to avoid waiting this exercise will archive immediately on task completion):

        .. code-block:: bash

            # Definition of the suite test.
            suite test
             edit ECF_INCLUDE "$HOME/course"
             edit ECF_HOME    "$HOME/course"
             edit SLEEP 20
             family lf1
                 autoarchive 0
                 task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
             endfamily
             family lf2
                 autoarchive 0
                 task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
             endfamily
             family lf3
                 autoarchive 0
                 task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
             endfamily
             family restore
                trigger ./lf1<flag>archived and ./lf2<flag>archived and ./lf3<flag>archived
                task t1
                   autorestore ../lf1 ../lf2 ../lf3.   # restore when t1 completes
             endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/autoarchive-and-autorestore.py
           :language: python
           :caption: $HOME/course/test.py


**What to do**

#. Apply the changes to :term:`suite definition`.
#. Copy the task script from the previous section example into the new families directories.
#. In the :term:`ecflow_ui`, run the suite and observer the nodes getting archived then restored.
#. Experiment with manually archive and restore using the :term:`ecflow_ui`.
#. Experiment with manually archive and restore using the :term:`ecflow_client`.

.. note::

   The :code:`autoarchive 0` can take up to one minute to take effect. The server has a 1-minute resolution.

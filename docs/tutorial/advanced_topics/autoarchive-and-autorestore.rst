.. index::
   single: autoarchive (tutorial)
   single: autoarestore (tutorial)

.. _tutorial-autoarchive-and-autorestore:

Autoarchive and autorestore
==============================

In the real world, suites can have several thousand tasks. These tasks are not required all the time. Having a server with an extremely large number of tasks can cause performance issues.

- The server writes to the checkpoint file periodically. This disk i/o can interfere with job scheduling when dealing with an excessively large number of tasks.
- Clients like GUI(ecflow_ui), are also adversely affected by the memory requirements, and slow interactive experience 
- Network traffic is heavily affected
  
This is where autoarchive becomes useful.


.. code-block:: shell
   :caption: autoarchive example

   autoarchive +01:00 # archive one hour after complete
   autoarchive 01:00  # archive at 1 am in morning after complete
   autoarchive 10     # archive 10 days after complete
   autoarchive 0      # archive immediately after complete, can take up to a minute

Autoarchive will write a portion of the definition to disk.

- Archives suite or family nodes *IF* they have child nodes(otherwise does nothing).
- Saves the suite/family nodes to disk, and then removes the in-memory child nodes from the definition.
- It improves time taken to checkpoint and reduces network bandwidth
- If archived node is re-queued or begun, the child nodes are automatically restored
- The nodes are saved to ECF_HOME/<host>.<port>.ECF_NAME.check, where '/' has been replaced with ':' in ECF_NAME
- Care must be taken if you have trigger reference to the archived nodes

Use  ecflow_client --archive to archive **manually**:

- ecflow_client --archive=/s1                       # archive suite s1
- ecflow_client --archive=/s1/f1 /s2            # archive family /s1/f1 and suite /s2
- ecflow_client --archive=force /s1 /s2      # archive suites /s1,/s2 even if they have active tasks

Autorestore can also be done automatically, but is **only** applied when a node **completes**.

To restore archived nodes **manually** use: 

- ecflow_client --restore=/s1/f1     # restore family /s1/f1
- ecflow_client  --restore=/s1 /s2  # restore suites /s1 and /s2

Text
-----

Let us modify the :term:`suite definition` file again. To avoid waiting this exercise will archive immediately.

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

Python
----------

.. literalinclude:: src/autoarchive-and-autorestore.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Edit the changes i.e. cp -r f5 lf1; cp -r f5 lf2; cp -r f5 lf3; 
#. Replace the :term:`suite definition`
#. Run the suite, you should see nodes getting archived, then restored in :term:`ecflow_ui`
#. Experiment with archive and restore in ecflow_ui.
#. Experiment with archive and restore from the command line.

.. note::

   The Autoarchive(0) can take up to one minute to take effect. The server has a 1-minute resolution.



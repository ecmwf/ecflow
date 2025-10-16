.. _tutorial-execute_rerun_and_requeue:

Execute, rerun and requeue
==========================

When using the :ref:`ecflow_ui` it is important to understand the distinction between
**execute**, **rerun** and **re-queue** .

These options are available for tasks via the context menu of the the tree view panel:

- **Execute**

  This means run the task immediately, ignoring any dependency that holds the task from executing.
  This option preserves previous job outputs, by incrementing and including a counter in the name of the output files
  each time the task is run (e.g. *task.1, task.2, task.3*).
                                                                       
- **Rerun**

  This places the task, back into the queued state. The task will now honour any dependencies that would
  hold the job (e.g. time dependencies, trigger, limits), and when it does run, it will preserve previous outputs.

- **Re-queue**

  This resets the task back to the queued state, including setting a default status if defined.
  The task output counter is reset, such that the next output will be written to **task.1**.
  This option will **overwrite** any existing output with that extension when the task runs.
  Any subsequent calls to execute or rerun will now **overwrite** the output files, **t1.2 t1.3**.

**What to do**

#. Suspend the suite, by selecting the suite node :code:`test`, and then selecting the *suspend* option from the context menu.
   
#. Select task :code:`t1`, and select *Execute* from the context menu.
   Even though the parent is suspended, that task will run. Do this several times.
   Notice that the output is preserved for each run -- see list of files on the *Output* tab.

#. Select task :code:`t1`, and select *Rerun* from the context menu.
   The node is placed in the queued state, but because we had suspended its parent it will not run.
   Resume the parent node :code:`test`. Task :code:`t1` will start execution.
   Notice that previous output is preserved.

#. Suspent the suit again.

#. Select task :code:`t1`, and select *Requeue* from the context menu.
   The node is placed in the queued state. The parent node is suspended and will prevent the task from running.
   Resume the parent node :code:`test`. Task :code:`t1` will start execution.
   Notice that execution overwrites the output file.

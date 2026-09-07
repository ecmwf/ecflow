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

Attributes reset by each option
-------------------------------

The difference between the three options goes well beyond the handling of the output files,
and is a frequent source of confusion.

*Execute* issues :code:`ecflow_client --run <path>`, and submits the job immediately, without
placing the node back into the :term:`queued` state and without resetting any attribute.
*Rerun* issues :code:`ecflow_client --force queued <path>`, and is deliberately minimal: it
changes the state of a single task, and resets only the attributes that other nodes may be
observing in a :term:`trigger` expression. *Re-queue* issues
:code:`ecflow_client --requeue=force <path>`, and is a comprehensive reset, applied recursively
to the node and all of its children.

.. list-table::
   :header-rows: 1
   :widths: 22 26 26 26

   * - 
     - Execute
     - Rerun
     - Re-queue
   * - Command issued
     - :code:`--run`
     - :code:`--force queued`
     - :code:`--requeue=force`
   * - Scope
     - the selected task
     - the selected node only
     - the node and, recursively, all its children
   * - Resulting state
     - :term:`submitted`, then :term:`active`
     - :term:`queued`
     - :term:`queued`, or the :term:`defstatus`, when defined
   * - Dependencies
     - ignored (:term:`triggers <trigger>`, time based attributes, :term:`limits <limit>`)
     - honoured on the next run
     - honoured on the next run
   * - Job output counter (:term:`ECF_TRYNO`)
     - incremented, so previous output is preserved
     - preserved, and incremented on the next run
     - reset, so the next run **overwrites** *task.1*
   * - :term:`events <event>`, :term:`meters <meter>`, late flag
     - preserved
     - reset
     - reset
   * - :term:`limit` tokens
     - consumed on submission, even beyond the limit
     - released
     - released
   * - Time based attributes (:term:`time`, :term:`today`, :term:`cron`)
     - the next time slot is missed, and thus advanced
     - the next time slot is neither reset nor re-evaluated
     - the next time slot is re-evaluated, relative durations are reset
   * - :term:`repeat` attributes
     - preserved
     - preserved
     - reset to their starting value
   * - Aborted reason, job password, process identifier
     - preserved
     - preserved
     - cleared
   * - Node flags (late, aborted, killed, jobcmd_failed, ...)
     - preserved
     - preserved
     - cleared, except the message and archived flags
   * - Queue attributes
     - preserved
     - preserved
     - reset
   * - :term:`labels <label>`
     - preserved
     - preserved
     - reset on suites and families only, **not** on tasks
   * - :term:`suspended` status
     - unchanged, and the job runs even when a parent is suspended
     - unchanged
     - cleared on the children, **not** on the selected node itself

From the table above, it is worth noting the following:

 - regardless of the option used, the :term:`labels <label>` of a task are reset when its job is submitted,
   together with a subset of the node flags; the table records only what each command itself does.
 - *Execute* and *Rerun* preserve the previous output because they leave the output counter alone,
   whereas *Re-queue* sets it back to zero, so that the next run writes to *task.1* again.
 - All three options require the enclosing :term:`suite` to have been begun.


In practice, use *Execute* to run a task now regardless of its dependencies, use *Rerun* to let a
task run again through its dependencies while keeping the history of the previous runs, and use
*Re-queue* to bring a node back to a pristine state, as if it had never run.

**What to do**

#. Suspend the suite, by selecting the suite node :code:`test`, and then selecting the *suspend* option from the context menu.

#. Select task :code:`t1`, and select *Execute* from the context menu.
   Even though the parent is suspended, that task will run. Do this several times.
   Notice that the output is preserved for each run -- see list of files on the *Output* tab.

#. Select task :code:`t1`, and select *Rerun* from the context menu.
   The node is placed in the queued state, but because the parent has been suspended it will not run.
   Resume the parent node :code:`test`. Task :code:`t1` will start execution.
   Notice that previous output is preserved.

#. Suspend the suite again.

#. Select task :code:`t1`, and select *Requeue* from the context menu.
   The node is placed in the queued state. The parent node is suspended and will prevent the task from running.
   Resume the parent node :code:`test`. Task :code:`t1` will start execution.
   Notice that execution overwrites the output file.

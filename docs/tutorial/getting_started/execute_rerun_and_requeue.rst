.. _tutorial-execute_rerun_and_requeue:

Execute, rerun and requeue
//////////////////////////

In :ref:`ecflow_ui` it is important to understand the distinction between
**execute rerun** and **re-queue** .

These options are available with the right mouse button over a task.

- **Execute**: This means run the task immediately. (Hence ignores any dependency that holds the task). This option preserves the previous output, from the job. You should see output files **t1.1 t1.2 t1.3** , each time the task is run.                                                               
                                                                       
- **Rerun**: This places the task, back into the queued state. The task will now honour any dependencies that would hold the job. i.e. time dependencies, limits, triggers, suspend, etc. (You will be introduced to these terms later on in the tutorial). If the task does run, the previous output is preserved.

- **Re-queue**: This resets the task back to the queued state. If the task has a default status, this is applied. The task output  number is reset, such that the next output will be written to **t1.1**. This will **overwrite** any existing output with that extension when the task runs. Any subsequent calls to execute or rerun will now **overwrite** the output files, **t1.2 t1.3** etc.                               

**What to do**

#. Suspend the suite. i.e. select the node 'test', then with the right mouse button select 'suspend' from the menu. 
   
#. Select task t1, and select execute from the right mouse button menu. Even though the parent is suspended, that task will run. Do this
   several times. Notice that the output is preserved for each run. **Click on output tab, in the info panel**.

#. Select task t1, and select rerun from the right mouse button menu. The node is placed into the re-queue state. But because we had suspended its parent it will not run. Resume the parent node 'test'. Task t1 will now run. Notice that old output is preserved.

#. Select node 'test' and with right mouse button select 'suspend'.

#. Select task t1, and select re-queue from the right mouse button menu. The node is placed in a queued state. The parent node is suspended and will prevent the task from running. 

#. Resume the parent node. This task t1 will now run. However, it will
   overwrite the output file, **t1.1**

.. index::
   single: alias (tutorial)

.. _tutorial-alias:

Alias
=====

A very useful addition to "edit" is the ability to clone a task, perform some minor modifications to its script, and run it as an "alias". (This can be very useful to debug your scripts)
Such a task can run under the control of the ecFlow but has no impact on the activity of the suite itself i.e. it will have no dependencies.
This feature can be used to rerun a task for a previous date or to solve transient problems such as full file systems.
In this case, the task can be run using a different disk, in order to guarantee the completion of the suite on schedule.
The analyst then has time to understand why this condition arises and take the necessary actions.
 
**What to do**

#. Pick an existing task and select the Edit tab, in the **info panel**.
#. Make some changes to the script. i.e. add something like sleep 1
#. Select 'Submit as alias', then submit the job. You should notice that a new node(alias) is added as a child of the task.
#. Select the same task, and repeat the process. This shows a task can have many aliases.Also, it should be noticed that your edit has been preserved.
#. Select one of your aliases and force it to be aborted. Right Mouse Button > Force > Aborted. Notice that alias node state is **NOT** propagated, to the parent task.
#. Finally select alias in the tree view, and with Right Mouse Button, select 'Remove'. The alias node is removed from the tree.
 
   .. note::

    If the 'Remove' menu is **not** available, please select Tools > Preferences. Then in the dialog select Menus. Finally, change menu mode to Administrator.
    
    Now retry step 6.

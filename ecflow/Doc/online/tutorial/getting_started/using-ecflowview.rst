.. index::
   single: Using ecflowview
   
.. _using_ecflowview:
   
Using ecflowview
================

| Let us have a look at :term:`ecflowview`, this provide the GUI for ecFlow.
| On the unix prompt type :term:`ecflowview`. 

Once the program is up:

* Select :guilabel:`"Preferences"`  from the :guilabel:`&Edit`  menu.
* Next click on the :guilabel:`Ser&vers` tab and enter details of your :term:`ecflow_server`. 
  
  Choose a name for your :term:`ecflow_server` and add the host name and port number 
  
  you noted down earlier and press the add button.
  
| You can now select the :term:`ecflow_server` from the main :guilabel:`Ser&ver` menu on :term:`ecflowview`. 
| You should get something similar to the following display :

.. image:: xecf0.*

The yellow boxes are called nodes. With the middle mouse button click on the node test.

.. image:: xecf1.*

| You can now view the :term:`ecf script`, the :term:`job file` and the output of the :term:`task` **t1**. 
| For that click on the :term:`node` **t1**, then press on the script icon. A window should open showing 
| the task script:

.. image:: xecf2.*

| To see the :term:`job file` or the output simply click on the tabs. 
| If you want to see the output in a different window, click on the output icon.

If we want to rerun the suite, move the mouse pointer over **test** and click on the right mouse button. 

| A pop-up menu will appear. Choose **requeue** (If the menu does not offer a **requeue** option, 
| select **Preferences...**  in the **Edit** menu and set the User level to **Administrator**)

| You will notice that the :term:`node`'s change colour. 
| The colours reflect the :term:`status` of the nodes.

.. image:: xecf4.*
 
| We have seen that :term:`ecflowview` uses the three buttons of the mouse to perform different actions. 
| The following figure show how the mouse buttons are used in :term:`ecflowview`:

.. image:: mouse.*

It is possible to walk the server tree displayed with ecflowview with keyboard keys:

* arrows up-down-left-right to move up and down the tree,
* Space  as click1 (like left mouse button, select node below pointer),
* F2     as click2 (expand/collapse tree),
* Return as click3 (open node or host menus according to pointer position)

**icons:**

Task nodes may have icons visible:

#. |byrule| task was completed by rule (complete expression) 
#. |cmd_failed| ECF_JOB_CMD failed 
#. |edit_failed| invalid script leading to inability to edit the script (missing micro % character ?) 
#. |force_abort| task was forced aborted by a user 
#. |killed| task was killed following a request by a user 
#. |no_script_found| script missing for task (ECF_HOME incorrect, ECF_FILE incorrect, or no script visible to this task under ECF_HOME or ECF_FILE directory) 
#. |queue_limit| task cannot be started due to a queue limit 
#. |task_aborted| task aborted
#. |user_edit| user edited the script and submitted it 

.. |byrule| image:: icon_byrule.gif
.. |cmd_failed| image:: icon_cmd_failed.gif
.. |edit_failed| image:: icon_edit_failed.gif
.. |force_abort| image:: icon_force_abort.gif
.. |killed| image:: icon_killed.gif
.. |no_script_found| image:: icon_no_script.gif
.. |queue_limit| image:: icon_queuelimit.gif
.. |task_aborted| image:: icon_task_aborted.gif
.. |user_edit| image:: icon_user_edit.gif

**What to do:**

1. Explore :term:`ecflowview` menus and windows


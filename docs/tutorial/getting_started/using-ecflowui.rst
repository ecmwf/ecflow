.. index::
   single: Using ecflow_ui (tutorial)
   
.. _tutorial-using_ecflowui:
   
Using ecflow_ui
================

The following diagram shows the typical status changes for a :term:`task`.

.. image:: /_static/tutorial/using_ecflow_ui/image3.png
   :width: 6.29167in
   :height: 2.125in


Let us have a look at :ref:`ecflow_ui`, this provides the GUI for ecFlow. On the UNIX prompt type:

.. code-block:: shell
  
  ecflow_ui

Once the program is up:

- Select *“Servers” menu.*
- Next click on the "Manage Servers..." menu
- "Add server" enter details of your ECF server.

You should also change the preferences to "Administrator" mode, which shows an enhanced set of menu options.

- In ecflow_ui: Select "Tools > Preferences"
- Select Menus and change "menu mode" to "Administrator".

Choose a name for your ECF server and add the Host name and port number you noted down earlier. Make sure 'Add server to current view' is clicked on the dialog. You should get something similar to the following display:

.. image:: /_static/tutorial/using_ecflow_ui/image1.png
   :width: 6.64028in
   :height: 4.16667in

.. image:: /_static/tutorial/using_ecflow_ui/image2.png
  :width: 0.16667in
  :height: 0.16667in

The yellow boxes are called nodes. With the left mouse button click on
the black triangle. This will expand the node. Repeat for node called
'test'. 

.. note::
  
  S - stands for the suite, F stands for Family, and T - stands for Task

.. image:: /_static/tutorial/using_ecflow_ui/image22.png
   :width: 6.64028in
   :height: 4.16667in

Now select the View menu and then "Add info panel". This panel can be re-arranged on the window.

You can now view the :term:`ecf script`, the :term:`job file` the and the output of the :term:`task` **t1** .                
                                                                       
For that click on the :term:`node` **t1** then press the "Script" tab.                           

.. image:: /_static/tutorial/using_ecflow_ui/image24.png
   :width: 6.64028in
   :height: 4.16667in

To see the ECF job or the output simply click on the tabs. If you want to see the output in a different window, select the node
**t1** , then with the right mouse button, select the "Output..." menu
option.

If we want to rerun the suite, move the mouse pointer over **test** and
click on the right mouse button, and choose **requeue** .

You will notice that the :term:`node` changed colour. The colours reflect the :term:`status` of the nodes.

There can be a long between the job changing status and that change being reflected in the ecflow_ui window (set to 60s as default). To update the current state more quickly in that window, click the
refresh (or F5).

.. image:: /_static/tutorial/using_ecflow_ui/image25.png
   :width: 6.64028in
   :height: 4.16667in

We will now add a table view. This allows us to see additional
attributes when a node is selected. From the View menu, select "Add
table view":

.. image:: /_static/tutorial/using_ecflow_ui/image26.png
   :width: 6.64028in
   :height: 4.16667in

You will notice that selection in the tree view, selects the
corresponding node in the table view.

Task nodes contain a flag that can reflect the reason for its being
aborted, complete, or submitted. Here is the list of the associated icons that can be displayed in :ref:`ecflow_ui`:

.. image:: /_static/tutorial/using_ecflow_ui/image27.png
   :width: 1.875in
   :height: 3.52917in

**What to do**

#. Explore ecflow_ui menus and windows

#. There can be a long delay (60s) between the job changing status and that change being reflected in the :ref:`ecflow_ui` window. To update the current state more quickly in that window, click on the green icon(top left-hand side), on the toolbar (or F5)
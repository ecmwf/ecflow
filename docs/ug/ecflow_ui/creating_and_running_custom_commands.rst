.. _creating_and_running_custom_commands:

Creating and running custom commands
////////////////////////////////////


ecFlowUI has a built-in editor for specifying your own commands if they
do not appear in the supplied context menu. Once you have run a new
command, it is recorded and will be added automatically to the context
menu under **Custom**. A maximum of the 10 most recently used custom
commands will appear here. You can also manage a list of saved commands,
which will appear before the **Recent** commands.

.. image:: /_static/ecflow_ui/creating_and_running_custom_commands/image1.png
   :width: 5.20833in
   :height: 4.30452in

When you select **Custom \| Manage commands...** from the node context
menu, the following editor appears in **Build command** mode:

.. image:: /_static/ecflow_ui/creating_and_running_custom_commands/image2.png
   :width: 5.90069in
   :height: 3.00238in

The **Command** line is already partially filled in with *ecflow_client*
and *<full_name>*, and the text cursor positioned between them.
Single-clicking on an action from the list brings up its associated help
page, while double-clicking on it inserts it into the command line.

Click the **Run** button to execute the command on the selected nodes.
The node selection can be refined before this step by clicking into the
**Selected Nodes** tab and deselecting some nodes from the list.

.. image:: /_static/ecflow_ui/creating_and_running_custom_commands/image3.png
   :width: 5.90069in
   :height: 3.62162in

When the command is run, the placeholders *<node_name>* and
*<full_name>* are replaced with a space-separated list of the names of
the selected nodes so that the command is performed on all the nodes in
one go.

Clicking on the **Save Options** button reveals the panel from which you
can save the current command to a list, and clicking on the **Saved
commands** tab switches the editor to the **Saved commands** tab:

.. image:: /_static/ecflow_ui/creating_and_running_custom_commands/image4.png
   :width: 5.90069in
   :height: 4.00881in

From here, you can do the following:

-  save your current command under a new name; the name will be used in
   the **Custom** context sub-menu, and the names must be unique

-  edit existing saved commands

-  duplicate, remove and re-order saved commands

-  recall a saved command into the **Command** edit line to be modified
   and run

Running shell commands
======================

This system can also be used to run and save arbitrary shell commands.
To do this, simply replace 'ecflow_client' with 'sh', followed by the
command and any arguments, for example::

    sh ls -l                                                           

The command will have as its working directory the directory from where
ecFlowUI was started. Expressions *<node_name>* and *<full_name>* are
substituted accordingly. Once the command has run, and output window
will appear, from where you can see the stdout and stderr from the
command, along with details of and output from the last 20 commands run
this session (also available from **Tools > Shell command
output**). This window can be prevented from popping up by using the
options in **Tools > Preferences > Menus > Shell commands**.

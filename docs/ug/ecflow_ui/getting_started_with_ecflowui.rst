.. _getting_started_with_ecflowui:

Getting Started with ecFlowUI
/////////////////////////////

Starting ecFlowUI
=================

The command to start the application from ECMWF workstations and ecgate
is::

   module load ecflow/5new                                            
   ecflow_ui                                                          

**This version should only be for testing the new user interface and not
for running a server!**

Starting ecFlowUI for the first time
====================================

When starting ecFlowUI for the first time, the application contains an
empty tree view:

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image1.png
   :width: 4.13706in
   :height: 2.60417in

The first thing to do is to populate it with servers.

Go to the **Servers** menu; this should contain all the servers that you
have configured for ecFlowview (these are copied over once, when you
first start ecFlowUI; from then on, the two lists are not synchronised).

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image2.png
   :width: 5.87164in
   :height: 1.95798in

New servers can be added by selecting **Manage servers**. This dialogue
is also where servers can be set as *favourites* for easy access.

Selecting a server will add it to the tree view. For large servers, a
**Loading** indicator may appear, but the user interface is still
responsive.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image3.png
   :width: 3.67784in
   :height: 1.84218in

Once loaded, a server appears in the tree view. Server and suite nodes
show, in brackets, the total number of children under them. Any number
of servers can be added to the view.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image4.png
   :width: 3.75in
   :height: 3.80096in

Viewing the tree
================

The tree nodes are expanded and collapsed using the small icons to the
left of each node (these may vary in appearance depending on your
workstation setup). Alternatively, double-click on the node name.

At the top of the tree is a set of *breadcrumbs*, showing the hierarchy
up to the currently-selected node. Clicking on a node in the breadcrumbs
will select the node in the tree.


.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image4.png
   :width: 2.56925in
   :height: 2.60417in

At the top-right of the panel are three icons which give access to:

-  Status filter (show only nodes with the selected statuses)

-  Show attributes

-  Show icons

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image5.png
   :width: 1.54277in
   :height: 2.60417in

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image6.png
   :width: 1.54277in
   :height: 2.60417in

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image7.png
   :width: 2.52866in
   :height: 2.60417in


When a node is selected, the toolbar at the top of the tree view has a
button which reveals options to display various information for the
selected node, such as Output and Script. Clicking one of these pops up
a floating Info Panel displaying the selected information in a tab.
Which attributes are shown by default can be changed from the **Edit >
Preferences** menu.

Node Information
================

At the top-right of the window is a group of icons - click on one of
these to obtain a floating information panel for the currently-selected
node. The panel is "detached" by default, meaning that its contents will
not change if a different node is selected.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image8.png
   :width: 3.04336in
   :height: 0.77898in

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image9.png
   :width: 2.88053in
   :height: 2.60417in

The panel can be detached or frozen by using the configuration menu at
the top-right of the panel:

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image10.png
   :width: 2.51642in
   :height: 1.77902in

It is also possible to have a permanently docked Info Panel: select
**Add info panel** from the **View** menu.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image11.png
   :width: 1.86043in
   :height: 0.92635in

The information panel appears at the bottom of the window.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image12.png
   :width: 3.28213in
   :height: 2.60417in

This docked information panel is linked to the currently-selected node,
but can also be detached and frozen using the configuration menu at the
top-right of the panel.

Layout
======

Each panel in ecFlowUI is resizeable and moveable, following a
*dashboard* approach. For example, drag the information panel to the
right of the tree view to get an arrangement like this:

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image13.png
   :width: 4.7092in
   :height: 2.60417in

Interacting with the servers
============================

In the tree view, the right-click context menu provides most of the
standard commands. Hovering over a menu option shows, in the status bar,
the command that will be sent to the server.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image14.png
   :width: 1.23049in
   :height: 2.60417in

It is possible to select multiple nodes with the mouse and select a
command which will be sent to them all. Also, using the **Custom...**
menu entry, any command may be typed in.

Configuration and setup
=======================

The list of available servers can be managed through the **Servers >
Manage Servers** menu.

Global server settings, notifications settings plus other settings such
as colour setup and fonts, can be found in the **Edit > Preferences**
menu.

Settings per server can be found in the Information panel, in the
**Settings** tab (when a server node is selected in the tree).

Table view
==========

From the toolbar at the top of the window, a new Table View panel can be
added. This provides an alternative, flat, way to view the nodes, and
allows sorting and filtering of nodes. This feature is still
experimental and is known to be slow when acting on large servers.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image15.png
   :width: 3.51639in
   :height: 2.60417in

Tabs and windows
================

ecFlowUI supports multiple windows and multiple tabs, available from the
**File** menu. Each tab contains its own list of active servers, and the
tab control itself shows the status of each server in that tab.

.. image:: /_static/ecflow_ui/getting_started_with_ecflowui/image16.png
   :width: 5.90069in
   :height: 1.44843in

Likewise, each window is separate from the other windows, each
containing their own set of tabs and therefore servers.

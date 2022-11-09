.. _communication_with_ecflow_servers:

Communication with ecFlow servers
/////////////////////////////////

Overview
========

ecFlowUI can observe and interact with many ecFlow servers at the same
time. Server communication has the potential to be costly in terms of
network usage and in waiting time, therefore ecFlowUI uses a separate
execution thread for communicating with each server. This allows the
user interface to remain responsive while communicating with busy
servers.

Each tab in ecFlowUI is linked to a specific set of servers; if you have
multiple tree or table views in a tab, they will all be linked to the
same set of servers.

ecFlow servers can have a heavy load, which can impact all users, so
ecFlowUI employs some strategies to reduce strain on the server. Users
are encouraged to note the information below and to help reduce
unnecessary server traffic.

Suite filters
=============

To save screen space and to reduce load on the server, ecFlowUI allows
you to select a subset of the available suites on a given server. Select
the server node in the tree, go to the Info Panel and select **Suite
Filter**. Enable the filter, then select just those suites of interest
and click the **Submit** button. This informs the server that you only
require updates for those suites, thus reducing network traffic and
server load.

.. image:: /_static/ecflow_ui/communication_with_ecflow_servers/image1.png
   :width: 4.16667in
   :height: 2.89135in

Refreshing and resetting a server
=================================

After initially loading a server's node tree, ecFlowUI will
automatically **refresh**, or synchronise with, the server at regular
intervals. This interval can be changed in the `Server
Settings <#changing-the-server-settings>`__. If a `suite
filter <#suite-filters>`__ is enabled, the synchronisation process only
considers those suites. There are three possible outcomes from the
**refresh**:

-  nothing has changed on the server since the last synchronisation

-  minor changes have occurred, such as nodes changing their status -
   the changes are sent by the server and incorporated into ecFlowUI's
   tree

-  major changes have occurred, such as nodes being added or removed - a
   complete new tree is sent by the server, causing a full **reset** of
   ecFlowUI's storage of the tree; this is a more costly operation

You can ask for a manual refresh of the server of the currently selected
node by clicking the green **refresh** button which appears at the top
of each panel. Doing so will reset the timer for the next automatic
refresh.

When a command is sent to a server (e.g. when you Requeue a node), a
refresh is automatically performed.

There are further options in the **Refresh** menu, allowing you to
**refresh** or **reset** all servers in the current tab or just the
currently selected server.

.. image:: /_static/ecflow_ui/communication_with_ecflow_servers/image2.png
   :width: 4.64569in
   :height: 1.2211in

Refresh drift
=============

In order to help reduce server load, ecFlowUI can reduce the frequency
of its server synchronisations over time if there is no user interaction
with the server ("interaction" here means sending commands to the server
or asking for a refresh). By default, this feature is enabled for all
servers. Using the default settings, the first server refresh is
performed after 60 seconds. If there has been no user interaction during
this time, then an extra 5 seconds will be added to the refresh period.
So the next refresh will be 65 seconds later, and if again there is no
user interaction with the server then the next refresh period will be 70
seconds away. This increment will stop being added once the refresh
period has reached an hour. All these parameters are configurable from
the `Server Settings <#changing-the-server-settings>`__.

Changing the server settings
============================

There are two ways to change the server settings: the global settings
and the per-server settings. By default, the per-server settings are
linked to the global settings, so changing the global settings affects
interactions with all servers. But for each server, the settings can be
unlinked and changed individually.

.. image:: /_static/ecflow_ui/communication_with_ecflow_servers/image3.png
   :width: 0.58071in
   :height: 0.2737in

The 'chain' button controls the linking with the global settings, and
the 'arrow' button resets a value to its default (only available if the
parameter is not linked to the global setting).

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/image4.png
   :width: 2.89444in
   :height: 1.47036in

   **Global settings**: From the main menu, choose **Edit > Preferences > Server options** 

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/image5.png
   :width: 2.89444in
   :height: 2.47999in

   **Per-server settings**: from th node tree (or table) select a server node, open an Info Panel and go to the **Settings** tab

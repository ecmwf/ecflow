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
   :align: center

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
   :align: center

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

The same **Update** settings also include the **Output log refresh
period**, which controls how often the :ref:`job output panel
<viewing_scripts,_jobs_and_output>` automatically reloads the current
output file when its **Automatic reload** button is enabled.

Server protocols
================

A server is reached over one of four protocols, chosen per server in the
:ref:`Manage Servers dialogue <server_management>`:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Protocol
     - Description

   * - **TCP/IP**
     - The ecFlow protocol over a plain socket, with no encryption (The default).

   * - **TCP/IP with SSL**
     - The ecFlow protocol inside a TLS connection. Both ends must resolve the same
       self-signed certificate.

   * - **HTTP**
     - The ecFlow protocol carried over HTTP requests.

   * - **HTTPS**
     - The ecFlow protocol carried over HTTP requests, inside a TLS connection.

Both ends must be configured for the same protocol. A client set to one protocol
cannot talk to a server serving another, however healthy that server is.

.. note::

   ``ecflow_server`` does not serve HTTPS. To provide HTTPS, a reverse proxy is
   placed in front of the server, which terminates TLS and forwards plain HTTP to it.

   A server configured for **HTTPS** in ecFlowUI must therefore point at the proxy.

The connection badge
--------------------

Every server row in the tree carries a badge showing the protocol it is
configured for, so that a misconfiguration can be spotted before it causes a
failure:

.. The badge images are copies of the icons the viewer itself uses, in
   Viewer/ecflowUI/images/protocol_*.svg; keep them in step if those change.

.. list-table::
   :header-rows: 1
   :widths: 10 32 58

   * - Badge
     - Appearance
     - Protocol

   * - .. image:: /_static/ecflow_ui/communication_with_ecflow_servers/protocol_plain.svg
          :width: 0.22in
          :height: 0.22in
          :alt: Grey shield
          :align: center
     - Grey shield
     - **TCP/IP** -- no transport protection.

   * - .. image:: /_static/ecflow_ui/communication_with_ecflow_servers/protocol_ssl.svg
          :width: 0.22in
          :height: 0.22in
          :alt: Green shield with a padlock
          :align: center
     - Green shield with a padlock
     - **TCP/IP with SSL**.

   * - .. image:: /_static/ecflow_ui/communication_with_ecflow_servers/protocol_http.svg
          :width: 0.22in
          :height: 0.22in
          :alt: Dark wireframe globe
          :align: center
     - Dark wireframe globe
     - **HTTP** -- no transport protection.

   * - .. image:: /_static/ecflow_ui/communication_with_ecflow_servers/protocol_https.svg
          :width: 0.22in
          :height: 0.22in
          :alt: Green wireframe globe with a padlock
          :align: center
     - Green wireframe globe with a padlock
     - **HTTPS**.

The shield marks the ecFlow protocol and the globe marks HTTP, so the two
families are told apart by shape; green and a padlock mark an encrypted
transport.

The badge can be switched off from **Tools > Preferences >
Appearance > Tree view > Options > Show connection badge**; see
:ref:`customising_the_views`.

When communication fails
========================

Connection status in the tree
-----------------------------

When a server cannot be reached, or is not being used, its row in the tree is
tinted and a coloured band is drawn down its left edge. The colour says what
kind of problem it is:

.. list-table::
   :header-rows: 1
   :widths: 18 82

   * - Band
     - Meaning

   * - **Orange**
     - The server cannot be reached. It may be down, starting up, or unreachable
       across the network. ecFlowUI keeps trying, and recovers on its own once the
       server answers again.

   * - **Red**
     - The server is not being used, and ecFlowUI has stopped trying. Either it was
       disconnected deliberately, or it cannot be used at all: its version is
       incompatible with the client, its SSL certificate was rejected, or the client
       could not be created.

   * - **Purple**
     - The client and the server are configured for different protocols. The server
       is running and answering correctly, just not to a client configured this way,
       which is why this is not shown in the same colour as a server that cannot be
       reached.

Orange is therefore the colour to wait on, while red and purple both need action
before the server will work.

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/connection_status_in_the_tree.png
   :scale: 75%
   :align: center

   Five servers in one tree. ``server-ok`` is connected and carries no band;
   ``server-unreachable`` is marked orange, ``server-with-cert-error`` red, and
   the two misconfigured servers purple. The badge beside each name shows the
   protocol that server is configured for.

Diagnosing a protocol mismatch
------------------------------

Selecting the server and opening the **Info** panel reports the failure in full:

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/diagnosing_a_protocol_mismatch.png
   :scale: 75%
   :align: center

   The Info panel for a server ecFlowUI is set to reach over HTTP, while the
   server itself serves plain TCP/IP.

The **Client protocol** line is what ecFlowUI is configured to use. The **Server
protocol** line appears only when the protocol the server actually speaks has
been determined, by probing it after the failure; when it could not be
determined the line is omitted rather than guessed at, so anything reported
there can be relied upon.

The same diagnosis, in short form, appears in the tooltip of the server row.

To fix a mismatch, set the server to the protocol reported on the **Server
protocol** line, using the :ref:`Manage Servers dialogue <server_management>`,
or restart the server so that it serves the protocol the client expects.

.. note::

   A failure to connect is not the same as a mismatch. When nothing is listening
   on the port at all, ecFlowUI reports that the connection was refused and marks
   the server orange, rather than blaming the configuration.

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
   :align: center

The 'chain' button controls the linking with the global settings, and
the 'arrow' button resets a value to its default (only available if the
parameter is not linked to the global setting).

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/image4.png
   :width: 2.89444in
   :height: 1.47036in
   :align: center

   **Global settings**: From the main menu, choose **Edit > Preferences > Server options** 

.. figure:: /_static/ecflow_ui/communication_with_ecflow_servers/image5.png
   :width: 2.89444in
   :height: 2.47999in
   :align: center

   **Per-server settings**: from th node tree (or table) select a server node, open an Info Panel and go to the **Settings** tab

.. _server_load_panel:

Server load panel
/////////////////

The **Server load** tab in an Info panel shows some important statistics
about how the actual server is used. When clicking this tab first the
**server log file** is fetched (it is either accessed locally or
transferred via the network) then the file contents is parsed to build
and display the statistics.

Server load statistics
======================

The server load is estimated by the number of requests/commands sent to
the server within a given period. The server load statistics are based
on this measure and tries to estimate the contribution from the
different  ecFlow suites and users. ecFlow distinguishes two kinds of
command:

-  child commands: e.g. ::

    chd:label

-  user commands: e.g. ::
  
    --alter delete inlimit

Most of the commands contain a path so the **suite** can be extracted
from them. If the suite is not available a suite name **<no suite>** is
assigned to the command in the server load statistics.

.. note::

    Requests related to state changes and errors are not taken account 
    for the server load statistics.                                    

Total charts
============

The initial view shows the **total charts** with 3 graphs showing the
total number of requests sent from **all the suites**:

-  Child + User requests: contains the total number of child and user
   requests

-  Child requests: contains the total number of child requests

-  User requests: contains the total number of user requests

The actual contribution from a given suite is displayed in the sidebar
(see the percentage value next the suite name). By clicking on suite in
the sidebar the suite specific graphs are added to the charts.


.. image:: /_static/ecflow_ui/server_load_panel/image1.png
   :width: 3.35693in
   :height: 2.60417in

.. image:: /_static/ecflow_ui/server_load_panel/image2.png
   :width: 3.76327in
   :height: 2.60417in


Other charts
============

The **Other charts** tab contains some more refined statistics with 4
different chart types:

Command graph per suite
-----------------------

It shows how the different **commands are used for a given suite**. The
sidebar contains two tabs: one listing the suites and another listing
the commands. The view displays **one chart per each suite** and there
is a another chart showing the statistics for all the suites (it always
appears at the top). By default the command used the most is selected.


.. image:: /_static/ecflow_ui/server_load_panel/image3.png
   :width: 3.75106in
   :height: 2.60417in

.. image:: /_static/ecflow_ui/server_load_panel/image4.png
   :width: 3.75106in
   :height: 2.60417in


Suite graph per command
-----------------------

It shows how the different **suites contribute to a given command**. The
sidebar contains two tabs: one listing the suites and another listing
the commands. The view displays **one chart per each command** and there
is a another chart showing the statistics for all the commands (it
always appears at the top). By default the suite contributing the most
is selected.


.. image:: /_static/ecflow_ui/server_load_panel/image5.png
   :width: 3.72999in
   :height: 2.60417in
 
.. image:: /_static/ecflow_ui/server_load_panel/image6.png
   :width: 3.76708in
   :height: 2.60417in


User graph per command
----------------------

It shows how the different **users contribute to a given command**. The
sidebar contains two tabs: one listing the users and another listing the
commands. The view displays **one chart per each command** and there is
a another chart showing the statistics for all the commands (it always
appears at the top). By default the user contributing the most to all
the commands is selected.


.. image:: /_static/ecflow_ui/server_load_panel/image7.png
   :width: 3.73213in
   :height: 2.60417in
 
.. image:: /_static/ecflow_ui/server_load_panel/image8.png
   :width: 3.76455in
   :height: 2.60417in


Command graph per user
----------------------

It shows how the different **commands are used by a given user**. The
sidebar contains two tabs: one listing the users and another listing the
commands. The view displays **one chart per each user** and there is a
another chart showing the statistics for all the users (it always
appears at the top). By default the command used the most is selected.


.. image:: /_static/ecflow_ui/server_load_panel/image9.png
   :width: 3.76157in
   :height: 2.60417in

.. image:: /_static/ecflow_ui/server_load_panel/image10.png
   :width: 3.74088in
   :height: 2.60417in


Tables
======

The **Tables tab** contains the very same information as the Other
charts tab but in a tabular form.

Interaction with the charts and other controls
==============================================

Zoom
----

Use the mouse to define a zoom rectangle. When the mouse is released the
zoom is automatically performed.

.. image:: /_static/ecflow_ui/server_load_panel/image11.png
   :width: 3.27691in
   :height: 1.5625in

Show full period
----------------

To show the full period again use the

.. image:: /_static/ecflow_ui/server_load_panel/image12.png
   :width: 0.23958in
   :height: 0.21875in

button.

Callout
-------

Click in the chart with the **Middle or Right mouse button** to get a
callout for the local maximum value. To remove the callouts left click
anywhere outside a chart plot area.

.. image:: /_static/ecflow_ui/server_load_panel/image13.png
   :width: 2.45833in
   :height: 1.92708in

Log entries
-----------

To see the log entries use the

.. image:: /_static/ecflow_ui/server_load_panel/image14.png
   :width: 0.30208in
   :height: 0.28125in

button. If there is a callout defined the corresponding log entries are
highlighted in the log entry view.

.. image:: /_static/ecflow_ui/server_load_panel/image15.png
   :width: 4.57036in
   :height: 2.60417in

Temporal resolution
-------------------

The temporal resolution of the charts can be controlled via the **Plot
resolution** combo box:

.. image:: /_static/ecflow_ui/server_load_panel/image16.png
   :width: 1.88542in
   :height: 0.94792in

Server load config options
--------------------------

There a some configuration options available for the server load panel
at Tools > Preferences > Server settings > Server load and timeline.
These settings can be applied globally or can be defined per server.

.. image:: /_static/ecflow_ui/server_load_panel/image17.png
   :width: 3.44845in
   :height: 1.5625in

Inspecting older log files
==========================

By default the Server load panel fetches the current log file. To work
with other (e.g. older) log files switch to the **Archived log** mode :

.. image:: /_static/ecflow_ui/server_load_panel/image18.png
   :width: 1.58333in
   :height: 0.65625in

and load a single (or multiple) log file(s) using the file load button:

.. image:: /_static/ecflow_ui/server_load_panel/image19.png
   :width: 1.3125in
   :height: 0.47917in

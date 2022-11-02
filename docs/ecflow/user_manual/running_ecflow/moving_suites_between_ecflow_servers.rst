.. _moving_suites_between_ecflow_servers:

Moving suites between ecFlow servers
////////////////////////////////////


When testing a suite, you may want to initially run on a test server.
Once operational you may then wish to move the suite onto another
server. Whilst you can replay the suite on the new server you can also
use the CLI swap command to move the suite. However, a more simple
method is to use ecflow_ui to **move** the suite into a new server.

To do this in ecflow_ui: right mouse button > Special > Mark for Move,
The suite you want to move.

.. image:: /_static/ug/moving_suites_between_ecflow_servers/image1.png
   :width: 3.65674in
   :height: 4.16667in

Then select the node where you want to move:  right mouse button >
Special > Move marked node here

.. image:: /_static/ug/moving_suites_between_ecflow_servers/image2.png
   :width: 2.80208in
   :height: 4.16667in

.. _stopping_ecflow_servers:

Stopping ecFlow servers
///////////////////////


To safely stop an ECFLOW server you should "halt", "checkpoint" and then
"terminate" the server. This can be done either through GUI (right-click
on the server) or directly by the CLI commands "halt", "check_pt" and
"terminate".

.. code-block:: shell

    # halt server, write out the in memory definition as a check*
    # point file, then terminate the server*
    # The 'yes' bypasses the prompt
    ecflow_client --group="halt=yes; check_pt; terminate=yes"  
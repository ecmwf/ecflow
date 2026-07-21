.. _using_ecflowui_via_the_ecmwf_teleport_gateway:

Using ecFlowUI via the ECMWF Teleport gateway
/////////////////////////////////////////////


The native ecFlowUI client can run on an End User device to access
remote servers via an SSH tunnel. The recommended approach uses dynamic
port forwarding.

.. warning::

    This requires at least ecFlow version 5.7.0, with some advanced features available only in version **5.9.0**.

.. note::

    ecFlowUI can be installed on MacOS platforms using
    :ref:`brew`, or the full ecFlow from :ref:`conda-forge`.

Dynamic port forwarding
=======================

Authenticate via Teleport
-------------------------

The first step is to authenticate via
`Teleport <https://confluence.ecmwf.int/display/UDOC/Teleport+SSH+Access>`__
on the End User device.

Set up dynamic port forwarding
------------------------------

The next step is to start dynamic port forwarding using the SOCKS
(Secure Socket) protocol. Assuming the target host is
``hpc-login``, type the following command in a terminal:

.. code-block:: bash

    ssh -v -C -N -D 9050 <user>@hpc-login
        # -v enables verbose output
        # -C enables data compression
        # -N disables opening a shell on the remote host
        # -D 9050 enables dynamic port forwarding on local port 9050
        # Adjust `<user>` to match the username on the remote host

This will open an SSH connection with dynamic port forwarding enabled,
from local port 9050 to the target host (hpc-login), that must be kept
running while ecFlowUI is used. Keeping the command in the foreground is
recommended, so that its status can be monitored.

This assumes `Teleport <https://confluence.ecmwf.int/display/UDOC/Teleport+SSH+Access>`_
has been configured correctly as described in the ECMWF User Documentation.

The port does not have to be 9050, but this is the default port used by
the proxychains tool (see below), so it is the most convenient option.

Run ecFlowUI via proxychains
----------------------------

Applications that use dynamic port forwarding must speak the SOCKS
protocol. ecFlowUI can be made to use the SOCKS protocol with a third party
tool called `proxychains <https://github.com/rofl0r/proxychains-ng>`__.

.. important::

    This is only possible on macOS platforms.

    Unfortunately, Linux platforms are *not* supported, because proxychains
    does not intercept the ``epoll_*`` system calls used by ecflow clients
    to communicate with the ecflow servers.

Installing proxychains
~~~~~~~~~~~~~~~~~~~~~~

The proxychains tool must be installed on the End User device.
On macOS, this can be done with ``brew install proxychains-ng``.

Start ecFLowUI
~~~~~~~~~~~~~~

Start ecFlowUI with the following command (when using version **>= 5.9.0**):

.. code-block:: bash

    ecflow_ui -pc4

Otherwise, use the ``-cmd`` flag:

.. code-block:: bash

    ecflow_ui -cmd proxychains4

When using the above options, ecFlowUI behaves (in terms of client/server
communication) as if it were running on the remote host, allowing
interaction with all the ecFlow servers available from that host.

Local file access via proxychains (only in version >= 5.9.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ecFlowUI accesses certain local files (e.g. output and server logs in
the **Output**, **Timeline** and **Server Load** panels) directly
(standard file I/O) without using the ecFlow client-server
communication. This poses a limitation when it runs via proxychains,
because these files are only local on the remote hosts, so ecFlowUI
cannot access them. To overcome this difficulty, ecFlowUI
**version >= 5.9.0** is required and file fetching then works
automatically. The only setting that might need to change is the SOCKS
port number, which by default is assumed to be 9050. When dynamic port
forwarding and proxychains are set up with a different port number,
set it in **Tools > Preferences > Network**:

.. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image1.png
   :width: 3.33333in
   :height: 2.38095in

System server lists via proxychains (only in version >= 5.9.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using ecflowUI outside ECMWF, the :ref:`system server list
files <system_server_list_files_in_ecflowui>`
are not automatically accessible and must be configured
manually. However, once this is done, they are automatically transferred
from the remote SOCKS host and loaded on each start-up. To define the
system server lists, probably the best option is to set **Tools >**
**Preferences > Server settings > System server list.** The paths
specified here have to be local paths on the remote SOCKS host.

.. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image2.png
   :width: 4.16667in
   :height: 1.91879in

Alternatively, the **ECFLOW_SYSTEM_SERVERS_LIST**
environment variable can be used to specify the very same paths.

.. note::

    The path to the system server list files for ATOS can be found
    `here <https://confluence.ecmwf.int/display/ECFLOW/The+central+ecFlow+server+list+file+on+ATOS>`__.

Comments
========

-  In case the dynamic port forwarding stops/hangs, the ecFlowUI
   loses connection to the servers, as indicated by the orange strip
   on the left and the dotted background:

   .. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image3.png
      :width: 2.08333in
      :height: 1.19454in

   If this happens, go to the terminal where the dynamic port
   forwarding was started, terminate it if it is still running, and run
   the command again. Exiting ecflowUI is not necessary; just refresh the
   servers and the connection is re-established.

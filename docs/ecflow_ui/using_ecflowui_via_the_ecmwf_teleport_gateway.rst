.. _using_ecflowui_via_the_ecmwf_teleport_gateway:

Using ecFlowUI via the ECMWF Teleport gateway
/////////////////////////////////////////////


You can use the native ecFlowUI client on your End User device to access
remote servers via an SSH tunnel. The recommended way is using dynamic
port forwarding.

.. warning::

    This will require at least ecFlow version 5.7.0. Some advanced     
    features are only available in version **5.9.0**.                  

.. note::

    You can install ecFlowUI on MacOS platforms using                  
    :ref:`Brew <brew>`, or    
    the full ecFlow from                                               
    `conda-forge <https://confluence.ecmwf.int/display/ECFLOW/Conda-forge>`__. 

Dynamic port forwarding
=======================

Authenticate via teleport
-------------------------

First you need to authenticate via
`Teleport <https://confluence.ecmwf.int/display/UDOC/Teleport+SSH+Access>`__
on your End User device.

Set up dynamic port forwarding
------------------------------

The next step is to start dynamic port forwarding using the SOCKS
(Secure Socket) protocol. Let us suppose the target host you want to
access is hpc-login. In a terminal on your End User device type::

    ssh -v -C -N -D 9050 -J myecuser@jump.ecmwf.int myecuser@hpc-login

and keep it running. This will forward all network traffic from local
port 9050 to the target SOCKS host (hpc-login). The proxy jump option
(-J) was chosen according to the
`Teleport <https://confluence.ecmwf.int/display/UDOC/Teleport+SSH+Access>`__
setup. The port does not have to be 9050, but this is the default port
used by the proxychains tool (see below) so it is the most convenient
option for us.

Run ecFlowUI via proxychains
----------------------------

Applications which want to use dynamic port forwarding must speak SOCKS
protocol. ecFlowUI can be SOCKS-ified using a third party tool called
`proxychains <https://github.com/rofl0r/proxychains-ng>`__.

Installing proxychains
~~~~~~~~~~~~~~~~~~~~~~

You need to install proxychains in your  End User device. On MacOS, you
can do it with "brew install proxychains-ng". Some Linux distributions
come with proxychains tool pre-installed.

Start ecFLowUI
~~~~~~~~~~~~~~

Start ecFlowUI with this command if you are using version **>= 5.9.0**::

    ecflow_ui -pc4                                                     

otherwise you need to use the -cmd flag::

    ecflow_ui -cmd proxychains4                                        

ecFlowUI is now behaving (in terms of client/server communication) as if
it were running on the SOCKS host (hpc-login in this case) and you
should be able to interact with all the ecFlow servers available from
that host.

Local file access via proxychains (only in version >= 5.9.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ecFlowUI accesses certain local files (e.g. output and server logs in
the **Output**, **Timeline** and **Server Load** panels) directly
(standard file I/O) without using the ecFlow client-server
communication. This poses a limitation if we run it via proxychains
because these files are only local on the remote hosts so ecFlowUI
cannot access them. To overcome this difficulty you need to use ecFlowUI
**version >= 5.9.0** and file fetching will automatically work for you.
The only settings that you might need to change is the SOCKS port
number, which by default is assumed to be 9050. Should you set up
dynamic port forwarding and use proxychains with a different port number
you need to set it in **Tools > Preferences > Network**:

.. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image1.png
   :width: 3.33333in
   :height: 2.38095in

System server lists via proxychains (only in version >= 5.9.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using ecflowUI outside ECMWF the :ref:`system server list
files <system_server_list_files_in_ecflowui>`
are not automatically accessible and you need to configure them
manually. However, when it is done they will automatically transferred
from the remote SOCKS host and loaded on each start-up. To define the
system server lists probably the best option is to set **Tools >**
**Preferences > Server settings > System server list.** The paths
specified here have to be local paths on the remote SOCKS host.  

.. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image2.png
   :width: 4.16667in
   :height: 1.91879in

Alternatively, you can use the **ECFLOW_SYSTEM_SERVERS_LIST**
environment variable to specify the very same paths.

.. note::

    The path to the system server list files for ATOS can be found     
    `here <https://confluence.ecmwf.int/display/ECFLOW/The+central+ecFlow+server+list+file+on+ATOS>`__. 

Comments
========

-  the dynamic port forwarding sometimes stops/hangs and as a result
   ecFlowUI loses connection to the servers (it is indicated by the
   orange strip on the left and the dotted background):

   .. image:: /_static/ecflow_ui/using_ecflowui_via_the_ecmwf_teleport_gateway/image3.png
      :width: 2.08333in
      :height: 1.19454in

   If it happens just go to the terminal where the dynamic port
   forwarding was started up, terminate it if it still running and run
   the command again. You do not need to exit ecflowUI, just refresh the
   servers and the connection will be re-established.

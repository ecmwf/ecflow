.. _system_server_list_files_in_ecflowui:

System server list files in ecFlowUI
////////////////////////////////////

.. note::

    This documentation is only valid for versions **>= 5.9.0**.        

System server list
==================

ecFlowUI can make use of multiple externally managed **system server
lists**. A system server list is just a text file where each row is
either a path to another system server list file or specifies a server
(name, host and port with a space separator)::

    path_to_another_system_server_list_file1
    name1 host1 port1                                             
    name2 host2 port2                                               
    ...                                                                

Servers from these lists are marked as "system" in ecFlowUI and
represented by a different colour and icon decoration (in column "S") in
the **Manage servers** dialogue (see server "demo2" in the snapshot
below).

.. image:: /_static/ecflow_ui/system_server_list_files_in_ecflowui/image1.png
   :width: 3.60417in
   :height: 0.60417in

.. warning::

    Please note that servers marked as "system" **cannot be modified   
    or removed**. Instead they are managed by an automatic update      
    process (see below).                                               

Specifying the system server lists
==================================

A colon separated list of system server list files can be defined via
the **ECFLOW_SYSTEM_SERVERS_LIST** environment variable. At ECMWF this
is pre-set in the ecFlowUI start-up script on installation so you do not
need to set it. However, if you define it it will overwrite the pre-set
value (if there is any).

It is also possible to specify this list of paths in ecFlowUI in
**Preferences > Server > System server list**. Any paths here will
**overwrite** the ones defined via the environment variable! Having set
this value and pressing **Apply/Ok** the system server list
synchronisation (see below) is automatically performed. The advantage of
using it over the environment variable is that it needs to be set only
once! Probably this is the best option when you access ECMWF servers
remotely via proxychains (:ref:`see
here <using_ecflowui_via_the_ecmwf_teleport_gateway>`).

.. image:: /_static/ecflow_ui/system_server_list_files_in_ecflowui/image2.png
   :width: 4.16667in
   :height: 1.91879in

You can always check the current value of ECFLOW_SYSTEM_SERVERS_LIST
before starting up ecFlowUI with the following command::

    ecflow_ui -h                                                       

However, it does not show what has been set in the ecFlowUI Preferences.

Getting updates from the system server list
===========================================

On *each startup* ecFlowUI checks the system server lists and updates
its local server list with it. This update process uses the following
algorithm:

-  If a server in the system list does not appear in the local list, it
   is **added** to the local list and marked as *system*.

-  If a server in the system list does appear in the local list but the
   host or port do not match, the **local version will be updated**.

-  If a server is marked as "system" in the local list but does not
   appear in the system list it will be **removed**!

If any changes happen during the update process an indicator icon will
appear in the left hand side of the statusbar:

.. image:: /_static/ecflow_ui/system_server_list_files_in_ecflowui/image3.png
   :width: 1.70987in
   :height: 0.89477in

Clicking on this icon will start up the **Manage servers** dialogue
showing the *change log* of the update process:

.. image:: /_static/ecflow_ui/system_server_list_files_in_ecflowui/image4.png
   :width: 3.64583in
   :height: 2.95143in

Closing this dialogue will make the statusbar notification disappear but
the change log can still be viewed in the **Manage servers** dialogue
any time by using the the **Show update log** button at the top of the
dialogue:

.. image:: /_static/ecflow_ui/system_server_list_files_in_ecflowui/image5.png
   :width: 0.33333in
   :height: 0.26314in

When no changes happen during the update this button is disabled.

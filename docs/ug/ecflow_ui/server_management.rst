.. _server_management:

Server management
/////////////////

The local server list
=====================

ecFlowUI maintains a local server list. This is populated when you first
start ecFlowUI by importing an existing server list from ecFlowview;
from then on, the two lists are not synchronised.

Loading servers from the menu
=============================

The **Servers** menu provides quick access to all your *favourite*
servers and the ones already loaded in the current tab. It also makes
the complete list of servers available in a submenu.

.. image:: /_static/ecflow_ui/server_management/image1.png
   :width: 3.32296in
   :height: 2.2422in

Selecting a server from the menu will add it to any tree or table views
in the current tab. For large servers, a **Loading** indicator may
appear, but the user interface is still responsive.

.. image:: /_static/ecflow_ui/server_management/image2.png
   :width: 3.67784in
   :height: 1.84218in

The Manage Servers dialogue
===========================

Servers can be fully managed (added, removed, renamed etc) by using the
**Manage servers** dialogue. You can start it up from the toolbar icon

.. image:: /_static/ecflow_ui/server_management/image3.png
   :width: 0.33333in
   :height: 0.33934in

or from the **Servers** menu or using the Ctrl+N short-cut.

.. image:: /_static/ecflow_ui/server_management/image4.png
   :width: 5.25813in
   :height: 2.60417in

This dialogue is also where servers can be set as *favourites* for easy
access.

.. note:: 
                                                       
    A server is identified by its name, host and port. A server name   
    is just an alias that you are free to customise. The only          
    requirement is that each server **must have a unique name**.       

System server list
==================

ecFlowUI can make use of a shared (potentially centrally managed) server
list. The details can be found on the following pages:

-  for versions >= 5.9.0: :ref:`System server list files in
   ecFlowUI <system_server_list_files_in_ecflowui>`

-  for earlier versions: `System server list files in ecFlowUI (pre
   5.9.0
   versions) <https://confluence.ecmwf.int/pages/viewpage.action?pageId=293717964>`__

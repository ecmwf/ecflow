.. _viewing_dependencies:

Viewing dependencies
////////////////////

Dependencies can be created between nodes by using triggers, limits,
date and time attributes, and can be shown in the tree and table views
as attributes.

A more advanced way to see dependencies is to use the *Triggers* tab in
the :ref:`Info
Panel <getting_started_with_ecflowui>`.
This provides detailed information about the dependencies related to the
currently-selected node.

.. image:: /_static/ecflow_ui/viewing_dependencies/image1.png
   :width: 4.16667in
   :height: 3.3701in

The *Triggers* tab consists of five sub-panels. These are as follows:

-  At the top the **trigger expression** of the node (if available) is
   displayed. It can be hidden/shown with the **Expression** button.

-  Just below this, the **currently-selected node** is visible.

-  The area below the currently-selected node is divided into two lists
   showing

   -  the **triggers of** the selected node (on the left)

   -  the nodes **triggered by** the selected node (on the right)

-  When we click on an item in either list the **details about the
   dependencies** will be displayed in textual format at the bottom of
   the interface. This part can be shown/hidden with
   the

      .. image:: /_static/ecflow_ui/viewing_dependencies/image2.png
         :width: 0.29036in
         :height: 0.26317in

   button in top-right corner of the Triggers tab.

The **triggers of** and **triggered by** lists form the core of the
Triggers panel so we will explain their usage in detail.

.. note::

    **Updated states and interaction**
                                    
    The graphical representation of the nodes and attributes appearing 
    in the trigger lists are the same as in the tree view. The states  
    of these items are updated with each server sync and they come     
    with a large set of actions in their context menu.                 

.. note::

    **Broadcast selection**

    *Double click* on a node/attribute in the trigger list or run      
    action '*Lookup in tree*' from the context menu to broadcast this  
    selection to the other views, e.g. to make it selected in the tree 
    view.The dependency details list contains textual information with      
    *hyperlinks*: when you click on a path the selection will be       
    broadcast to the other views.                                      

Triggers of the selected node
=============================

The list on the left shows the **triggers of** the currently-selected
node.

.. image:: /_static/ecflow_ui/viewing_dependencies/image3.png
   :width: 2.60417in
   :height: 2.08333in

A *direct trigger* can be

-  a node or attribute that appears in the selected node's trigger
   expression

-  a limit that the currently-selected node consumes

-  a date or time attribute of the currently-selected node

Direct triggers are displayed with a white background. For example, in
our snapshot the first four items (the generated variables /eda/main:YMD
and /eda/lag:YMD, and the nodes an and fc) are direct triggers because
they all appear in the selected node's trigger expression, which reads
as::

    (/eda/main:YMD gt /eda/lag:YMD) or (/eda/main:YMD eq /eda/lag:YMD  
    and /eda/main/12/an==complete and /eda/main/12/fc==complete)       

When the **dependencies** are enabled (using the **Dependencies** button
at the top-right corner of this panel) an additional set of triggers
will be shown with a grey background. A node or attribute is regarded as
a *trigger through dependency* when it

-  directly triggers a parent of the currently-selected node

-  directly triggers a child of the currently-selected node and it is
   not an ancestor of it

To find out more about a trigger through dependency we can click on it
to see its dependency details list. For example if we click on limit
/eda/limits:mars we get these dependency details:

.. image:: /_static/ecflow_ui/viewing_dependencies/image4.png
   :width: 5.20833in
   :height: 1.125in

In this example, the first line tells us that /eda/limits:mars triggers
the node /eda/lag/12/archive/ansfc, which is the child of the currently
selected node (/eda/lag/12/archive).

Nodes triggered by the selected node
====================================

The list on the right shows the *nodes* that are **triggered by** the
currently-selected node.

.. image:: /_static/ecflow_ui/viewing_dependencies/image5.png
   :width: 3.14583in
   :height: 1.15694in

A node is *directly triggered* when the currently-selected node appears
in its trigger expression, and is displayed with a white background. For
example, in our snapshot the first item (node /eda/lag/12/clean) is
directly triggered by the currently-selected node (/eda/lag/12/archive)
because the former node's trigger expression reads as::

    fb == complete and archive == complete                             

When the **dependencies** are enabled an additional set of triggered
nodes will be shown with grey background. A node is regarded as a
*triggered through dependency* when either a parent or a child of the
currently-selected node triggers it.

To find out more about a node triggered through dependency, we need to
click on it and check its dependency details list. For example, if we
click on node /eda/lag/logfiles these dependency details will be listed:

.. image:: /_static/ecflow_ui/viewing_dependencies/image6.png
   :width: 4.375in
   :height: 0.5365in

This tells us that /eda/lag/12, which is the parent of  the currently
selected node (/eda/lag/12/archive), directly triggers
/eda/lag/logfiles. The trigger expression of /eda/lag/logfiles verifies
this fact::

    ./00 == complete and ./12 == complete and /eda/main:YMD gt         
    /eda/lag:YMD                                                       

.. image:: /_static/ecflow_ui/viewing_dependencies/image2.png
   :width: 0.29036in
   :height: 0.26317in

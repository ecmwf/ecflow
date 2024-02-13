.. _viewing_and_editing_variables:

Viewing and editing variables
/////////////////////////////


The :ref:`Info
Panel <getting_started_with_ecflowui>`
contains a *Variables* tab, providing a powerful place for viewing and
editing the variables connected to the currently-selected node.

The panel is divided into sections, one for each level in the hierarchy
from the server to the current node. Each node in this hierarchy has its
own set of variables, and these are inherited by the selected node; this
means that all the variables visible in the panel are active for the
current node (with the exception of *shadowed* variables, see below).

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image1.png
   :width: 5.20833in
   :height: 4.9905in

Variable types
==============

-  Variables defined in the suite definition are shown in black

-  Generated variables are shown in green and have a 'G' icon next to
   them

-  Shadowed variables (see below) are shown in grey and can be hidden

-  Read-only variables have a padlock icon next to them

Hovering the mouse cursor over a variable displays a small tooltip which
also gives this information.

Finding variables
=================

The controls at the top of the panel provide two different ways to find
variables.

**Filter mode** reduces the list of variables shown, displaying only
those whose name or value match the filter term.

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image2.png
   :width: 5.20833in
   :height: 1.47285in

**Search mode** shows the whole list of variables, but highlights those
that match the search term.

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image3.png
   :width: 5.20833in
   :height: 2.03287in

Shadowed variables
==================

A variable is called *shadowed* if it is redefined deeper in the node
hierarchy. For example, it could be defined at the family level, and
then again in a task within that family; in this case, the variable
defined in the family is 'shadowed' by the one in the task and will not
be used. Shadowed variables are displayed in grey. They can be shown or
hidden with the 'V V' button in the Variables panel. By default,
shadowed variables are visible in the panel.

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image4.png
   :width: 5in
   :height: 1.96705in

In the above screenshot, the Family1 and Family variables are taken from
the **analysis** level, not the **0** level, as the **analysis** node is
closer in the hierarchy to the selected node.

Adding, deleting and modifying variables
========================================

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image5.png
   :width: 3.125in
   :height: 1.53711in

Adding a variable
-----------------

To add a new variable, either click the green 'plus' button or use the
context menu. Note that the new variable *will always be added to the
selected node*, even if you right-click within the variables section
belonging to a parent in the hierarchy.

Deleting a variable
-------------------

To delete a variable, either click the red 'X' button or use the context
menu. Note that it is only possible to delete variables from the
currently-selected node. You cannot delete variables from ancestors in
the hierarchy; to do this, you must first select their node in the tree
(or use the breadcrumbs).

Editing a variable
------------------

To edit a variable, either select it and then click the 'pencil and
paper' icon, or right-click the variable and choose 'Edit variable' from
the context menu. A shortcut is to double-click the variable. A simple
dialogue allows for the modification of the variable's value:

.. image:: /_static/ecflow_ui/viewing_and_editing_variables/image6.png
   :width: 4.11875in
   :height: 2.16851in
|
.. note::
    ecFlow allows the definition of variable values with multi-line content.

    However, both ecFlowUI and ecFlow CLI client display the content of
    multi-line values replacing new line characters with :code:`\\n` (n.b. when
    using Japanese locale, :code:`\\n` might be displayed as :code:`¥n`).

    When performing the job script generation, ecFlow uses the actual value
    of the variable including actual new line characters.

If the name of the variable is changed, a new variable will be created
and the old one will remain.

It is important to understand what happens when editing a variable
inherited from higher in the hierarchy. In this case, the ancestor node
is not modified, but a duplicate of its variable is created at the level
of the selected node.

The following example shows what happens. Here, the node
"**get_observations**" is selected and we will change a variable
inherited from one of its parents, "**0**".

1. Here, the variable tree has been collapsed in order to show just the relevant parts. We've elected to modify the variable LAST_STEP. This is inherited from a node (0) higher in the hierarchy and does not exist locally in the selected node (**get_observations**). As the dialogue indicates, we will be modifying the **get_observations** node, not the 0 node

   .. image:: /_static/ecflow_ui/viewing_and_editing_variables/image7.png
      :width: 300px

2. A confirmation box explains that a new variable called LAST_STEP will be created for the **get_observations**task, and the original one, on node **0**, will now be shadowed.

   .. image:: /_static/ecflow_ui/viewing_and_editing_variables/image8.png
      :width: 300px

3. The result is shown. LAST_STEP is now local to **get_observations**, and the version in node **0** is shadowed, i.e. not inherited by **get_observations**.

   .. image:: /_static/ecflow_ui/viewing_and_editing_variables/image9.png
      :width: 300px

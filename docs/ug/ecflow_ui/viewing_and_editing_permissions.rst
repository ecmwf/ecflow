.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: permissions (ecFlowUI)

.. _viewing_and_editing_permissions:

Viewing and editing permissions
///////////////////////////////

On a server using :ref:`Permissions attributes <permissions_attribute>`, ecFlowUI shows and changes the
:term:`permissions` attributes of the nodes, and of the server. Every change is validated by the server, as for
``ecflow_client --permissions``; a change requires the role ``p`` at the node. The complete reference is in
:ref:`permissions_attribute` (section ecFlowUI).

Viewing the attribute
=====================

- The attribute of a node is shown as a row among its attributes, with its entries in the command-line form, e.g.
  ``alice:rwxp,bob:r``; its tooltip lists one entry per line.
- The entry "Permissions" of the attribute filter shows or hides these rows; new tree views show them by default.
- The server attribute is not shown as a row of the tree.
- The Info panel of the server and of every node shows the line ``permissions :``, listing the effective roles of
  every user there, e.g. ``admin:rwxlop, alice:rwxp, bob:r``; the users without any role there are not listed.

Changing the attribute
======================

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Action
     - How
   * - Add an attribute to a node without one
     - "Add permissions" of the "Special" menu, in a dialog holding one entry per line
   * - Edit the attribute of a node
     - "Edit ..." on the row of the attribute
   * - Delete the attribute of a node
     - "Delete" on the row of the attribute, after a confirmation; the node then inherits its roles from above
   * - Edit the server attribute
     - "Edit Permissions ..." of the context menu of the server; the server attribute is never added nor deleted
       from ecFlowUI

A change rejected by the server (for example one that leaves nobody holding ``p``) is reported like any other failed
command, and the attribute is left unchanged.

Searching
=========

The search dialog finds the attribute ``permissions`` by its value or by its users, and the condition
``has_permissions`` selects the nodes carrying one (see :ref:`searching_for_nodes`).

When access is lost
===================

When the permissions change during a session:

- the nodes that can no longer be read disappear, with the panels open on them;
- when the user loses the role ``r`` at server level, the server denies the refresh itself: the tree and its panels
  are cleared, and the server is shown as lost with the reason;
- ecFlowUI synchronises in full once access returns.

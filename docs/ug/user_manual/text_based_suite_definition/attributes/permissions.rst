.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: permissions (text based definition)

.. _text_based_def_permissions:

permissions
///////////

.. caution::
  The Permissions attribute is experimental, and its details are subject to change. Users are encouraged to try it
  and provide feedback, but it is **not to be used in production** (see :ref:`permissions_attribute`).

This defines a :term:`permissions` attribute, which grants roles on a :term:`node` to the users it lists. A suite,
a family, a task or an alias can carry one. The attribute takes effect only on a server that has a server
Permissions attribute; the rules are described in detail in :ref:`permissions_attribute`.

.. code-block:: shell

    suite team_a
      permissions [{"user": "alice", "permissions": "rwxp"}, {"user": "andy", "permissions": "rwx"}, {"user": "ops1", "permissions": "rx"}]
      family forecast
        task run
      endfamily
      family archive
        # restricts the roles inherited from the suite: andy is not listed, and has no role here
        permissions [{"user": "alice", "permissions": "rwxp"}, {"user": "ops1", "permissions": "r"}]
        task store
      endfamily
    endsuite

The keyword ``permissions`` is followed by a JSON list of entries, each an object with the following members:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Member
     - Description
   * - :code:`user`
     - The user name (see :ref:`permissions_attribute_users`).
   * - :code:`permissions`
     - The letters of the roles granted, in any order, each at most once:

       - :code:`r` read
       - :code:`w` write
       - :code:`x` execute
       - :code:`l` log
       - :code:`o` owner
       - :code:`p` permissions

       The letter :code:`s` makes the entry sticky; it grants nothing by itself. The commands that require each
       role are listed in :ref:`permissions_attribute_commands`.

.. important::

   - A node carries at most one :term:`permissions` attribute.
   - An invalid attribute makes the definition invalid, and the definition fails to load: an empty list, an entry
     whose members are not exactly the strings :code:`user` and :code:`permissions`, an unknown or repeated letter,
     an entry granting no role, a user listed twice, or an invalid user name.
   - A definition cannot set the server attribute: a :code:`permissions` line outside any suite is rejected. The
     server attribute is set in the :ref:`server configuration file <permissions_attribute_server>`.
   - The attribute lists every user it grants something to: a user it does not list has no role from that node
     down, unless the entry of that user above is sticky.

Loading the definition
======================

The definition is checked against the server when it is loaded (``ecflow_client --load``, or ``--replace``), as
described in :ref:`permissions_attribute_changes`:

- on a server without a server attribute, a definition carrying :term:`permissions` attributes is rejected;
- every user named by the attribute of a suite must be listed by the server attribute, and every user named below a
  suite must hold a role at the parent node;
- loading a suite carrying :term:`permissions` attributes requires the role :code:`o` at the server level, and the
  role :code:`p` at the suite: for a suite not yet loaded, at the server level (see
  :ref:`permissions_attribute_commands`).

In Python, :py:meth:`ecflow.Defs.check_permissions` performs the same checks against a given server attribute,
before the definition is loaded.

Effective permissions
=====================

The roles a user holds on a node follow from the attributes met on the path from the server down to the node: the
attribute of a suite replaces the server entry of the user, while the attributes below a suite can only restrict the
inherited roles. See :ref:`permissions_attribute_effective`.

Changing the attribute
======================

The attribute of a node is changed at run time, by a user holding the role :code:`p` there, with:

- :ref:`permissions_cli` (``create``, ``read``, ``update``, ``delete``, ``effective``), and not with
  :ref:`alter_cli`;
- the Python methods :py:meth:`ecflow.Client.create_permissions`, :py:meth:`ecflow.Client.update_permissions` and
  :py:meth:`ecflow.Client.delete_permissions`;
- ecFlowUI, see :ref:`viewing_and_editing_permissions`.

Compatibility
=============

The attribute is available from ecFlow 5.20.0. A server using Permissions attributes requires clients of version
5.20.0 or later, and its checkpoints cannot be loaded by older servers; see
:ref:`permissions_attribute_compatibility`.

.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _permissions_attribute:

Permissions attribute
/////////////////////

.. caution::
  The Permissions attribute is experimental, and its details are subject to change. Users are encouraged to try it
  and provide feedback, but it is **not to be used in production**. The documentation reflects its current operation.

The ``permissions`` attribute lists users and the roles each of them holds. The server can carry one, configured
in ``<host>.<port>.ecf.cfg``, and so can any node: suites, families, tasks and aliases.

Roles and flag
==============

Each entry of the attribute grants one or more roles, identified by a letter:

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Letter
     - Role
     - Purpose
   * - ``r``
     - read
     - Read nodes and their attributes.
   * - ``w``
     - write
     - Change nodes and their attributes.
   * - ``x``
     - execute
     - Run, requeue and otherwise act on nodes.
   * - ``l``
     - log
     - Change the server log.
   * - ``o``
     - owner
     - Load and delete suites.
   * - ``p``
     - permissions
     - Change ``permissions`` attributes.

An entry may also carry the flag ``s`` (sticky), which grants nothing by itself.

Forms
=====

The attribute has two textual forms, which describe the same value:

- the command-line form, a comma-separated list of ``<user>:<letters>`` entries:

  .. code-block:: none

     alice:rwxops,bob:r

- the JSON form, used in definition files, checkpoints and configuration files:

  .. code-block:: none

     [{"user": "alice", "permissions": "rwxops"}, {"user": "bob", "permissions": "r"}]

The letters are lowercase, in any order, each at most once. The attribute is always printed with the entries
sorted by user, and with the letters in the order ``r w x l o p s``. In definition files, the keyword
``permissions`` precedes the JSON form (see :ref:`attr-permissions`).

Rules:

- the attribute holds at least one entry;
- each user is listed at most once;
- each entry grants at least one role (an entry holding only ``s`` is invalid);
- an invalid value is rejected wherever it is given, never ignored or replaced.

.. _permissions_attribute_effective:

Effective permissions
=====================

The roles a user holds on a node, its *effective permissions*, follow from the attributes met on the path from the
server down to the node. They apply only when the server has a Permissions attribute; otherwise, the server is
authorised as without Permissions (by the white list file, when one is configured, and without restriction
otherwise).

The rules are:

- **Admission:** a user must be listed by the server attribute, with ``r``; any other user has no roles at all, and
  can only ``--ping`` the server, whatever the attributes below grant.
- **Server:** at the server, the user holds the roles of its server entry.
- **Suites:** the attribute of a suite replaces the server entry of the user: the suite may grant more roles than the
  server, or fewer. A suite *without* attribute takes the server entries unchanged: this is deliberate, so that a
  server attribute alone governs every suite that does not say otherwise.
- **Below suites:** the attribute of a family, task or alias restricts the inherited roles: the user keeps only the
  roles granted both above and by the attribute. Below a suite, roles can only be removed, never added.
- **Complete lists:** an attribute lists every user it grants something to; a user it does not list has no roles from
  that node down.
- **Sticky entries:** an entry with the flag ``s`` cannot be weakened below it:

  - at the server, the user always holds the roles of that entry, everywhere, added to whatever the attributes below
    grant;
  - on a suite, family, task or alias, the roles reached there are locked: the attributes below neither remove nor add
    roles, even when they do not list the user.

There are no deny entries: roles are removed by omission, or by restriction below a suite.

Removing a user from an attribute also removes that user from every attribute below it, since no attribute may name
a user absent from above. An attribute left empty by this removal is itself removed, and its node then inherits from
above: the users of the upper level may then gain access to that branch. Check the attributes below a node before
removing a user from it.

Examples, for the user ``alice``:

.. list-table::
   :header-rows: 1
   :widths: 30 30 20 20

   * - Server attribute
     - Attributes below
     - Node
     - Effective roles
   * - ``alice:rwx``
     - none
     - ``/s/f/t``
     - ``rwx``
   * - ``alice:r``
     - ``/s``: ``alice:rwx``
     - ``/s``
     - ``rwx``
   * - ``alice:rwx,bob:r``
     - ``/s``: ``bob:r``
     - ``/s``
     - none
   * - ``alice:rwx``
     - ``/s``: ``alice:r``; ``/s/f``: ``alice:rwx``
     - ``/s/f``
     - ``r``
   * - ``alice:rxs,bob:r``
     - ``/s``: ``bob:r``
     - ``/s``
     - ``rx`` (sticky at the server)
   * - ``alice:rwx``
     - ``/s``: ``alice:rw``; ``/s/f``: ``alice:rwxs``; ``/s/f/t``: ``alice:r``
     - ``/s/f/t``
     - ``rw`` (locked at ``/s/f``)
   * - ``alice:wx``
     - ``/s``: ``alice:rwx``
     - ``/s``
     - none (no ``r`` at the server)

.. _permissions_attribute_server:

Server attribute
================

The server attribute is set by the setting ``/ecflow/server/permissions`` of the configuration file
``<host>.<port>.ecf.cfg`` (see :ref:`server_configuration_file`), in the JSON form:

.. code-block:: json

   {
     "version": 1,
     "ecflow": {
       "server": {
         "permissions": [{"user": "admin", "permissions": "rwxlops"}, {"user": "ops", "permissions": "rx"}]
       }
     }
   }

At start-up, and with ``ecflow_client --restore_from_checkpt``:

- the configuration file is read first, then the checkpoint;
- a server attribute in the checkpoint overrides that of the configuration file;
- a checkpoint carrying a server attribute, while the configuration file sets none, stops the start-up (and makes
  the restore fail).

An invalid server attribute in the configuration file stops the start-up, before the log file is created. The log
summarises the attributes in force (see :ref:`ecflow_log_file`). Definitions cannot set the server attribute; it
appears only in checkpoints and in the definitions sent to clients.

.. _permissions_attribute_compatibility:

Compatibility with other versions
=================================

The attribute is available from ecFlow 5.20.0:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Situation
     - Behaviour
   * - Server 5.20.0 or later, without Permissions attributes
     - Works with clients of any supported version; its checkpoints can be loaded by older servers.
   * - Server 5.20.0 or later, with Permissions attributes
     - Requires clients of version 5.20.0 or later, which the server does not enforce; its checkpoints carry the
       attributes, which older servers cannot load.
   * - Downgrade of a server that uses Permissions attributes
     - Requires removing, by hand, the server attribute from ``<host>.<port>.ecf.cfg``, and every Permissions
       attribute from the primary and backup checkpoints, before starting the older server.

.. _permissions_attribute_unrestricted:

Making a server unrestricted again
==================================

A server becomes unrestricted only through its configuration and a restart:

#. stop the server;
#. remove ``/ecflow/server/permissions`` from ``<host>.<port>.ecf.cfg``;
#. remove the server attribute and every node attribute (the ``permissions`` lines) from the primary and the backup
   checkpoints;
#. start the server.

Otherwise, a checkpoint that still carries a server attribute stops the start-up.

.. _permissions_attribute_users:

User names
==========

A user name in an entry:

- is not empty, and is valid UTF-8;
- contains no white space, ``#``, ``,``, ``:`` or ``;``;
- does not start with ``-``;
- is not ``*``: there is no wildcard, and every user is named explicitly.

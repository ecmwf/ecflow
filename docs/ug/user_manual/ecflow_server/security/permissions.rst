.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _permissions_attribute:

Permissions attribute
/////////////////////

.. caution::
  The Permissions attribute is experimental, and its details are subject to change. Users are encouraged to try it
  and provide feedback, but it is **not to be used in production**. The documentation reflects its current operation.

The ``permissions`` attribute lists users and the roles each of them holds. Any node can carry one: suites,
families, tasks and aliases.

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

.. _permissions_attribute_users:

User names
==========

A user name in an entry:

- is not empty, and is valid UTF-8;
- contains no white space, ``#``, ``,``, ``:`` or ``;``;
- does not start with ``-``;
- is not ``*``: there is no wildcard, and every user is named explicitly.

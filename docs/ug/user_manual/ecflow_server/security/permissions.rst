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

.. _permissions_attribute_modes:

Authorisation modes
===================

The server authorises each user request in one of three modes:

.. list-table::
   :header-rows: 1
   :widths: 20 35 45

   * - Mode
     - Selected when
     - Requests allowed
   * - Permissions
     - a server attribute is in force
     - as granted by the Permissions attributes; the whitelist is not used
   * - Whitelist
     - no server attribute, and the :ref:`whitelist <ecflow_white_list_file>` (``ECF_LISTS``) is loaded
     - as granted by the whitelist
   * - Unrestricted
     - neither
     - every request

Notes:

- The whitelist remains supported during a transition. Setting ``/ecflow/server/whitelist/ignore`` to ``true`` in
  ``<host>.<port>.ecf.cfg`` makes the server ignore it: the file is neither loaded at start-up nor reloaded
  (``ecflow_client --reloadwsfile`` fails), and the log states ``ECF_LISTS (...) = ignored``.
- In Permissions mode, no user holds an implicit right: the user running the server is listed like any other, and
  the write access the whitelist grants to that user does not apply.
- Each command requires a role at each node it addresses (see :ref:`permissions_attribute_commands`); a node that
  does not exist is checked at its closest existing ancestor, and a request addressing the server at the server
  level.
- An error while evaluating the permissions denies the request.

Some requests are handled apart:

- task requests, sent by jobs (``--init``, ``--complete``, ``--label``, ...), are never subject to the authorisation
  of users, in any mode;
- in Permissions mode, ``--ping`` is allowed to every user, listed or not;
- in Permissions mode, each command of a ``--group`` is authorised just before it runs, as if sent on its own: a
  denied command neither undoes the earlier ones nor prevents the later ones, and the group is reported as denied;
  in whitelist mode, a group is allowed only when all its commands are.

Permissions never affect scheduling: dependencies, limits, job generation and job submission behave as without
attributes, whatever the attributes grant.

.. _permissions_attribute_commands:

Roles of commands
=================

The commands below require a role at the server level, or at the node they address:

.. list-table::
   :header-rows: 1
   :widths: 55 15 30

   * - Commands
     - Role
     - Where
   * - ``--ping``
     - none
     - in Permissions mode, every user
   * - ``--server_version``, ``--stats``, ``--stats_server``, ``--suites``, ``--zombie_get``, ``--server_load``,
       ``--log=get``, ``--log=path``, ``--sync``, ``--sync_full``, ``--sync_clock``, ``--news``, ``--ch_*``,
       ``--get``, ``--get_state``, ``--migrate``, ``--why`` and ``--show`` without a path
     - ``r``
     - server
   * - ``--get``, ``--get_state``, ``--migrate``, ``--why`` with a path; ``--query``
     - ``r``
     - the node (the server for ``--query ... /:name``)
   * - ``--msg``, ``--stats_reset``, ``--debug_server_on``, ``--debug_server_off``, ``--zombie_adopt``,
       ``--zombie_block``, ``--zombie_fail``, ``--zombie_fob``, ``--zombie_kill``, ``--zombie_remove``
     - ``w``
     - server
   * - ``--log=clear``, ``--log=flush``, ``--log=new``
     - ``l``
     - server
   * - ``--halt``, ``--shutdown``, ``--restart``, ``--terminate``, ``--check_pt`` (with or without arguments),
       ``--restore_from_checkpt``, ``--reloadwsfile``, ``--reloadpasswdfile``, ``--reloadcustompasswdfile``,
       ``--force-dep-eval``
     - ``x``
     - server
   * - ``--job_gen``, ``--checkJobGenOnly``
     - ``x``
     - the node, or the server without a path
   * - ``--file``, ``--edit_script`` (``edit``, ``pre_process``, ``pre_process_file``), ``--check``
     - ``r``
     - the node (the server for ``--check=_all_``)
   * - ``--edit_history`` with a path
     - ``r``
     - server: the history is returned unfiltered
   * - ``--alter``, ``--order``, ``--archive``
     - ``w``
     - the nodes (the server for ``--alter`` on ``/``)
   * - ``--edit_history=clear``
     - ``w``
     - server
   * - ``--suspend``, ``--resume``, ``--kill``, ``--status``, ``--force`` (states and events), ``--requeue``,
       ``--run``, ``--free-dep``, ``--edit_script`` (``submit``, ``submit_file``)
     - ``x``
     - the nodes
   * - ``--edit_script ... submit_file ... create_alias``
     - ``x`` and ``w``
     - the node
   * - ``--restore``
     - ``w`` and ``p``
     - the nodes: restored nodes may carry Permissions attributes

Notes:

- Client handles belong to the user who created them: only that user can change or drop a handle
  (``--ch_add``, ``--ch_rem``, ``--ch_auto_add``, ``--ch_drop``), and ``--ch_drop_user`` drops the handles of the
  requester only.
- Each command is checked at the nodes it names, not at the nodes it affects below them: ``x`` on a suite allows
  requeueing the whole suite. An event or attribute named as ``path:name`` is checked at the node ``path``.
- The other commands read with ``r`` and change with ``w`` at the nodes they address.
- In whitelist mode, the same commands require read access for ``r``, and write access for every other role. Hence
  ``--msg``, ``--log=clear``, ``--log=flush``, ``--check_pt`` without arguments, ``--debug_server_on``,
  ``--debug_server_off``, the zombie actions and ``--checkJobGenOnly`` require write access, and handles are bound
  to their creator as in Permissions mode.

.. _permissions_attribute_threat_model:

Threat model and scope
======================

Permissions attributes govern authorisation only. They rely on the authentication of every request beforehand (see
:ref:`security`): without authentication, a user name is merely a claim, and the attributes protect nothing.

Under that assumption:

.. list-table::
   :header-rows: 1
   :widths: 25 40 35

   * - Actor
     - Protected against
     - Not protected against
   * - Authenticated user
     - reading and changing the nodes, attributes and server settings not granted to them
     - what other channels reveal: the files of the server host, the outputs of jobs on disk
   * - Job (task requests)
     - nothing: task requests have a special grant, and are not subject to the attributes
     - a request presenting the job identifier of a task (``ECF_PASS``, used to detect zombies, not a password)
       acting on that task; how task requests are authenticated is outside the scope of the attributes
   * - Job running as the user of the server
     - nothing
     - access to the files of the server: configuration, checkpoints, password files
   * - User with access to the server host as the user of the server
     - nothing
     - changes to the configuration file and the checkpoints, which hold the attributes

Notes:

- ``w`` on a node grants more than changing its attributes: the variables of a node include commands the server
  runs as the user of the server (``ECF_JOB_CMD``, ``ECF_KILL_CMD``, ``ECF_STATUS_CMD``) and paths it reads and
  writes (``ECF_HOME``, ``ECF_FILES``, ``ECF_INCLUDE``, ``ECF_JOB``, ``ECF_JOBOUT``). A user holding ``w`` can
  therefore run commands and reach files as the user of the server: grant ``w`` only to users trusted with that.
- Requests and replies travel in clear text unless the server and its clients use SSL.
- The existence of a node addressed by a denied request is not concealed: the denial names the command only, and a
  request on a node that does not exist fails as such.
- The attributes restrict commands only, never the file system (the files of the server host, in the table above).
  The files the server writes hold what the attributes conceal from its clients, and the server creates them with
  the permissions its umask allows, usually readable by every account of the host:

  - the checkpoint and its backup (``ECF_CHECK`` and ``ECF_CHECKOLD``, by default ``<host>.<port>.ecf.check`` and
    ``<host>.<port>.ecf.check.b`` in ``ECF_HOME``): every node and attribute, the server attribute, and the edit
    history, which names users;
  - the archives, in the ``ECF_HOME`` of each archived node, named after its path with ``:`` in place of ``/`` (e.g.
    ``<host>.<port>.:s:f.check`` for ``/s/f``): the archived nodes and their attributes;
  - the server log (``ECF_LOG``): every request with its user, the changes of attributes, the denied and audited
    requests.

  When the server uses Permissions attributes, only the user of the server should be able to read these files:

  - give the directories holding them (``ECF_HOME``, the directories of ``ECF_CHECK``, ``ECF_CHECKOLD`` and
    ``ECF_LOG`` when set elsewhere, and the ``ECF_HOME`` of archived nodes when it differs) the mode ``0700``;
  - start the server with the umask ``077``, for the files it creates from then on;
  - restrict the files that already exist (e.g. ``chmod 600``): the server keeps the mode of a file it appends to
    (the log) or renames (the checkpoint becoming the backup), whatever its umask.

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

A server stops using Permissions attributes only through its configuration and a restart:

#. stop the server;
#. remove ``/ecflow/server/permissions`` from ``<host>.<port>.ecf.cfg``;
#. remove the server attribute and every node attribute (the ``permissions`` lines) from the primary and the backup
   checkpoints;
#. start the server.

Otherwise, a checkpoint that still carries a server attribute stops the start-up.

The server is then in whitelist mode when its :ref:`whitelist <ecflow_white_list_file>` (``ECF_LISTS``) exists, and
unrestricted only without it (see :ref:`permissions_attribute_modes`). To make it unrestricted, also remove the
whitelist file, or set ``/ecflow/server/whitelist/ignore`` to ``true``, before starting the server.

.. _permissions_attribute_users:

User names
==========

A user name in an entry:

- is not empty, and is valid UTF-8;
- contains no white space, ``#``, ``,``, ``:`` or ``;``;
- does not start with ``-``;
- is not ``*``: there is no wildcard, and every user is named explicitly.

.. _permissions_attribute_denials:

Denied requests
===============

A request refused by the authorisation of the server is a *denial*. The message of a denial names the command only,
never a node, an entry, a file or an argument of the request:

.. code-block:: text

  Permission denied: --suspend

Each client reports a denial distinctly from any other error:

.. list-table::
   :header-rows: 1

   * - Client
     - Denial
   * - ``ecflow_client``
     - Exit status ``3``; any other error keeps exit status ``1``
   * - Python
     - Raises ``ecflow.PermissionDenied``, a subclass of ``RuntimeError``
   * - Rust
     - Returns the failure ``Failure::PermissionDenied``

Notes:

- The client prefixes the message with the request as given by the user, as for any other error.
- The server records each denial in its log file, at a bounded rate (see :ref:`ecflow_log_file_denials`).
- The denials come from the Permissions attributes or from the :ref:`whitelist <ecflow_white_list_file>`,
  according to the :ref:`mode <permissions_attribute_modes>` in force.

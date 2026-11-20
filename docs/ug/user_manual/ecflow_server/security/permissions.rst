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

Removing a user from an attribute also removes that user from the attributes below it, wherever the user no longer
holds any role above them, since no attribute may name a user absent from above (see
:ref:`permissions_attribute_changes`). An attribute left empty by this removal is itself removed, and its node then
inherits from above: the users of the upper level may then gain access to that branch. Check the attributes below a
node before removing a user from it.

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

.. _permissions_attribute_audit:

Audit-only mode
---------------

The audit-only mode trials a configuration of Permissions attributes: the requests the attributes deny are executed
and recorded, instead of being refused, except reads, which are always enforced. It is selected in
``<host>.<port>.ecf.cfg``:

.. code-block:: json

   {"version": 1, "ecflow": {"server": {
     "permissions": [{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}],
     "authorisation": {"audit_only": true}}}}

In audit-only mode:

- each request is evaluated as in Permissions mode;
- a request denied for want of the role ``r`` is refused, as in Permissions mode, so that no content is returned
  that Permissions mode would refuse (for example by ``--file``, ``--why``, ``--check``, ``--edit_script`` or
  ``--edit_history``);
- a request denied only for want of another role, or only because a client handle belongs to another user, is
  executed nevertheless, and recorded in the server log as a warning, e.g. ``WAR:[...] Audit only, executed although
  denied: Authorisation (user) failed, due to: Insufficient permissions (role x required at /s) [--suspend /s
  :alice@host]``;
- each command of a ``--group`` is evaluated, and then refused or executed, on its own;
- the content returned to each user is filtered as in Permissions mode: unreadable nodes stay hidden;
- the checks that are not authorisation still apply: authentication, the validation of changes (for example a change
  leaving nobody with ``p`` is refused), and the :ref:`limits <permissions_attribute_limits>`;
- a request whose evaluation fails, or whose record cannot be written to the log (for example on a full file system),
  is refused, as in Permissions mode.

Notes:

- The mode applies to Permissions mode only: without a server attribute, the whitelist is enforced as usual.
- The start-up log states ``Mode = permissions (audit only)``.
- Every requirement of ``r`` of a request is checked before its other requirements, so a request lacking ``r``
  wherever it requires that role is refused, even when it also lacks another role. ``--edit_history`` requires
  ``r`` at server level only.
- A request that needs only other roles is executed even at a node the user cannot read, for example ``--suspend``,
  which needs only ``x``; it returns no content of the node.
- Before enforcing a configuration, search the log for ``Audit only`` to find the requests it would refuse; the
  refused reads are logged as ``Command not accepted``, as in Permissions mode.

.. _permissions_attribute_monitoring:

Monitoring
----------

``ecflow_client --stats`` reports, in its section ``Authorisation``:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Line
     - Value
   * - ``Mode``
     - ``permissions``, ``permissions (audit only)``, ``whitelist`` or ``unrestricted``
   * - ``Denied --<command>``
     - the requests refused by the authorisation, per command, in any mode; in Permissions mode each command of a
       ``--group`` counts on its own, while in whitelist mode a refused group counts as ``--group``
   * - ``Audited --<command>``
     - the requests executed in :ref:`audit-only mode <permissions_attribute_audit>` although denied, per command
   * - ``Filtered replies``
     - the replies built from the nodes their requester can read (definitions, nodes, synchronisations); in
       Permissions mode, every such reply
   * - ``Filtering time``
     - the time taken to build those replies, including the selection of the readable nodes
   * - ``Readable sets cached``
     - the full definitions held in the cache of the server, one for each set of readable nodes

Notes:

- ``ecflow_client --stats_reset`` clears the counters; the mode and the cache are kept.
- Lines with a zero count are not shown.
- A server older than the Permissions attribute reports no ``Authorisation`` section.

.. _permissions_attribute_limits:

Limits
------

The server can bound the inputs of its clients. Each limit is set in ``<host>.<port>.ecf.cfg``, under
``/ecflow/server/limits``, and enforced where the input arrives; ``0``, the default, sets no limit:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Setting
     - Effect
   * - ``request_size``
     - the largest request accepted, in bytes (its serialised form); a larger request is refused before it is read,
       with an error reply (over HTTP, the status ``413 Payload Too Large``), and the server log records it; the server
       keeps serving the other requests
   * - ``permissions_entries``
     - the largest number of entries of a Permissions attribute brought in by ``--permissions create`` or
       ``update``, ``--load``, ``--replace`` and ``--plug`` (on the receiving server); a larger attribute refuses the
       whole request, and nothing changes
   * - ``handles_per_user``
     - the largest number of client handles a user holds; ``--ch_register`` beyond it is refused, while replacing a
       handle of the user, or dropping one, is always possible

For example:

.. code-block:: json

   {"version": 1, "ecflow": {"server": {
     "limits": {"request_size": 104857600, "permissions_entries": 200, "handles_per_user": 20}}}}

Notes:

- The limits apply in every mode, with or without Permissions attributes.
- Over HTTP, without ``request_size``, the HTTP library keeps its own default limit on the size of a request.
- The attributes already held by the server (in checkpoints and archives) are not checked against
  ``permissions_entries``.

.. _permissions_attribute_commands:

Roles of commands
=================

The commands below require a role at the server level, or at the node they address. The help of each command
(``ecflow_client --help=<command>``, and its page in the :ref:`command reference <ecflow_cli>`) states the same
roles:

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
   * - ``--load``, ``--begin``; ``--delete`` of a suite, or of all suites
     - ``o``
     - server
   * - ``--load`` of definitions carrying server variables
     - ``w`` too
     - server
   * - ``--load`` of a suite carrying Permissions attributes, or overwriting (``--force``) one that carries them
     - ``p`` too
     - the suite
   * - ``--replace`` of a node; ``--delete`` of a node below a suite
     - ``w``
     - the node (for a node created by ``--replace``, its closest existing ancestor)
   * - ``--replace`` of a suite, or creating one
     - ``o``
     - server
   * - ``--replace`` bringing or replacing Permissions attributes
     - ``p`` too
     - the node
   * - ``--plug``
     - ``w`` and ``p``
     - the source and the destination; for a destination on another server, that server checks the destination
       under the identity of the user: ``w`` and ``p`` there, or ``o`` (and ``p`` for attributes) for a whole suite
   * - ``--permissions read``, ``--permissions effective``
     - ``r``
     - the node (the server for ``/``)
   * - ``--permissions create``, ``--permissions update``, ``--permissions delete``
     - ``p``
     - the node (the server for ``/``)

Notes:

- Client handles belong to the user who created them: only that user can change or drop a handle
  (``--ch_add``, ``--ch_rem``, ``--ch_auto_add``, ``--ch_drop``), and ``--ch_drop_user`` drops the handles of the
  requester only.
- Each command is checked at the nodes it names, not at the nodes it affects below them: ``x`` on a suite allows
  requeueing the whole suite. An event or attribute named as ``path:name`` is checked at the node ``path``.
- The other commands read with ``r`` and change with ``w`` at the nodes they address.
- ``--begin``, and ``--delete`` of a suite or of all suites, require ``o`` at server level only, whatever the
  attributes of the suites: a user holding ``o`` at the server begins and deletes any suite, including the suites
  that user cannot read. Grant ``o`` only to the administrators of the server.
- In whitelist mode, the same commands require read access for ``r``, and write access for every other role. Hence
  ``--msg``, ``--log=clear``, ``--log=flush``, ``--check_pt`` without arguments, ``--debug_server_on``,
  ``--debug_server_off``, the zombie actions and ``--checkJobGenOnly`` require write access, and handles are bound
  to their creator as in Permissions mode.

.. _permissions_attribute_filtering:

What a user receives
====================

A user receives only the nodes where the user holds ``r``: a node the user cannot read is left out of every reply,
with its attributes, its state and every node below it. Since permissions only restrict from the suite down, no
node below an unreadable node is readable.

The replies filtered are:

- the definitions sent by ``--get``, ``--get_state`` and ``--migrate`` without a path;
- the node sent by ``--get``, ``--get_state`` and ``--migrate`` with a path, which the user can read, without the
  nodes below it that the user cannot read;
- the full synchronisation of a client (e.g. ecFlowUI), with or without a client handle;
- the incremental synchronisation of a client: the changes of the nodes it cannot read are left out, and the
  children added below a readable node, the order of its children and the order of the suites name only the nodes
  the client can read.
- ``--suites``, which lists the readable suites, and ``--stats``, which counts them;
- ``--zombie_get``, which lists the zombies of the readable tasks;
- the state of the server shown at the root (e.g. by ecFlowUI), and returned by ``--query state /``, aggregated from
  the readable suites: every readable suite without a client handle, the readable suites of the handle otherwise.

Every reply type declares how it is filtered: not at all (no node data, or data accepted unfiltered, listed below),
by content (definitions and nodes) or as a listing (names and counts of nodes).

Users reading the same nodes share the content the server prepares for them; users reading different nodes never
receive content prepared for another.

Client handles (``--ch_register``, ``--ch_add``, ``--ch_suites``) follow the same rule:

- registering, or adding to a handle, a suite the user cannot read is denied (``role r required at /<suite>``); on
  the handle of another user, the denial names both causes (``the handle belongs to another user; Insufficient
  permissions (role r required at /<suite>)``);
- a registered suite that becomes unreadable stays registered, and is left out of every reply until it is readable
  again;
- the suites added automatically to a handle (``--ch_auto_add``) are sent only when readable;
- ``--ch_suites`` lists the user's own handles, with the suites the user can read.

The following are not filtered, and remain visible to every user holding ``r`` at server level:

- the server log (``--log=get``, ``--server_load``) and the edit history (``--edit_history``, and the history
  included by ``--migrate``), whose lines are free text;
- the references to other nodes written in the attributes of a readable node: trigger and complete expressions,
  limits and ``inlimit``, Mirror and Aviso attributes, shown as written;
- the content of a file read through a command authorised on a readable node (e.g. ``--file`` of its job output),
  wherever the variables of that node point;
- the change numbers returned by ``--news``, which tell that something changed in the server, not what;
- the results of the requests evaluating a readable node, which may depend on, and name, nodes below it or elsewhere:
  the diagnostics of ``--check`` (e.g. a trigger of a task below the node that names a node missing), the
  explanations of ``--why``, and the value of an expression evaluated by ``--query``.

These channels let a user infer that unreadable nodes exist (e.g. a trigger naming them, or a change number moving
while nothing readable changed); this is accepted.

.. _permissions_attribute_changes:

Changing permissions
====================

The command ``--permissions`` reads and changes attributes at run time. It acts on the whole list of one attribute,
at exactly one path; ``/`` addresses the server attribute:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Command
     - Effect
   * - ``--permissions create <path> <value>``
     - Gives the node an attribute; fails when the node already has one.
   * - ``--permissions read <path>``
     - Prints the attribute; fails when the node has none.
   * - ``--permissions update <path> <value>``
     - Replaces the attribute; fails when the node has none.
   * - ``--permissions delete <path>``
     - Removes the attribute; the node then inherits from above. Fails when the node has none.
   * - ``--permissions effective <path> [<user>]``
     - Prints the effective roles of the user at the path (the requester by default), e.g. ``rwx``; nothing when the
       user holds no role there.

Notes:

- The value is given in the command-line form (``alice:rwxp,bob:r``) or in the JSON form; ``--fmt json`` makes
  ``read`` and ``effective`` print the JSON form.
- ``read`` and ``effective`` require ``r`` at the path; ``create``, ``update`` and ``delete`` require ``p``.
- At ``/``, only ``update`` is possible: the server attribute is created or removed only through the configuration
  file and a restart (see :ref:`permissions_attribute_unrestricted`).
- On a server without a server attribute, node attributes cannot be created.

Every change, whether made by ``--permissions`` or brought by another command, is validated as a whole, and applied
completely or not at all. A change is rejected when:

- an entry names a user who is not listed above the node: at a suite, a user not listed by the server attribute;
  below a suite, a user holding no effective role at the parent node;
- it would leave a node, or the server, where some user held ``p``, without any user holding ``p``.

Removing a user from an attribute also removes that user from the attributes below it, wherever the user no longer
holds any role above them (a sticky server entry keeps its user listed everywhere); an attribute left empty is
removed, and its node then inherits from above. This can give the users of the upper level access to the branch
below. Archived nodes are adjusted in the same way when they are restored (see
:ref:`permissions_attribute_restore`).

The definitions brought by ``--load`` and ``--replace`` are validated where their nodes go, with the same rules,
before anything changes:

- an entry naming a user who is not listed above its node rejects the whole request: such entries are never removed
  from the definitions brought;
- ``--load`` rejects definitions that carry a server attribute;
- on a server without a server attribute, definitions carrying node attributes are rejected.

The attributes they add, change or remove, ``--load --force`` and the nodes replaced included, are written to the
server log like the changes made by ``--permissions``.

``--plug`` validates the node plugged, with its attributes, archived parts included, where it goes, with the same
rules:

- for a local plug, on this server: a node that had a user holding ``p`` where it was must keep one where it goes;
- for a plug to another server, on the receiving server, which knows nothing of the sending one: a node with a user
  holding ``p`` at the destination (or at server level, for a whole suite) must keep one.

The log holds both paths: the attributes removed from where the node was, and added where it goes (on each server,
for a plug to another server). ``--delete`` logs the attributes removed with the nodes deleted, archived ones
included.

Each change is written to the server log, with the requester, the path, and the old and new values, e.g.::

   LOG:[10:12:01 15.11.2026]  permissions /s alice:rwxp,bob:r -> alice:rwxp by alice@host
   LOG:[10:12:01 15.11.2026]  permissions /s/f alice:rp,bob:r -> alice:rp (removal of users above) by alice@host

A change of permissions can change what clients may read: the clients receiving the nodes concerned (every client,
for a change of the server attribute) synchronise fully at their next request, and from then on receive only what
the new permissions allow.

Configuring the server attribute, or removing it, requires a restart: every client then reconnects and synchronises
in full.

.. _permissions_attribute_python:

Python
======

The Python module binds the attribute as :py:class:`ecflow.Permissions`, built from either textual form or from a
list of entries:

.. code-block:: python

   import ecflow

   team = ecflow.Permissions("alice:rwxp,bob:r")          # or ["alice:rwxp", "bob:r"], or the JSON form
   defs = ecflow.Defs(
       ecflow.Suite("s", team, ecflow.Family("f", ecflow.Permissions("alice:rwxp"), ecflow.Task("t"))))

   # One attribute applied to a set of nodes, all or none; replace=True replaces existing attributes
   ecflow.Permissions("alice:rxp").add_to(defs.get_all_tasks())

   # Offline validation, as a load into a server with this server attribute would validate the definitions
   errors = defs.check_permissions(ecflow.Permissions("admin:rwxlops,alice:rwxp,bob:r"))
   assert errors == "", errors

A :py:class:`ecflow.Permissions` object provides:

- ``entries()``: the entries, each a string ``<user>:<letters>``, sorted by user;
- ``roles(user)``: the letters of the roles granted to a user, without the sticky flag, or ``None``;
- ``sticky(user)``: whether the entry of a user is sticky;
- ``to_cli()`` and ``to_json()``: the command-line and JSON forms;
- ``add_to(nodes, replace=False)``: the attribute added to a set of nodes, all or none.

Its string form is its line in a definition file. Nodes provide:

- ``add_permissions(value)``, returning the node;
- ``get_permissions()``, returning a copy of the attribute, or ``None``;
- ``delete_permissions()``, after which the node inherits its roles from above.

The full reference is in :py:class:`ecflow.Permissions`, :py:meth:`ecflow.Node.add_permissions` and
:py:meth:`ecflow.Defs.check_permissions`.

The client provides the operations of ``--permissions``:

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Method
     - Command
   * - ``create_permissions(path, value)``
     - ``--permissions create``
   * - ``get_permissions(path)``, returning a :py:class:`ecflow.Permissions`
     - ``--permissions read``
   * - ``update_permissions(path, value)``
     - ``--permissions update``
   * - ``delete_permissions(path)``
     - ``--permissions delete``
   * - ``effective_permissions(path, user="")``, returning the letters, e.g. ``"rwx"``
     - ``--permissions effective``

A denied request raises ``ecflow.PermissionDenied``. An invalid value, or any other failure of a request, raises
``RuntimeError``; an argument of another type (neither a ``Permissions`` object, a string, nor a list or tuple of
strings) raises ``TypeError``.

On a server without a server attribute, ``effective_permissions`` raises ``RuntimeError``, since the server does not
use Permissions attributes, and ``create_permissions`` is rejected.

.. _permissions_attribute_rust:

Rust
====

The ``ecflow`` crate provides the operations of ``--permissions`` on ``Client``, and gives the attribute in the JSON
form:

.. list-table::
   :header-rows: 1
   :widths: 55 45

   * - Method
     - Command
   * - ``create_permissions(path, value)``
     - ``--permissions create``
   * - ``read_permissions(path)``, returning the JSON form
     - ``--permissions read``
   * - ``update_permissions(path, value)``
     - ``--permissions update``
   * - ``delete_permissions(path)``
     - ``--permissions delete``
   * - ``effective_permissions(path, user)``, returning the letters; ``None`` for the requester
     - ``--permissions effective``

The value is given in either textual form. A denied request fails with an error whose ``failure()`` is
``Failure::PermissionDenied``.

ecFlowUI
========

ecFlowUI shows the attribute of a node as a row of its attributes, with its entries in the command-line form; the
tooltip lists one entry per line. The attribute can be:

- shown or hidden with the entry "Permissions" of the attribute filter; new tree views show it by default;
- found by a search on the attribute ``permissions``, by its value or by its users;
- tested with the condition ``has_permissions`` in the menu configuration and in searches;
- added to a node without one with "Add permissions" of the "Special" menu, in a dialog holding one entry per
  line. Accepting the dialog runs ``ecflow_client --permissions create``;
- edited with "Edit ...", in a dedicated dialog holding one entry per line. Applying the dialog runs
  ``ecflow_client --permissions update``;
- deleted with "Delete", after a confirmation whose default answer is "Cancel". This runs
  ``ecflow_client --permissions delete``; the node then inherits its roles from above.

The server attribute is not shown as a row of the tree. On a server that has one, it is:

- edited with "Edit Permissions ..." of the context menu of the server, in the same dialog holding one entry per
  line. Accepting the dialog runs ``ecflow_client --permissions update /``;
- never added nor deleted from ecFlowUI: it is configured at start-up (see :ref:`permissions_attribute_server`).

The server validates every change, as for ``ecflow_client`` itself: a rejected change (for example one that leaves
nobody holding ``p``) is reported like any other failed command, and the attribute is left unchanged.

The Info panel of the server and of every node shows, on a server using Permissions, the line ``permissions :``
listing the effective roles of every user there, for example ``admin:rwxlop, alice:rwxp, bob:r``. The roles are
computed by ecFlowUI with the rules of the server; users are listed by name, and users without any role there are
not listed.

When permissions change during a session, ecFlowUI follows the full synchronisation of its client: nodes that can no
longer be read disappear, with the panels open on them. When the server denies the refresh itself (the user lost
read access at server level), the tree and its panels are cleared, unlike after a connection failure, and the server
is shown as lost with the reason, and the info panels showing the server are emptied; ecFlowUI synchronises in
full once access returns. A suite filter naming a suite that the user can no longer read registers the suites that
can be read; a suite of the filter that becomes readable again returns at the next refresh.

REST API
========

The REST API does not support Permissions: against a server that has a server attribute, it refuses every request
(see :ref:`rest_api_permissions`).

.. _permissions_attribute_restore:

Restoring archived nodes
========================

Archived nodes are restored (``--restore``, ``autorestore``, or a ``--begin`` or ``--requeue`` of an archived
container) with the attributes saved in their archive, which may no longer fit the attributes above them.

On a server with a server attribute:

- the entries of users no longer listed above a restored node are removed, as when a user is removed above it, and
  an attribute left empty is removed; each removal is written to the server log (``... (removal of users above) by
  restore of <path>``);
- the restore never fails because of this, even when it leaves a restored node without any user holding ``p``. The
  server then writes a warning naming the node, e.g.::

     WAR:[10:12:01 15.11.2026] restore of /s/f: nobody is able to change the Permissions attribute of /s/f/g; a sticky server entry holding p can change it

  Such a node can be read and run as its attribute allows, but its attribute cannot be changed by ``--permissions``
  until a user holds ``p`` there. To recover, give a user a sticky server entry holding ``p`` (for example
  ``--permissions update / ...,admin:rwxlops``), change the attribute of the node, and remove the sticky entry if it
  is no longer needed.

On a server without a server attribute, where node attributes are not allowed:

- an explicit ``--restore`` of nodes carrying attributes fails, as a load would, and leaves the archive in place;
- an automatic restore (``autorestore``, ``--begin``, ``--requeue``) removes their attributes, and writes a warning
  naming the nodes.

.. _permissions_attribute_checkpoints:

Invalid checkpoints
===================

At start-up, and with ``--restore_from_checkpt``, every attribute of a checkpoint is checked as a load would check it:
its syntax, the users listed above each node, and node attributes only with a server attribute (that of the
checkpoint, or of the configuration file). An invalid attribute makes the checkpoint unusable; it is never dropped:

- the server then uses the backup checkpoint, which is checked in the same way, and writes a warning, since the
  backup may carry older attributes, granting more than those of the rejected checkpoint;
- when both checkpoints are unusable, the server does not start. Its output names each file and the reason: the line
  of a syntax error, or the node and the user at fault.

To recover, correct the checkpoint by hand at the line or node reported (or remove the attribute concerned, or the
entry naming the user), and start the server again.

.. _permissions_attribute_internal_clients:

Granting access to internal clients
===================================

Internal clients (health checks, monitoring) are ecFlow users like any other, and receive no implicit right:

- ``--ping`` needs no role, and suits a health check that only verifies that the server answers;
- a client reading the server (``--stats``, ``--suites``, ``--news``) needs an entry granting ``r`` in the server
  attribute; a client acting on suites needs the corresponding roles there;
- the identity of an internal client is authenticated as any other, e.g. by a custom password file
  (``ECF_CUSTOM_PASSWD``).

For example, a monitoring client named ``monitor`` reading the server only:

.. code-block:: json

   {"user": "monitor", "permissions": "r"}

The REST API does not support Permissions: granting roles to the user of the REST server does not enable it on a
server using Permissions.

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
- The existence of a node addressed by a denied request is not concealed: a request on a node that does not exist
  fails as such.
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

A user name in an entry follows the :ref:`user name model <user_names>`, as every user name does. In particular, a
name is not ``*``: there is no wildcard, and every user is named explicitly.

.. _permissions_attribute_denials:

Denied requests
===============

A request refused by the authorisation of the server is a *denial*. Its message names the reason, the request and the
requester:

.. code-block:: text

  Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role x required at /s/f) [--suspend /s/f :bob@client.host]

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

To obtain the access a denial reports:

#. read the role and the node it names, e.g. ``role x required at /s/f``;
#. ask a user holding ``p`` at that node to grant the role (``ecflow_client --permissions update``): the attribute in
   force is that of the node, or of its closest ancestor carrying one, and ``ecflow_client --permissions read`` on
   that node shows its entries; ``ecflow_client --permissions effective /s/f <user>`` tells the roles a user holds;
#. when nobody holding ``p`` can be reached, ask the administrators of the server, who hold ``p`` at the server level
   (a sticky entry keeps it on every node, see :ref:`permissions_attribute_effective`).

The errors of the command ``--permissions`` follow one style, without internal names: the option, then the reason,
e.g. ``--permissions: /s/f has no Permissions attribute``.

Notes:

- The server writes the same message in its log file, for every denial (see :ref:`ecflow_log_file_denials`).
- The denials come from the Permissions attributes or from the :ref:`whitelist <ecflow_white_list_file>`,
  according to the :ref:`mode <permissions_attribute_modes>` in force.

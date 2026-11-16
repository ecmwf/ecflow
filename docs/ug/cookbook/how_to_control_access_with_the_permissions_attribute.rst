.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: permissions (cookbook)
   single: access control (cookbook)

.. _how_to_control_access_with_the_permissions_attribute:

How to control access to suites with the Permissions attribute?
***************************************************************

.. caution::
  The Permissions attribute is experimental, and its details are subject to change. Users are encouraged to try it
  and provide feedback, but it is **not to be used in production** (see :ref:`permissions_attribute`).

The :term:`permissions` attribute grants roles to the users it lists, on the server and on any :term:`node`. This
recipe configures a server shared by two teams, and shows the common tasks of its users and administrators: granting
read-only access, letting operators run suites they cannot edit, hiding a family, adding a team member, handing a
suite over, and recovering access. The rules of the attribute are described in detail in
:ref:`permissions_attribute`.

The deployment of this feature has the following requirements:

- ecFlow 5.20.0 or later, for the server and for every client (see
  :ref:`permissions_attribute_compatibility`);
- authenticated users, e.g. by password files or by an authenticating reverse proxy: without authentication, a user
  name is merely a claim (see :ref:`permissions_attribute_threat_model`);
- no use of the REST API, which does not support Permissions (see :ref:`rest_api_permissions`).

Roles at a glance
=================

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Letter
     - Role
     - Allows, for example
   * - ``r``
     - read
     - ``--get``, ``--suites``, ``--why``, ``--file``; being admitted to the server at all
   * - ``w``
     - write
     - ``--alter``, ``--replace`` of a node, ``--delete`` of a node below a suite
   * - ``x``
     - execute
     - ``--suspend``, ``--resume``, ``--requeue``, ``--run``, ``--kill``, ``--force``
   * - ``l``
     - log
     - ``--log=clear``, ``--log=flush``, ``--log=new``
   * - ``o``
     - owner
     - ``--load``, ``--begin``, ``--delete`` of a suite
   * - ``p``
     - permissions
     - ``--permissions create``, ``update`` and ``delete``
   * - ``s``
     - sticky (a flag)
     - nothing by itself: the roles of the entry cannot be weakened below it

The complete list is in :ref:`permissions_attribute_commands`.

The example setting
===================

The users of the server are:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - User
     - Role in the organisation
   * - ``admin``
     - the administrator of the server
   * - ``alice``, ``andy``
     - team A; ``alice`` leads the team
   * - ``bob``
     - team B, and its lead
   * - ``ops1``
     - an operator, running the suites of both teams
   * - ``carol``
     - an observer, reading everything
   * - ``eve``
     - an authenticated user, not admitted to the server

Configuring the server
======================

The server attribute is set in the :ref:`server configuration file <server_configuration_file>`
``<host>.<port>.ecf.cfg``, in the directory where the server is started:

.. code-block:: json

    {"version": 1, "ecflow": {"server": {
      "permissions": [{"user": "admin", "permissions": "rwxlops"},
                      {"user": "alice", "permissions": "r"},
                      {"user": "andy", "permissions": "r"},
                      {"user": "bob", "permissions": "r"},
                      {"user": "carol", "permissions": "rs"},
                      {"user": "ops1", "permissions": "r"}]}}}

Notes:

- Only the users listed with ``r`` are admitted: any other user can only ``--ping`` the server.
- The user running the server has no implicit right: list it, when it issues commands.
- ``admin`` holds every role, and its entry is sticky (``s``): it keeps these roles on every node, whatever the
  attributes below grant.
- ``carol`` holds ``r``, sticky: carol reads every node, and changes nothing.
- The file is read at start-up: configuring the server attribute, or removing it, requires a restart.

At start-up, the server log summarises the attributes in force:

.. code-block:: text

    MSG:[10:50:02 18.11.2026] Permissions:
    MSG:[10:50:02 18.11.2026]   Mode = permissions
    MSG:[10:50:02 18.11.2026]   Source = configuration file
    MSG:[10:50:02 18.11.2026]   Users = 6
    MSG:[10:50:02 18.11.2026]   Sticky users = admin, carol
    MSG:[10:50:02 18.11.2026]   Nodes with permissions = 0

``ecflow_client --stats`` reports the mode in its section ``Authorisation``:

.. code-block:: text

    Authorisation
      Mode                            permissions

.. _how_to_control_access_with_the_permissions_attribute_two_teams:

Two teams sharing a server
==========================

Each team receives a suite, whose attribute replaces the server entries:

.. code-block:: shell

    suite team_a
      permissions [{"user": "alice", "permissions": "rwxp"}, {"user": "andy", "permissions": "rwx"}, {"user": "ops1", "permissions": "rx"}]
      family forecast
        task run
      endfamily
      family archive
        permissions [{"user": "alice", "permissions": "rwxp"}, {"user": "ops1", "permissions": "r"}]
        task store
      endfamily
    endsuite
    suite team_b
      permissions [{"user": "bob", "permissions": "rwxp"}, {"user": "ops1", "permissions": "rx"}]
      family forecast
        task run
      endfamily
    endsuite

``admin`` loads and begins the suites. Loading a suite carrying Permissions attributes requires the role ``o`` at the
server level, and the role ``p`` at the suite; for a suite not yet loaded, ``p`` is checked at the server level.

.. code-block:: shell

    ecflow_client --load=teams.def
    ecflow_client --begin=team_a
    ecflow_client --begin=team_b

In Python, :py:meth:`ecflow.Defs.check_permissions` checks the definitions against a server attribute, as the load
would, before they are saved or loaded:

.. code-block:: python

    from ecflow import Defs, Suite, Family, Task, Permissions

    defs = Defs(
        Suite("team_a", Permissions("alice:rwxp,andy:rwx,ops1:rx"),
              Family("forecast", Task("run")),
              Family("archive", Permissions("alice:rwxp,ops1:r"), Task("store"))),
        Suite("team_b", Permissions("bob:rwxp,ops1:rx"), Family("forecast", Task("run"))))

    server = Permissions("admin:rwxlops,alice:r,andy:r,bob:r,carol:rs,ops1:r")
    assert defs.check_permissions(server) == ""

    # A server attribute that does not list andy:
    print(defs.check_permissions(Permissions("admin:rwxlops,alice:r,bob:r,ops1:r")))
    # suite /team_a: The Permissions attribute of /team_a names the user 'andy', who is not listed above it

The roles each user holds, as reported by ``ecflow_client --permissions effective <path> <user>`` (``-`` stands for
no role):

.. code-block:: text

    user   /        /team_a  /team_b  /team_a/archive
    admin  rwxlop   rwxlop   rwxlop   rwxlop
    alice  r        rwxp     -        rwxp
    andy   r        rwx      -        -
    bob    r        -        rwxp     -
    carol  r        r        r        r
    ops1   r        rx       rx       r

``bob`` receives only the suite of team B, and cannot act on the suite of team A:

.. code-block:: text

    $ ecflow_client --suites
    team_b
    $ ecflow_client --suspend=/team_a
    Error: request( --suspend /team_a ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role x required at /team_a) [--suspend /team_a :bob@host]
    $ echo $?
    3

Notes:

- A suite without attribute takes the server entries unchanged: on this server, every admitted user could read it.
- ``o`` at the server allows ``--begin`` and ``--delete`` of any suite, including the suites its holder cannot read:
  grant it only to administrators.
- ``w`` on a node allows changing its variables, among which the commands the server runs (``ECF_JOB_CMD``): grant
  ``w`` only to users trusted with the account of the server (see :ref:`permissions_attribute_threat_model`).
- Triggers may name the nodes of another team: the expressions of a readable node are shown as written, while the
  nodes they name stay hidden (see :ref:`permissions_attribute_filtering`).

Read-only observers
===================

An observer is admitted with ``r`` at the server:

- with ``r``, the observer reads the server and the suites without attribute, but not the suites whose attribute
  omits the observer;
- with ``rs`` (sticky), the observer reads every node, whatever the attributes below.

``carol``, a sticky observer, reads both suites, and cannot change them:

.. code-block:: text

    $ ecflow_client --suites
    team_a team_b
    $ ecflow_client --suspend=/team_b
    Error: request( --suspend /team_b ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role x required at /team_b) [--suspend /team_b :carol@host]

A monitoring client is an observer like any other (see :ref:`permissions_attribute_internal_clients`); a health check
needs no role at all, since ``--ping`` is allowed to every user. ``eve``, not listed by the server attribute, can only
ping the server:

.. code-block:: text

    $ ecflow_client --ping
    $ ecflow_client --suites
    Error: request( --suites ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role r required at /) [--suites :eve@host]

Operators who run but do not edit
=================================

``ops1`` holds ``rx`` on both suites: ``ops1`` suspends, resumes and requeues their nodes, and cannot change them.
Below a suite, an attribute restricts the roles further: the family ``archive`` grants ``ops1`` only ``r``.

.. code-block:: text

    $ ecflow_client --suspend=/team_a/forecast
    $ ecflow_client --resume=/team_a/forecast
    $ ecflow_client --alter change variable ECF_TRIES 2 /team_a
    Error: request( --alter change variable ECF_TRIES 2 /team_a ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role w required at /team_a) [--alter change variable ECF_TRIES 2 /team_a :ops1@host]
    $ ecflow_client --suspend=/team_a/archive
    Error: request( --suspend /team_a/archive ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role x required at /team_a/archive) [--suspend /team_a/archive :ops1@host]

Hiding a family
===============

``alice``, holding ``p`` on the suite, gives the family ``forecast`` an attribute that omits ``andy``:

.. code-block:: shell

    ecflow_client --permissions create /team_a/forecast alice:rwxp,ops1:rx

``andy`` no longer receives the family, while the sticky entries keep their roles there:

.. code-block:: text

    andy at /team_a/forecast:
    carol at /team_a/forecast: r
    admin at /team_a/forecast: rwxlop

Adding a team member
====================

A new member must first be admitted by the server attribute, before a suite can name the member. The server attribute
is updated as a whole, by a user holding ``p`` at the server:

.. code-block:: shell

    # as admin
    ecflow_client --permissions update / admin:rwxlops,alice:r,amy:r,andy:r,bob:r,carol:rs,ops1:r
    # as alice
    ecflow_client --permissions update /team_a alice:rwxp,amy:rwx,andy:rwx,ops1:rx

``amy`` then holds ``rwx`` on ``/team_a``.

Handing a suite over
====================

A suite is handed over in two steps, so that a user holding ``p`` remains at each moment: the current lead adds the
new lead, who then removes the former one.

.. code-block:: shell

    # as bob
    ecflow_client --permissions update /team_b andy:rwxp,bob:rwxp,ops1:rx
    # as andy
    ecflow_client --permissions update /team_b andy:rwxp,ops1:rx

Notes:

- A change that would leave a node, where some user held ``p``, without any user holding ``p`` is rejected. On this
  server, ``admin`` holds ``p`` everywhere through its sticky entry, so the rule is always met.
- Removing a user from an attribute also removes that user from the attributes below it; an attribute left empty is
  removed, and its node then inherits from above (see :ref:`permissions_attribute_changes`).
- The clients of the former lead lose the suite at their next synchronisation.

Each change is recorded in the server log, with its requester:

.. code-block:: text

    LOG:[10:50:03 18.11.2026]  permissions /team_a (none) -> alice:rwxp,andy:rwx,ops1:rx by admin@host
    LOG:[10:50:03 18.11.2026]  permissions /team_a/archive (none) -> alice:rwxp,ops1:r by admin@host
    LOG:[10:50:03 18.11.2026]  permissions /team_b (none) -> bob:rwxp,ops1:rx by admin@host
    LOG:[10:50:04 18.11.2026]  permissions /team_a/forecast (none) -> alice:rwxp,ops1:rx by alice@host
    LOG:[10:50:05 18.11.2026]  permissions / admin:rwxlops,alice:r,andy:r,bob:r,carol:rs,ops1:r -> admin:rwxlops,alice:r,amy:r,andy:r,bob:r,carol:rs,ops1:r by admin@host
    LOG:[10:50:05 18.11.2026]  permissions /team_a alice:rwxp,andy:rwx,ops1:rx -> alice:rwxp,amy:rwx,andy:rwx,ops1:rx by alice@host
    LOG:[10:50:05 18.11.2026]  permissions /team_b bob:rwxp,ops1:rx -> andy:rwxp,bob:rwxp,ops1:rx by bob@host
    LOG:[10:50:05 18.11.2026]  permissions /team_b andy:rwxp,bob:rwxp,ops1:rx -> andy:rwxp,ops1:rx by andy@host

``ecflow_client --stats`` counts the denied requests:

.. code-block:: text

    Authorisation
      Mode                            permissions
      Denied --alter                  1
      Denied --suites                 1
      Denied --suspend                3

Trialling with audit-only before enforcing
==========================================

The audit-only mode executes the requests the attributes deny, except reads, and records them in the server log. It
is selected in ``<host>.<port>.ecf.cfg``, next to the server attribute:

.. code-block:: json

    {"version": 1, "ecflow": {"server": {
      "permissions": [{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}],
      "authorisation": {"audit_only": true}}}}

Steps:

#. configure the attributes, with ``audit_only`` set to ``true``, and restart the server;
#. let the users work as usual;
#. find the requests the configuration would refuse: ``ecflow_client --log=get | grep 'Audit only'``, and the
   ``Audited`` lines of ``ecflow_client --stats``;
#. correct the attributes, then remove ``audit_only`` and restart the server.

See :ref:`permissions_attribute_audit`.

Migrating from a white list file
================================

In Permissions mode the white list file is not used. Its entries translate as follows:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - White list entry
     - Permissions
   * - ``fred`` (read/write access)
     - a server entry granting the roles ``fred`` needs, at most ``fred:rwxlop`` (the roles of write access)
   * - ``-joe`` (read access)
     - the server entry ``joe:r``
   * - ``user1 /a`` (read/write access to the suite ``/a``)
     - the server entry ``user1:r``, and ``user1`` with the roles needed in the attribute of ``/a``
   * - ``*`` or ``-*`` (every user)
     - no equivalent: every user is named (see :ref:`permissions_attribute_users`)

Steps:

#. write the server attribute, and the attributes of the suites, from the white list file;
#. list the user running the server, if it issues commands: it has no implicit right;
#. trial the configuration in audit-only mode;
#. optionally, set ``/ecflow/server/whitelist/ignore`` to ``true``, so that the file is not loaded.

To return to the white list file, remove the server attribute and every node attribute, as described in
:ref:`permissions_attribute_unrestricted`.

Recovering access
=================

There is no override: no user, the user running the server included, bypasses the attributes. To keep a way back:

- give the administrators a sticky server entry holding ``p`` (e.g. ``admin:rwxlops``): such a user changes the
  attribute of every node;
- the server refuses a change that leaves nobody holding ``p`` where someone held it;
- when an attribute can no longer be changed by anyone (e.g. after a restore of archived nodes), add a sticky entry
  holding ``p`` to the server attribute, as described in :ref:`permissions_attribute_restore`.

When a request is denied
========================

A denied request is reported distinctly from any other error:

- ``ecflow_client`` exits with status ``3``;
- the Python client raises :py:class:`ecflow.PermissionDenied`;
- the message names the missing role and the node, e.g. ``role x required at /team_a``.

To find out what is missing:

- ``ecflow_client --permissions effective <path>`` prints the roles held at the path;
- the Info panel of ecFlowUI shows the line ``permissions :`` (see :ref:`viewing_and_editing_permissions`);
- the users holding ``p`` at the node, or the administrators, grant the role (see
  :ref:`permissions_attribute_denials`).

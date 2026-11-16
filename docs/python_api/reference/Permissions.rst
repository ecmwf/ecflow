.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

ecflow.Permissions
//////////////////


.. py:class:: Permissions
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

Grants roles on a :term:`node` to the users it lists

A node carries at most one Permissions attribute. Each entry names a user and the letters of the roles
granted: r (read), w (write), x (execute), l (log), o (owner), p (permissions), and the flag s (sticky).
The attribute takes effect only on a server that has a server Permissions attribute.
See :ref:`permissions_attribute` for the meaning of each role.

Constructor::

   Permissions(value)
      value: one of
         - a string in the command-line form, e.g. 'alice:rwxs,bob:r'
         - a string in the JSON form, e.g. '[{"user": "alice", "permissions": "rwxs"}]'
         - a list or tuple of entries, e.g. ['alice:rwxs', 'bob:r']
         - a Permissions object, which is copied

Exceptions:

- Raises RuntimeError for an invalid value: no entry, an invalid user name, a user listed twice,
  an unknown or repeated letter, or an entry granting no role
- Raises TypeError for a value of any other type, or a list or tuple holding an item that is not a
  string

Methods:

- entries(): the entries, each a string <user>:<letters>, sorted by user
- roles(user): the letters of the roles granted to a user, or None
- sticky(user): whether the entry of a user is sticky
- to_cli(), to_json(): the command-line and JSON forms
- add_to(nodes, replace=False): adds the attribute to a set of nodes, all or none

Permissions objects compare equal when they hold the same entries, and can be hashed and copied.
repr() returns an expression that recreates the object, e.g. "Permissions('bob:r')".

Usage:

.. code-block:: python

   p = Permissions('alice:rwxp,bob:r')
   p.entries()        # ['alice:rwxp', 'bob:r']
   p.roles('bob')     # 'r'
   str(p)             # 'permissions [{"user": "alice", ...}]', the line in a definition file
   s = Suite('s', p, Family('f', Permissions('bob:r')))
   s.get_permissions() == p   # True


.. py:method:: Permissions.add_to(self: ecflow.Permissions, nodes: collections.abc.Iterable, replace: bool = False) -> None
   :module: ecflow

Adds the attribute to each node of a sequence, all or none

::

   None add_to(nodes, replace=False)
      nodes  : an iterable of nodes
      replace: when True, replaces the attribute of the nodes that already carry one

Exceptions:

- Raises TypeError when an item is not a node
- Raises RuntimeError, when replace is False, if a node already carries a Permissions attribute

Nothing changes when an exception is raised.

Usage:

.. code-block:: python

   Permissions('alice:rx').add_to(defs.get_all_tasks())


.. py:method:: Permissions.entries(self: ecflow.Permissions) -> list
   :module: ecflow

Returns the entries of the attribute

::

   list entries()

Returns a list of strings <user>:<letters>, sorted by user name; the letter s marks a sticky entry.

Usage:

.. code-block:: python

   Permissions('bob:r,alice:rwxs').entries()   # ['alice:rwxs', 'bob:r']


.. py:method:: Permissions.roles(self: ecflow.Permissions, user: str) -> object
   :module: ecflow

Returns the letters of the roles the attribute grants to a user

::

   string roles(
      string user : The user name
   )

Returns the letters in canonical order (r, w, x, l, o, p), without the sticky flag, or None when the
attribute does not list the user. The roles a user actually holds at a node also depend on the
attributes above it: see :py:meth:`ecflow.Client.effective_permissions`.

Usage:

.. code-block:: python

   p = Permissions('alice:rwxs,bob:r')
   p.roles('alice')   # 'rwx'
   p.roles('carol')   # None


.. py:method:: Permissions.sticky(self: ecflow.Permissions, user: str) -> bool
   :module: ecflow

Returns whether the entry of a user is sticky

::

   bool sticky(
      string user : The user name
   )

Returns True when the attribute lists the user with the flag s; False otherwise, including when the
user is not listed.

Usage:

.. code-block:: python

   Permissions('alice:rwxs,bob:r').sticky('alice')   # True


.. py:method:: Permissions.to_cli(self: ecflow.Permissions) -> str
   :module: ecflow

Returns the command-line form of the attribute

::

   string to_cli()

Returns the entries separated by commas, sorted by user, as accepted by the constructor and by
ecflow_client --permissions.

Usage:

.. code-block:: python

   Permissions(['bob:r', 'alice:rwxs']).to_cli()   # 'alice:rwxs,bob:r'


.. py:method:: Permissions.to_json(self: ecflow.Permissions) -> str
   :module: ecflow

Returns the JSON form of the attribute, as written in definition files

::

   string to_json()

Returns a compact JSON array of objects with the keys user and permissions, sorted by user, as accepted
by the constructor.

Usage:

.. code-block:: python

   Permissions('bob:r').to_json()   # '[{"permissions":"r","user":"bob"}]'

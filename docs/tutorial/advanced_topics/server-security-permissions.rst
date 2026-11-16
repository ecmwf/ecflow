.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: server security  (tutorial)
   single: permissions (tutorial)

.. _tutorial-server-security-permissions:

Server security - permissions attribute
=======================================

The :term:`permissions` attribute grants roles to the users it lists: ``r`` (read), ``w`` (write), ``x`` (execute),
``l`` (log), ``o`` (owner) and ``p`` (permissions). Unlike the white list file, the attribute can be given to any
:term:`node`, so that access is granted per :term:`suite`, :term:`family` or :term:`task`.

The node attributes take effect only when the server itself has an attribute, the *server attribute*, which lists
every user admitted to the server. It is set in the server configuration file ``<host>.<port>.ecf.cfg``, read when
the server starts.

.. note::

    The user running the server has no implicit right: list it in the server attribute, as any other user.
    Otherwise, the server no longer accepts the commands of that user.

Server configuration
--------------------

Create the file ``<host>.<port>.ecf.cfg`` in the directory where the server is started (with
``ecflow_start.sh``, the ``ECF_HOME`` of the server, by default ``$HOME/ecflow_server``), with the host name and the
port of the server, e.g. ``machine1.3141.ecf.cfg``. Replace ``myuid`` with the name of the tutorial user; ``guest`` stands for a
second user, allowed only to read the server:

.. code-block:: json
    :caption: <host>.<port>.ecf.cfg

    {
      "version": 1,
      "ecflow": {
        "server": {
          "permissions": [{"user": "myuid", "permissions": "rwxlop"}, {"user": "guest", "permissions": "r"}]
        }
      }
    }

Restart the server, so that it reads the file. The start-up lines of the server log then state:

.. code-block:: text

    MSG:[10:50:32 18.11.2026] Permissions:
    MSG:[10:50:32 18.11.2026]   Mode = permissions

A :term:`suite` without attribute takes the roles of the server attribute: ``myuid`` holds ``rwxlop`` on the suite
``test``, and ``guest`` holds ``r``.

Suite Definition
----------------

Give :term:`family` ``f1`` an attribute that grants ``myuid`` the roles ``r``, ``x`` and ``p``, but not ``w``. Below a
suite, an attribute can only restrict the roles inherited; ``guest``, not listed, loses every role on ``f1``.

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file, as follows:

        .. code-block:: shell

           # Definition of the suite test.
           suite test
            edit ECF_INCLUDE "$HOME/course"
            edit ECF_HOME    "$HOME/course"

            family f1
                permissions [{"user": "myuid", "permissions": "rxp"}]
                edit SLEEP 20
                task t1
                task t2
            endfamily
           endsuite

    .. tab:: Python

        .. literalinclude:: src/server-security-permissions.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Create the server configuration file, with the name of the tutorial user, and restart the server.
#. Apply the changes to the :term:`suite definition`, and replace the suite ``/test`` in the server.
#. Show the roles each user holds on ``f1``:

   .. code-block:: shell

      ecflow_client --permissions effective /test/f1          # rxp
      ecflow_client --permissions effective /test/f1 guest    # nothing: guest holds no role on f1
      ecflow_client --permissions effective /test guest       # r

#. Try to change a variable of ``f1``. The server denies the request, since it requires the role ``w``, and
   ``ecflow_client`` exits with status ``3``:

   .. code-block:: shell

      ecflow_client --alter change variable SLEEP 5 /test/f1
      echo $?

   .. code-block:: text

      Error: request( --alter change variable SLEEP 5 /test/f1 ) failed! Server reply: Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role w required at /test/f1) [--alter change variable SLEEP 5 /test/f1 :myuid@machine1]
      3

#. Grant ``myuid`` the role ``w`` on ``f1``, using the role ``p`` it holds there, and change the variable again:

   .. code-block:: shell

      ecflow_client --permissions update /test/f1 myuid:rwxp
      ecflow_client --alter change variable SLEEP 5 /test/f1

#. In :term:`ecflow_ui`

   * Observe the row of the attribute of ``f1``, and its tooltip
   * Open the Info panel of ``f1``, and read the line ``permissions :``, listing the roles of every user there

For more information see :ref:`permissions_attribute`, :ref:`permissions_cli` and
:ref:`how_to_control_access_with_the_permissions_attribute`.

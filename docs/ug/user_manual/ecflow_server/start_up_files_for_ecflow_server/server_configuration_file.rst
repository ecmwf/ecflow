.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _server_configuration_file:

<host>.<port>.ecf.cfg
/////////////////////

The configuration file ``<host>.<port>.ecf.cfg`` configures one server, identified by its host and port.
It can provide every setting of the server, and can be used together with, or instead of,
:ref:`server_environment.cfg`.

.. important::

    ``<host>.<port>.ecf.cfg`` is the preferred way to configure the server. It provides every setting in one
    file, with typed values, and is validated strictly at start-up.

Location and name
=================

The server looks for the file in the startup directory, the current working directory when
``ecflow_server`` is invoked, before changing to ``ECF_HOME``. Its name is made of:

- ``<host>``: the host name of the machine;
- ``<port>``: the port of the server, given by the built-in default, ``server_environment.cfg``,
  the environment variable ``ECF_PORT`` and the option ``--port``, in increasing precedence.

For example, a server started with ``--port=3142`` on the machine ``machine1`` reads
``machine1.3142.ecf.cfg``. The files of other ports are ignored.

The file is optional; without it, the server is configured as described in :ref:`server_configuration`,
without this origin. The file is read once, at start-up; a change takes effect at the next start of the server.

The start-up lines of the :ref:`log file <ecflow_log_file>` state whether the file was loaded, and which settings
it provides.

Precedence
==========

The file overrides ``server_environment.cfg``, and is overridden by the environment variables and the
command-line options. From lowest to highest precedence, the origins are:

#. the built-in defaults;
#. ``server_environment.cfg``;
#. ``<host>.<port>.ecf.cfg``;
#. the environment variables;
#. the command-line options.

Format
======

The file is a JSON document, holding an object with the following members:

- ``version``: the version of the format, ``1``; required.
- ``ecflow``: an object holding a ``server`` object, which holds:

  - ``variables``: an object holding the server variables, each under its name, e.g. ``ECF_HOME``;
  - ``http``, ``ipv6``, ``ssl``, ``job_generation``: the options of the server, as booleans;
  - ``log``: the extra information written to the :ref:`log file <ecflow_log_file>`, available only in this file.

For example:

.. code-block:: json

    {
      "version": 1,
      "ecflow": {
        "server": {
          "variables": {
            "ECF_HOME": "/home/ecflow/server",
            "ECF_LOG": "server.log",
            "ECF_CHECKINTERVAL": 300,
            "ECF_CHECKMODE": "CHECK_ON_TIME",
            "ECF_DEBUG_SERVER": false
          },
          "http": true
        }
      }
    }

The options correspond to the command-line options as follows:

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Member
     - Command-line option
     - Value
   * - ``http``
     - ``--http``
     - ``true`` selects HTTP
   * - ``ipv6``
     - ``--v6``
     - ``true`` selects IPv6
   * - ``ssl``
     - ``--ssl``
     - ``true`` enables SSL, looking for the shared certificate first
   * - ``job_generation``
     - ``--dis_job_gen``
     - ``false`` disables job generation, for testing only

The ``log`` member holds:

- ``commands``: an object holding ``enabled``, a boolean, ``false`` by default; ``true`` records the spawn, the
  context and the termination of ``ECF_JOB_CMD``, ``ECF_KILL_CMD`` and ``ECF_STATUS_CMD``, see
  :ref:`ecflow_log_file_commands`.

For example:

.. code-block:: json

    {
      "version": 1,
      "ecflow": {
        "server": {
          "log": {
            "commands": { "enabled": true }
          }
        }
      }
    }

The settings, their defaults and their accepted values are listed in :ref:`server_configuration`.

Values
======

Each value has the native JSON type of its setting:

- **Integer:** a JSON integer, e.g. ``300``; not a string, nor a number with a fraction or an exponent.
- **Text:** a JSON string, e.g. ``"server.log"``.
- **Boolean:** ``true`` or ``false``, e.g. for ``ECF_DEBUG_SERVER`` and the options.

The file can also state the host and the port of its server, which must then match its name:

- ``ECF_HOST``: the host name of the machine;
- ``ECF_PORT``: the port of the server.

Validation
==========

The file is read strictly. Any of the following stops start-up, before the log is created, with a
diagnostic naming the file, the entry as a JSON Pointer, and the reason:

- a path that is not a readable regular file, e.g. a directory;
- a path that cannot be inspected, e.g. in a start-up directory without search permission;
- content that is not a single JSON document, or holds a member more than once;
- a missing or unsupported ``version``;
- an unknown member;
- a value of the wrong type, or refused by its setting, e.g. an empty ``ECF_HOME``;
- ``ECF_HOST`` or ``ECF_PORT`` other than the host or the port of the file name.

An invalid value is refused even when an origin of higher precedence also provides the setting.
For example:

.. code-block:: text

    machine1.3142.ecf.cfg: /ecflow/server/variables/ECF_PORT: invalid value 3141: expected the port of the file name, 3142

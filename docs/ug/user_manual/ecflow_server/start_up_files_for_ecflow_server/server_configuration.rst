.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _server_configuration:

Server configuration
////////////////////

This page lists every setting of the ecFlow server that can be configured at start-up, with its
default and the origins that can provide it.

Origins and precedence
======================

A setting takes its value from the origin of highest precedence that provides it. From lowest to
highest precedence, the origins are:

#. the built-in defaults;
#. :ref:`server_environment.cfg`, read from the directory in which the server starts;
#. :ref:`server_configuration_file`, read from the same directory, when it exists;
#. the environment variables;
#. the command-line options, for the settings that have one (see :ref:`ecflow_server_initialisation`).

Each origin can provide only the settings listed for it in the table below; ``<host>.<port>.ecf.cfg``
can provide every setting, as described in :ref:`server_configuration_file`. An unknown setting in
either file, or an unknown command-line option, stops start-up; an environment variable not listed for
a setting is ignored.

Validation
==========

Each value is checked when its origin is read, whatever the origin:

- An invalid value stops start-up, before the log file is created.
- An invalid value is refused even when an origin of higher precedence also provides the setting.
- The diagnostic names the setting as its origin names it, the value, and the reason, e.g.

  - ``Environment variable ECF_PORT: invalid value '80': expected a value in [1024, 49150]``;
  - ``Invalid server command line: --port: invalid value '80': expected a value in [1024, 49150]``.

The values are written as follows:

- **Integer:** decimal digits, optionally preceded by ``-``, without spaces or a ``+`` sign; on the
  command line, a leading ``+`` is also accepted.
- **Text:** any characters; leading and trailing spaces are removed in ``server_environment.cfg``.
- **Switch:** a command-line option without a value, e.g. ``--http``; giving it sets the setting.

Settings
========

.. list-table:: Server settings
   :header-rows: 1
   :widths: 18 22 10 10 10 30

   * - Setting
     - Default
     - In ``server_environment.cfg``
     - Environment variable
     - Command-line option
     - Accepted values
   * - ``ECF_HOME``
     - ``.`` (the directory in which the server starts)
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_PORT``
     - ``3141``
     - yes
     - yes
     - ``--port``
     - integer from 1024 to 49150
   * - ``ECF_CHECK``
     - ``<host>.<port>.ecf.check``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_CHECKOLD``
     - ``<host>.<port>.ecf.check.b``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_CHECKINTERVAL``
     - ``120`` (seconds)
     - yes
     - yes
     - no
     - integer from 1 to 2147483647
   * - ``ECF_CHECKMODE``
     - ``CHECK_ON_TIME``
     - yes
     - no
     - no
     - ``CHECK_NEVER``, ``CHECK_ON_TIME`` or ``CHECK_ALWAYS``
   * - ``ECF_LOG``
     - ``<host>.<port>.ecf.log``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_LISTS``
     - ``<host>.<port>.ecf.lists``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_PASSWD``
     - ``<host>.<port>.ecf.passwd``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_CUSTOM_PASSWD``
     - ``<host>.<port>.ecf.custom_passwd``
     - yes
     - yes
     - no
     - non-empty text
   * - ``ECF_TASK_THRESHOLD``
     - ``4000`` (milliseconds)
     - yes
     - yes
     - no
     - integer from 1 to 2147483647
   * - ``ECF_PRUNE_NODE_LOG``
     - ``30`` (days)
     - yes
     - yes
     - no
     - integer from 0 to 2147483647; ``0`` preserves all edit history
   * - ``ECF_INTERVAL``
     - ``60`` (seconds)
     - yes
     - no
     - ``--ecfinterval``
     - integer from 1 to 60
   * - ``ECF_JOB_CMD``
     - ``%ECF_JOB% 1> %ECF_JOBOUT% 2>&1``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_KILL_CMD``
     - ``kill -15 %ECF_RID%``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_STATUS_CMD``
     - ``ps --pid %ECF_RID% -f > %ECF_JOB%.stat 2>&1``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_CHECK_CMD``
     - ``ps --pid %ECF_RID% -f``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_URL_CMD``
     - ``${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_URL_BASE``
     - ``https://confluence.ecmwf.int``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_URL``
     - ``display/ECFLOW/ecflow+home``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_MICRODEF``
     - ``%``
     - yes
     - no
     - no
     - non-empty text
   * - ``ECF_SSL``
     - none: SSL disabled
     - no
     - yes
     - see ``--ssl`` below
     - any text, see :ref:`open_ssl`
   * - ``ECF_DEBUG_SERVER``
     - debugging disabled
     - no
     - yes
     - ``--debug``, ``-d``
     - any text: defining the variable enables debugging; ``--debug`` enables it too
   * - ``ECF_HOST``
     - the host name of the machine
     - no
     - no
     - no
     - only in ``<host>.<port>.ecf.cfg``: the host name of the machine
   * - HTTP transport
     - disabled: the ecFlow protocol over TCP/IP
     - no
     - no
     - ``--http``
     - switch: clients connect over HTTP, or HTTPS when SSL is enabled
   * - IPv6
     - disabled: IPv4
     - no
     - no
     - ``--v6``
     - switch: the server listens on IPv6
   * - SSL
     - disabled
     - no
     - no
     - ``--ssl``
     - switch: enables SSL, looking for the shared certificate first; ``ECF_SSL``, when defined, decides
       the certificate lookup order instead, see :ref:`open_ssl`
   * - Job generation
     - enabled
     - no
     - no
     - ``--dis_job_gen``
     - switch: disables job generation, for testing only

Notes:

- The defaults written ``<host>.<port>.<name>`` are the default filenames prefixed with the host
  and port of the server. An explicitly set filename equal to the default filename, e.g.
  ``ECF_LOG = ecf.log``, is prefixed in the same way.
- Relative paths are resolved as described in :ref:`start_up_files_for_ecflow_server`.
- A server built without SSL support refuses ``ECF_SSL`` and any request for SSL, whatever the origin, before
  creating its log.

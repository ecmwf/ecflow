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
#. the environment variables;
#. the command-line options, for the few settings that have one (see :ref:`ecflow_server_initialisation`).

Each origin can provide only the settings listed for it in the table below. An unknown setting in
``server_environment.cfg`` stops start-up; an environment variable not listed for a setting is ignored.

Validation
==========

Each value is checked when its origin is read, whatever the origin:

- An invalid value stops start-up, before the log file is created.
- An invalid value is refused even when an origin of higher precedence also provides the setting.
- The diagnostic names the setting as its origin names it, the value, and the reason, e.g.
  ``Environment variable ECF_PORT: invalid value '80': expected a value in [1024, 49150]``.

The values are written as follows:

- **Integer:** decimal digits, optionally preceded by ``-``, without spaces or a ``+`` sign.
- **Text:** any characters; leading and trailing spaces are removed in ``server_environment.cfg``.

Settings
========

.. list-table:: Server settings
   :header-rows: 1
   :widths: 20 25 10 10 35

   * - Setting
     - Default
     - In ``server_environment.cfg``
     - Environment variable
     - Accepted values
   * - ``ECF_HOME``
     - ``.`` (the directory in which the server starts)
     - yes
     - yes
     - non-empty text
   * - ``ECF_PORT``
     - ``3141``
     - yes
     - yes
     - integer from 1024 to 49150
   * - ``ECF_CHECK``
     - ``<host>.<port>.ecf.check``
     - yes
     - yes
     - non-empty text
   * - ``ECF_CHECKOLD``
     - ``<host>.<port>.ecf.check.b``
     - yes
     - yes
     - non-empty text
   * - ``ECF_CHECKINTERVAL``
     - ``120`` (seconds)
     - yes
     - yes
     - integer from 1 to 2147483647
   * - ``ECF_CHECKMODE``
     - ``CHECK_ON_TIME``
     - yes
     - no
     - ``CHECK_NEVER``, ``CHECK_ON_TIME`` or ``CHECK_ALWAYS``
   * - ``ECF_LOG``
     - ``<host>.<port>.ecf.log``
     - yes
     - yes
     - non-empty text
   * - ``ECF_LISTS``
     - ``<host>.<port>.ecf.lists``
     - yes
     - yes
     - non-empty text
   * - ``ECF_PASSWD``
     - ``<host>.<port>.ecf.passwd``
     - yes
     - yes
     - non-empty text
   * - ``ECF_CUSTOM_PASSWD``
     - ``<host>.<port>.ecf.custom_passwd``
     - yes
     - yes
     - non-empty text
   * - ``ECF_TASK_THRESHOLD``
     - ``4000`` (milliseconds)
     - yes
     - yes
     - integer from 1 to 2147483647
   * - ``ECF_PRUNE_NODE_LOG``
     - ``30`` (days)
     - yes
     - yes
     - integer from 0 to 2147483647; ``0`` preserves all edit history
   * - ``ECF_INTERVAL``
     - ``60`` (seconds)
     - yes
     - no
     - integer from 1 to 60
   * - ``ECF_JOB_CMD``
     - ``%ECF_JOB% 1> %ECF_JOBOUT% 2>&1``
     - yes
     - no
     - non-empty text
   * - ``ECF_KILL_CMD``
     - ``kill -15 %ECF_RID%``
     - yes
     - no
     - non-empty text
   * - ``ECF_STATUS_CMD``
     - ``ps --pid %ECF_RID% -f > %ECF_JOB%.stat 2>&1``
     - yes
     - no
     - non-empty text
   * - ``ECF_CHECK_CMD``
     - ``ps --pid %ECF_RID% -f``
     - yes
     - no
     - non-empty text
   * - ``ECF_URL_CMD``
     - ``${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%``
     - yes
     - no
     - non-empty text
   * - ``ECF_URL_BASE``
     - ``https://confluence.ecmwf.int``
     - yes
     - no
     - non-empty text
   * - ``ECF_URL``
     - ``display/ECFLOW/ecflow+home``
     - yes
     - no
     - non-empty text
   * - ``ECF_MICRODEF``
     - ``%``
     - yes
     - no
     - non-empty text
   * - ``ECF_SSL``
     - none: SSL disabled
     - no
     - yes
     - any text, see :ref:`open_ssl`
   * - ``ECF_DEBUG_SERVER``
     - debugging disabled
     - no
     - yes
     - any text: defining the variable enables debugging

Notes:

- The defaults written ``<host>.<port>.<name>`` are the default filenames prefixed with the host
  and port of the server. An explicitly set filename equal to the default filename, e.g.
  ``ECF_LOG = ecf.log``, is prefixed in the same way.
- Relative paths are resolved as described in :ref:`start_up_files_for_ecflow_server`.

.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _handling_cli_options:

Handling CLI options
********************

Some ecFlow client options, such as the user name and password or the ecFlow server host and port, are used very frequently.

To make it easier to use these options, the ecFlow client allows the user to provide these options using multiple methods, namely:

- most options can be defined by exporting specific environment variables
- all options can be provided on the command line
- some ancillary options can be provided in a configuration file (and activated by an environment variable)

When the user provides the same option using multiple methods described above, the following precedence order is used:

1) The command line option has the highest precedence and will override any other methods
2) The environment variable options have the next highest precedence and will override configuration file options

The configuration file options are handled separately, with specific behaviour described in the relevant section below.


.. rubric:: ecFlow server host and port options

The server host and port options are used by all ecFlow clients to connect to the ecFlow server and can be provided by:

- the environment variables :code:`ECF_HOST` and :code:`ECF_PORT` (one or both, can be provided)
- the command line options :code:`--host` and :code:`--port`, which override the related environment variables

The user can also export the environment variable :code:`ECF_HOSTFILE`, and define a list of :ref:`backup servers <using_backup_servers>` to be used.

.. important::

  Notice that the content of the :code:`ECF_HOSTFILE` file does not override the main host and port, but provides a
  list of alternative hosts and ports to try if the main host is not available.

.. note::

  When the environment variable :code:`ECF_HOSTFILE` is defined, after failing to contact the provided main host and port,
  the ecFlow client will attempt to connect to the alternative ecFlow servers.

  This behaviour is default for all ecFlow task commands, and can be explicitly activated also for user commands by
  exporting the environment variable :code:`ECF_HOSTFILE_POLICY` and setting it to :code:`all`.




.. rubric:: SSL certificate directory

For SSL connections, ``ecflow_client`` uses ``$HOME/.ecflowrc/ssl/`` by default.
``ECF_SSL_DIR`` selects another directory, with or without a trailing ``/``.

Use ``ECF_SSL=X`` to try the server-specific certificate first, falling back to
``server.crt`` if it is absent. This is the recommended mode and matches
certificate selection in :term:`ecflow_ui`.

Shared certificates should be avoided. Use ``ECF_SSL=1`` only for configurations
that intentionally share a certificate; its existing behaviour is retained for
compatibility. Certificate selection depends on ``ECF_SSL``:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Value
     - First certificate
     - Fallback if absent

   * - Any other value, such as ``X``
     - ``<host>.<port>.crt``
     - ``server.crt``

   * - ``1`` or empty
     - ``server.crt``
     - ``<host>.<port>.crt``

Example using a server-specific certificate:

.. code-block:: shell

    ECF_SSL_DIR="/path/to/certificates" ECF_SSL=X \
        ecflow_client --host server.example --port 3141 --ping

Enable SSL with ``ECF_SSL`` or :ref:`--ssl <ssl_cli>`; ``ECF_SSL_DIR`` only
selects the directory. ``ECF_SSL`` values select a mode, not a certificate path.

Add ``-d`` to report how SSL was enabled and the requested certificate order.
When ``ECF_SSL`` is set, the output includes its value.

Before the certificate is loaded, the debug output reports its absolute path
and identifies whether it is shared or server-specific.

The directory override also applies to :term:`ecflow_ui`, which selects
certificates per server. See :ref:`open_ssl` for certificate setup.


.. rubric:: Option Overriding Summary

The table below summarises the options that can be provided using environment variables and command line options.

.. list-table:: Summary of `ecflow_client` options that can be provided using environment variables and command line options
    :header-rows: 1
    :width: 100%
    :widths: 20 40 40
    :name: ecflow_client_options_overriding

    * - Option
      - Environment Variable
      - Command Line Option

    * - User name
      - :code:`ECF_USER`
      - :code:`--user`

    * - Password
      - :code:`ECF_PASS`
      - :code:`--pass`

    * - Server host
      - :code:`ECF_HOST`
      - :code:`--host`

    * - Server port
      - :code:`ECF_PORT`
      - :code:`--port`

    * - SSL
      - :code:`ECF_SSL`
      - :code:`--ssl`

    * - SSL certificate directory
      - :code:`ECF_SSL_DIR`
      - No command-line option

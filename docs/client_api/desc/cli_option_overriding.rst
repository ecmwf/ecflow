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

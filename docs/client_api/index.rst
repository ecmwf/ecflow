
.. _ecflow_cli:

Command line interface (CLI)
//////////////////////////// 

The :term:`ecFlow command line interface (CLI) <ecflow_client>` is provided by the :code:`ecflow_client` executable.
A large number of commands/options enabled by the :ref:`ecflow_ui` are also available as CLI commands.

:code:`ecflow_client` accepts a variety of commands, specified as ``--<command>``.
For example, the command :code:`--load` can be used, as in the example below, to load the given file into the server.

.. code-block:: shell

    ecflow_client --load host1.3141.check

The comprehensive :ref:`list of ecflow_client commands <ecflow_client_commands>` is presented below.
These commands can be combined with :ref:`ecflow_client common options <ecflow_client_options>` to further customise the
:term:`ecflow_client` behaviour.

.. rubric:: Getting help

The list of commands, amongst other details, can be displayed by using the option ``--help``.

.. code-block:: shell

    ecflow_client --help


.. toctree::
    :maxdepth: 1
    
    desc/cli_option_overriding
    desc/cli_scripting_in_batch
    desc/using_backup_servers

----

.. toctree::
    :maxdepth: 1

    cli_commands
    cli_options
    command_internals

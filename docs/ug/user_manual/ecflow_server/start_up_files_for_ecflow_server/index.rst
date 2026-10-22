.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _start_up_files_for_ecflow_server:

Start-up files for ecFlow server
////////////////////////////////

The ecFlow server uses files for configuration and reporting.

Initialisation distinguishes two directory roles:

- **Startup directory:** the current working directory when ``ecflow_server`` is invoked.
  The server reads :ref:`server_environment.cfg` here, before changing directory.
- **Server home:** the directory selected by :term:`ECF_HOME` from the configuration file or
  an :ref:`environment override <ecflow_server_environment_variables>`.
  The default, ``.``, means the startup directory. A relative home is resolved against that
  directory before runtime filenames are constructed.

The server changes into ``ECF_HOME`` before opening the log and loading the password and white list files.

Relative paths for these files are resolved against ``ECF_HOME``.
Relative checkpoint paths are anchored there before the directory change; absolute file paths
are used unchanged.

``ECF_HOME`` must exist and be accessible and writable by the account running the server;
otherwise, initialisation fails.

See :ref:`ecflow_server_initialisation` for configuration precedence, the complete loading
sequence and an example with separate startup and server-home directories.

.. toctree::
    :maxdepth: 1

    ecflow_log_file
    ecflow_check_point_file_and_failure_tolerance
    server_environment.cfg

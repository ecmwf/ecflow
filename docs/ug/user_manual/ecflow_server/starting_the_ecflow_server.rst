.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _starting_the_ecflow_server:

Starting the ecFlow Server
//////////////////////////

The executable ecflow_server is used to start an ecflow_server using the
default port number or that defined by the environment variable
ECF_PORT.

.. code-block:: shell

    cd ECF_dir1
    nohup ecflow_server & # start ecFlow with **default** port 3141

Multiple ECFs can be run on the same host using different port numbers.
There are two mechanisms for specifying the port number:

-  Using arguments on the command line. i.e. ecflow_server --port=3141

-  Using the Environment variable. ECF_PORT

If both are specified the command line argument takes precedence

.. code-block:: shell

   cd ../ECF_dir2
   ecflow_server –-port=3142& # start ecFlow with port number 3142
   cd ../ECF_dir3
   export ECF_PORT=3143
   ecflow_server &            # starts ecFlow with port number 3143

**Note:** the ECFs are started in different directories so that the
output and checkpoint files are not overwritten

.. rubric:: ecflow_ui configuration

Adding a new server to ecflow_ui adds the definition to the file::

  ~/.ecflowrc/servers.

This can be modified directly.

.. rubric:: Start-up scripts

A start-up script, :code:`ecflow_start.sh`, is a useful way of starting and configuring ecFlow server.
An example start-up script is included in the default installation of ecFlow. A start-up script can be used to:

- configure some of the default ecFlow variables
- check that an ecFlow server is not already running (by using :code:`ecflow_client --ping`)
- set the appropriate environment (or do it in .profile or .cshrc)
- backup previous log files and checkpoint files
- and, eventually, launch :code:`ecflow_server` in the background, and alert operators if a problem occurs while starting the server.

Two ecFlow servers cannot be started on the same host machine with the same port number.
To allow users that want to run multiple ecFlow servers on the same host machine, the script
:code:`ecflow_start.sh` can choose a difference port number based on the unique user ID.

Which ports are being used can be checked with netstat. To list
all open network ports on your machine, consider running the following command:

.. code-block:: shell

   netstat -lnptu

Here is a breakdown of the parameters:

-  l - List all listening ports

-  n - Display the numeric IP addresses (i.e., don't do reverse DNS
   lookups

-  p - List the process name that is attached to that port

-  t - List all TCP connections

-  u - List all UDP connections

When using non-default ecFlow servers, ecflow_ui needs to be configured
to recognise the port used. This can be done in the Manage servers dialog.

.. figure:: /_static/ug/starting_the_ecflow_server/image1.png
   :width: 4.02282in
   :height: 2.60417in

   Viewing new ECFLOW servers with ecflow_ui


.. _ecflow_server_initialisation:

Server initialisation
---------------------

Command-line syntax is parsed before server settings are loaded. ``--help`` (``-h``)
and ``--version`` (``-v``) print the requested information and exit successfully
without reading the configuration file, parsing environment settings, accessing
``ECF_HOME``, loading security files, enabling SSL, creating logs or opening a listener.
This also applies when server settings are invalid. Invalid command-line syntax,
missing or invalid numeric arguments, and repeated numeric options still fail;
informational options do not conceal command-line errors.

Numeric options accept both ``--port=3141`` and ``--port 3141`` forms (and likewise
for ``--ecfinterval``). Repeating either numeric option is rejected, rather than
selecting one of the supplied values.

For a normal start, ``ecflow_server`` performs the following steps in order:

#. **Read the configuration file.** The server reads :ref:`server_environment.cfg`
   from the directory in which the process starts, before changing to ``ECF_HOME``.
   The filename is fixed, and no environment variable or command-line option allows
   selecting another file.
   A parsing error stops initialisation before environment overrides are applied;
   the diagnostic identifies the configuration file and the original parsing error.

#. **Apply overrides.** Environment variables override settings in the configuration file.
   Command-line options override the corresponding environment settings; for example,
   ``--port`` overrides ``ECF_PORT``. The precedence, from lowest to highest, is:

   - built-in defaults;
   - settings in ``server_environment.cfg``;
   - environment variables;
   - command-line options, where available.

#. **Resolve the filenames.** The server stores the resolved paths in its internal
   configuration and later exposes them as the following ecFlow server variables:

   - ``ECF_CHECK``: the checkpoint file;
   - ``ECF_CHECKOLD``: the backup checkpoint file;
   - ``ECF_PASSWD``: the password file;
   - ``ECF_CUSTOM_PASSWD``: the custom password file;
   - ``ECF_LISTS``: the white list file.

   When an effective value is still the bare default filename, the server prefixes it with
   ``<host>.<port>``, using the final port selected above. This also applies when the default
   filename was explicitly set in the configuration file or environment. Non-default filenames
   are not prefixed. Relative checkpoint paths are additionally prepended with ``ECF_HOME``;
   relative security-file paths remain relative and are opened after the directory change.
   These adjustments do not modify the process environment or the configuration file.

#. **Change the working directory to** ``ECF_HOME``. The default value, ``.``, means the
   directory in which the process started. A relative ``ECF_HOME`` is converted to an absolute
   path against that startup directory before filenames are resolved or the working directory changes.
   The resolved value is exposed as the ecFlow server variable ``ECF_HOME``.
   The directory must exist and allow read, write
   and search access for the account running the server; otherwise, initialisation fails.

#. **Open the log.** The server applies the same host and port prefix rule to the log filename,
   then opens the log and records the start time, version and effective runtime configuration.
   The resulting path is later exposed as the ecFlow server variable ``ECF_LOG``.
   A relative log path is resolved against ``ECF_HOME``.

#. **Load the security files.** After changing directory, the server loads, in order:

   - the password file selected by ``ECF_PASSWD``;
   - the custom password file selected by ``ECF_CUSTOM_PASSWD``;
   - the white list file selected by ``ECF_LISTS``.

   A missing security file does not prevent startup. An existing file that cannot be parsed
   prevents startup. Each password file, when present, must contain at least one entry
   matching the server host and port. A missing white list imposes no white-list restrictions;
   it does not disable password authentication configured through the password files.

   Existing security paths must name readable regular files; directories and unreadable
   files prevent startup. Empty and version-only password files contain no matching
   credentials and therefore prevent startup. Empty and version-only white lists impose
   no white-list restrictions, just like an absent white list. Regular and custom password
   files protect their respective identity kinds independently; configuring one does not
   supply credentials for the other.

#. **Restore the server definition and start accepting requests.** The server attempts to
   restore ``ECF_CHECK``, then ``ECF_CHECKOLD`` if necessary. With neither file present,
   it starts with an empty definition. The initial server state is ``HALTED``; accepting
   client requests does not itself start job scheduling. See
   :ref:`ecflow_check_point_file_and_failure_tolerance` for checkpoint recovery.

Security-file paths
~~~~~~~~~~~~~~~~~~~

These variables can be set in ``server_environment.cfg`` or in the process environment:

.. list-table:: Server security files
   :header-rows: 1

   * - Variable
     - Default filename after prefixing
   * - ``ECF_PASSWD``
     - ``<host>.<port>.ecf.passwd``
   * - ``ECF_CUSTOM_PASSWD``
     - ``<host>.<port>.ecf.custom_passwd``
   * - ``ECF_LISTS``
     - ``<host>.<port>.ecf.lists``

Path resolution follows these rules:

- Relative paths are resolved against ``ECF_HOME``, not the directory containing
  ``server_environment.cfg`` when those directories differ.
- Absolute paths are used unchanged.
- Setting a variable to its bare default name, such as ``ECF_PASSWD=ecf.passwd``,
  still adds the host and port prefix.
- A different name or a path, such as ``operators.passwd`` or ``./ecf.passwd``,
  is used without a prefix.

For example, a configuration file at ``/srv/ecflow/admin/server_environment.cfg`` can contain:

.. code-block:: text

   ECF_HOME = /srv/ecflow/workspace
   ECF_PASSWD = /srv/ecflow/admin/ecf.passwd
   ECF_CUSTOM_PASSWD = /srv/ecflow/admin/ecf.custom_passwd
   ECF_LISTS = /srv/ecflow/admin/ecf.lists

Start the server from the configuration directory:

.. code-block:: shell

   cd /srv/ecflow/admin
   ecflow_server --port=3141

The server reads the configuration from ``/srv/ecflow/admin``, changes into
``/srv/ecflow/workspace``, and loads the security files from the absolute paths in the
configuration. Environment variables, if set, override those paths.

The configuration file is read only at startup. Security-file contents can be reloaded
with :ref:`reloadpasswdfile_cli`, :ref:`reloadcustompasswdfile_cli` and :ref:`reloadwsfile_cli`.
These commands reuse the paths selected at startup; they do not reread the configuration file
or select new paths.

A successful reload replaces the contents of the selected password database or whitelist.
Updated passwords, added or removed identities, and changed whitelist permissions take effect
on subsequent requests. Reloading one file does not replace the other security databases.

If the selected file has been removed, is unreadable, names a directory or cannot be parsed,
the reload fails and reports the file and cause. Previously active credentials and permissions remain in force;
neither an empty database nor partially parsed replacement entries are installed. Correcting
the original file and retrying the reload does not require restarting the server.

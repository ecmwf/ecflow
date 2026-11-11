.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _version_5.20:

Version 5.20 updates
********************

.. role:: jiraissue
   :class: hidden

.. role:: githubissue
   :class: hidden

.. role:: commitid
   :class: hidden

Version 5.20.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2026-11-12

General
-------

- **Feature** enable Aviso v2 in the :code:`aviso` attribute, with new capabilities: event streaming, the ability to act only on the latest of the pending notifications (:code:`--collapse`), and the ability to requeue a task automatically on completion (:code:`--repeat`) :jiraissue:`ECFLOW-2121` :jiraissue:`ECFLOW-2125` :commitid:`6a6fb14fd` :commitid:`ca281eeb8`

  .. admonition:: Disclaimer

     Aviso v1 is no longer supported, and 5.19.x is its last release.

     The Aviso v1 options :code:`--schema` and :code:`--polling` are rejected in definitions and in
     :code:`--alter` commands, and are silently dropped when a definition is retrieved from a 5.19.x server.

  .. important::

     **Compatibility of 5.19.x clients with 5.20.0 servers**

     ecFlow 5.20.0 clients work with 5.19.x servers, but 5.19.x clients (:code:`ecflow_client`, the Python
     API and ecFlowUI) cannot parse the new Aviso options. A 5.19.x client can no longer retrieve the
     definition of *any* suite from a 5.20.0 server once that server holds an :code:`aviso` attribute that:

     - uses :code:`--collapse` or :code:`--repeat`, or
     - has released its task.

     The client then fails with an error such as :code:`unrecognised option '--collapse'`.

     Upgrade all clients to 5.20.0 before using Aviso attributes on a 5.20.0 server.

- **Feature** allow :code:`--alter change repeat` to set the begin, end, step and current value of a Repeat together :jiraissue:`ECFLOW-1852` :commitid:`84113ee92`
- **Feature** add :code:`--evaluate` to resolve the :code:`%VAR%` references in the value returned by :code:`--query variable` :jiraissue:`ECFLOW-1975` :commitid:`b28ce4b0b`
- **Feature** add Rust bindings for the ecFlow client, with one typed method per command, mirroring the Python client :commitid:`308e12336`

- **Improvement** reduce the default :code:`ECF_TIMEOUT` to 1 hour and :code:`ECF_ZOMBIE_TIMEOUT` to 30 minutes :jiraissue:`ECFLOW-2050` :commitid:`33a2f870a`
- **Improvement** adjust the SSL certificate selection, so that both server and client prefer, by default, the server-specific certificate :code:`<host>.<port>.crt`, when available, to the shared :code:`server.crt` :jiraissue:`ECFLOW-2129` :jiraissue:`ECFLOW-1989` :commitid:`cda723415` :commitid:`ad247eb92`
- **Improvement** distinguish an unanswered connection attempt from a refused one, and report an HTTP read or write that times out as a timeout :jiraissue:`ECFLOW-2128` :commitid:`54bb3c006`
- **Improvement** adopt SPDX licence identifiers and REUSE-compliant file headers across all sources :jiraissue:`ECFLOW-2117` :commitid:`bcb2b83f1` :commitid:`13d4aabca`

- **Fix** preserve label values containing hash, quote and blank characters across restore, sync and :code:`--get_state`, and warn when a value may have been truncated :jiraissue:`ECFLOW-1890` :commitid:`5a188fcaf`
- **Fix** parse date-times beyond the range of 32-bit seconds (after 2038) :jiraissue:`ECFLOW-1852` :commitid:`84113ee92`
- **Fix** correct the CMake configuration with tests enabled or disabled :commitid:`57dcc1173`

Server
------

- **Feature** configure the server from :code:`<host>.<port>.ecf.cfg`, a strict JSON file, which overrides the defaults and :code:`server_environment.cfg`, and is overridden by the environment and the command line :jiraissue:`ECFLOW-2130` :commitid:`45b1c3e00`
- **Feature** enable, in :code:`<host>.<port>.ecf.cfg`, the logging of the :code:`ECF_JOB_CMD`, :code:`ECF_KILL_CMD` and :code:`ECF_STATUS_CMD` commands spawned by the server, with credentials masked :jiraissue:`ECFLOW-2126` :commitid:`23dafe032`

- **Improvement** enhance the version information with additional details, including the Git commit, the Aviso version and the serialisation protocol :jiraissue:`ECFLOW-2127` :commitid:`14adcaa24`
- **Improvement** extend the server log, which now records:

  - the version description and the runtime configuration of the server, at start-up :jiraissue:`ECFLOW-2127` :commitid:`14adcaa24`
  - the source of each setting, and the authentication and authorisation in force, at start-up :jiraissue:`ECFLOW-2130` :commitid:`45b1c3e00`
  - the SSL certificate selected at start-up, with a warning on a fallback to :code:`server.crt` :jiraissue:`ECFLOW-2129` :commitid:`cda723415`
  - the command, user and client host of a refused request :jiraissue:`ECFLOW-2120` :commitid:`741697779`
  - a :code:`--news` request and its outcome as a single record :commitid:`a6da0adf7`

- **Improvement** close the descriptors inherited by a spawned job in constant time, using :code:`close_range` on Linux :jiraissue:`ECFLOW-2123` :commitid:`35e75d72e`
- **Improvement** refuse unusable start-up settings, such as a malformed :code:`server_environment.cfg` or an invalid :code:`ECF_CHECKMODE`, naming the setting, path and cause :commitid:`6407e6cba`
- **Improvement** keep the active credentials and permissions when a reload of the security files fails :commitid:`6407e6cba`

- **Fix** resolve a relative :code:`ECF_HOME` against the start-up directory :commitid:`6407e6cba`
- **Fix** recover from :code:`ECF_CHECKOLD` into a fresh definition after a failed :code:`ECF_CHECK`, and refuse to start, leaving both files unchanged, when neither loads :commitid:`6407e6cba`
- **Fix** refuse SSL in a server built without SSL support :jiraissue:`ECFLOW-2130` :commitid:`45b1c3e00`
- **Fix** stop the server when the HTTP server cannot listen on its port, naming the port and the cause :commitid:`01f435ede`

Python API
----------

- **Feature** add the :code:`evaluate` argument to :code:`Client.query` to resolve the :code:`%VAR%` references in the value of a variable :jiraissue:`ECFLOW-1975` :commitid:`b28ce4b0b`
- **Feature** expose the Aviso :code:`repeat` option :jiraissue:`ECFLOW-2125` :commitid:`ca281eeb8`

ecFlowUI
--------

- **Feature** show the Aviso :code:`--repeat` option, and search Aviso attributes by :code:`--collapse` and :code:`--repeat` :jiraissue:`ECFLOW-2125` :commitid:`ca281eeb8`

- **Improvement** prefer the server-specific certificate, falling back to :code:`server.crt`, independently of :code:`ECF_SSL` :jiraissue:`ECFLOW-2129` :commitid:`cda723415`

Documentation
-------------

- **Improvement** document the SSL certificate selection rules and the files of each certificate kind :jiraissue:`ECFLOW-2129` :commitid:`cda723415`
- **Improvement** document the client timeouts of every command :jiraissue:`ECFLOW-2128` :commitid:`54bb3c006`
- **Improvement** add a Cookbook recipe on the log of the commands spawned by the server and the masking of credentials :jiraissue:`ECFLOW-2126` :commitid:`23dafe032`
- **Improvement** document the server initialisation sequence, with the correct start-up order :commitid:`6407e6cba`
- **Improvement** update the list of supported platforms, distinguishing the platforms tested in CI from those with package builds :commitid:`56268ce2b`

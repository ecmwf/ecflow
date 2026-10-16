.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _server.cfg:

server.cfg
//////////

The file ``server.cfg`` holds the settings of the server that are not variables: unlike the
entries of :ref:`server_environment.cfg`, they are neither exported to the suites nor shown to
the clients. The file is optional; without it, every setting keeps its default.

Location
========

The server looks for the file **beside** ``server_environment.cfg``, in the directory it is
started from, and reads the first of:

1. ``<host>.<port>.server.cfg``, the file of one server;
2. ``server.cfg``, shared by every server started from that directory.

When both exist, only the per-server file is read.

Format
======

The file is a JSON object. Every key is a setting; a key that the server does not know, or a
value of the wrong type, is an error.

.. code-block:: json
    :caption: server.cfg

    {
        "spawn_as_owner": true
    }

.. list-table::
   :header-rows: 1

   * - Setting
     - Type
     - Default
     - Meaning
   * - ``spawn_as_owner``
     - boolean
     - ``false``
     - Run every job, kill and status command as the owner of the task, instead of as the
       account of the server. See :ref:`running_jobs_as_their_owner`.

Errors
======

The server does not start, and reports the file and the fault, when:

* the file is not valid JSON, or is not a JSON object;
* a setting has a value of the wrong type (for example ``"spawn_as_owner": "yes"``);
* a setting is unknown (a misspelt key counts as unknown, so that a typo never silently leaves
  a setting at its default);
* ``spawn_as_owner`` is ``true`` and the server does not run as root, since only root can switch
  to another account.

At start-up, the log names the file read and reports whether jobs are spawned as their owner or
as the server account; ``ecflow_server --debug`` shows both in its dump of the environment.

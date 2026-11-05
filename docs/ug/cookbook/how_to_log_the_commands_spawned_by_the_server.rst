.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: log of commands
   single: ECF_JOB_CMD (log of commands)

.. _how_to_log_the_commands_spawned_by_the_server:

How to log the commands spawned by the server?
**********************************************

The server spawns a command to submit each task (``ECF_JOB_CMD``), to kill it (``ECF_KILL_CMD``) and to query its
status (``ECF_STATUS_CMD``). The server can record, in its :ref:`log file <ecflow_log_file>`, each of these commands,
the variables used to render it, and how it terminated. The credentials that the server can recognise, by the name
of a variable or by the form of a value, are masked in these records.

This recipe enables the records on a server, runs a small suite, and shows the records written in each case. The
records are described in detail in :ref:`ecflow_log_file_commands`.

Enabling the records
====================

The records are enabled only by the :ref:`server_configuration_file`, ``<host>.<port>.ecf.cfg``, placed in the
directory in which the server is started. No environment variable or command-line option enables them.

For a server started on host ``server1`` with port ``3141``, create ``server1.3141.ecf.cfg``:

.. code-block:: json

    {
      "version": 1,
      "ecflow": {
        "server": {
          "variables": {"ECF_HOME": "/home/ops/ecflow"},
          "log": {"commands": {"enabled": true}}
        }
      }
    }

Then start the server from that directory:

.. code-block:: shell

    ecflow_server --port=3141

Notes:

- The file is read once, at start-up: a server already running must be restarted for a change to take effect.
- An invalid value, e.g. ``"enabled": "yes"``, stops the start-up, with a message naming the file and the entry.

Confirming the setting
======================

The start-up lines of the log state whether the file was loaded, and the setting in force with its origin:

.. code-block:: text

    MSG:[08:57:29 05.11.2026]     ecf.cfg (/home/ops/server1.3141.ecf.cfg) = loaded
    MSG:[08:57:29 05.11.2026]     ECF_HOME = /home/ops/ecflow @ ecf.cfg
    MSG:[08:57:29 05.11.2026]     Log of commands = enabled @ ecf.cfg

Without the file, or without the setting, the line reads ``Log of commands = disabled @ defaults``.

Example suite
=============

The suite below submits its tasks with ``troika``. Its variables are chosen to show how credentials are handled:

- ``TROIKA_TOKEN`` is a credential name, since one of its parts is ``token``: its value is masked wherever it is
  used;
- ``TROIKA_OPTS`` refers to ``TROIKA_TOKEN``, so the token reaches the commands through another variable;
- task ``t2`` writes a password literally in its command;
- task ``t4`` holds a secret in ``DB_PASS``, which is **not** a credential name.

.. code-block:: shell

    suite demo
      edit USER ops
      edit ECF_TRIES 1
      edit SCHOST hpc
      edit TROIKA_TOKEN tk-8f2c91
      edit TROIKA_OPTS "--token %TROIKA_TOKEN%"
      edit ECF_JOB_CMD "troika %TROIKA_OPTS% submit -u %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%"
      edit ECF_KILL_CMD "troika %TROIKA_OPTS% kill -u %USER% %SCHOST% %ECF_JOB%"
      edit ECF_STATUS_CMD "troika %TROIKA_OPTS% monitor -u %USER% %SCHOST% %ECF_JOB%"
      task t1
      task t2
        edit SCHOST hpc-down
        edit ECF_JOB_CMD "troika submit --password hunter2 -u %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%"
      task t3
        edit SCHOST hpc-hang
      task t4
        edit DB_PASS pa55w0rd
        edit ECF_JOB_CMD "troika submit --db-pass %DB_PASS% -u %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%"
    endsuite

In this example, ``troika`` succeeds for host ``hpc``, fails for host ``hpc-down``, and is killed by a signal for host
``hpc-hang``.

.. warning::

    ``troika`` does not support the options ``--token``, ``--password`` and ``--db-pass``. They are used here only
    as examples of options whose values are masked, or not, in the records; the commands of this suite are not
    valid ``troika`` commands.

    See the :ref:`rules of the masking <ecflow_log_file_masking>` for the options, names and URLs whose values are
    masked.

Reading the records
===================

The records are ``EXT`` lines of the log. Retrieve the log with :ref:`ecflow_client --log <log_cli>`, and select the
records, e.g. those of task ``/demo/t1``:

.. code-block:: shell

    ecflow_client --log=get | grep '^EXT:' | grep 'path(/demo/t1)'

The records of one command share its kind, its process id and the path of its task.

Successful submission
---------------------

Task ``t1`` is submitted, and ``troika`` exits with status 0:

.. code-block:: text

    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD spawn pid(9371) path(/demo/t1) cmd[ troika --token *** submit -u ops hpc /home/ops/ecflow/demo/t1.job1 /home/ops/ecflow/demo/t1.1 ]
    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD context pid(9371) path(/demo/t1) {"TROIKA_OPTS":"--token ***","TROIKA_TOKEN":"***","USER":"ops","SCHOST":"hpc","ECF_JOB":"/home/ops/ecflow/demo/t1.job1","ECF_JOBOUT":"/home/ops/ecflow/demo/t1.1"}
    EXT:[08:57:50 05.11.2026] ECF_JOB_CMD exit pid(9371) path(/demo/t1) status(0)

Notes:

- ``spawn`` shows the command with the token masked, although the command references it through ``TROIKA_OPTS``.
- ``context`` lists each variable used, once, in the order of first use. ``TROIKA_TOKEN`` is masked by its name;
  the value of ``TROIKA_OPTS``, ``--token %TROIKA_TOKEN%``, is logged as ``--token ***``, since the value of the
  option ``--token`` is masked by the fixed rules.
- ``exit`` is written when the server handles the termination, at the end of the next job scheduling: usually
  within ``ECF_INTERVAL`` seconds of the end of the command, but later when the server is busy or halted.

Killing a task and querying its status
--------------------------------------

The kill and the status commands of task ``t1``, requested with ``ecflow_client --kill=/demo/t1`` and
``ecflow_client --status=/demo/t1``, are recorded in the same way:

.. code-block:: text

    EXT:[08:57:52 05.11.2026] ECF_KILL_CMD spawn pid(9426) path(/demo/t1) cmd[ troika --token *** kill -u ops hpc /home/ops/ecflow/demo/t1.job1 ]
    EXT:[08:57:52 05.11.2026] ECF_KILL_CMD context pid(9426) path(/demo/t1) {"TROIKA_OPTS":"--token ***","TROIKA_TOKEN":"***","USER":"ops","SCHOST":"hpc","ECF_JOB":"/home/ops/ecflow/demo/t1.job1"}
    EXT:[08:57:52 05.11.2026] ECF_STATUS_CMD spawn pid(9428) path(/demo/t1) cmd[ troika --token *** monitor -u ops hpc /home/ops/ecflow/demo/t1.job1 ]
    EXT:[08:57:52 05.11.2026] ECF_STATUS_CMD context pid(9428) path(/demo/t1) {"TROIKA_OPTS":"--token ***","TROIKA_TOKEN":"***","USER":"ops","SCHOST":"hpc","ECF_JOB":"/home/ops/ecflow/demo/t1.job1"}
    EXT:[08:57:53 05.11.2026] ECF_KILL_CMD exit pid(9426) path(/demo/t1) status(0)
    EXT:[08:57:53 05.11.2026] ECF_STATUS_CMD exit pid(9428) path(/demo/t1) status(0)

The two commands run at the same time: their records are interleaved, and are told apart by their kind and process
id.

Failed submission
-----------------

The submission of task ``t2`` fails: ``troika`` exits with status 1, and the task is aborted:

.. code-block:: text

    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD spawn pid(9372) path(/demo/t2) cmd[ troika submit --password *** -u ops hpc-down /home/ops/ecflow/demo/t2.job1 /home/ops/ecflow/demo/t2.1 ]
    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD context pid(9372) path(/demo/t2) {"USER":"ops","SCHOST":"hpc-down","ECF_JOB":"/home/ops/ecflow/demo/t2.job1","ECF_JOBOUT":"/home/ops/ecflow/demo/t2.1"}
    EXT:[08:57:50 05.11.2026] ECF_JOB_CMD exit pid(9372) path(/demo/t2) status(1)
    ERR:[08:57:50 05.11.2026] ECF_JOB_CMD PID(9372) path(/demo/t2) exited with status 1 [ troika submit --password *** -u ops hpc-down /home/ops/ecflow/demo/t2.job1 /home/ops/ecflow/demo/t2.1 ]
    LOG:[08:57:50 05.11.2026]  aborted: /demo/t2 try-no: 1 reason: ECF_JOB_CMD PID(9372) path(/demo/t2) exited with status 1 [ troika submit --password *** -u ops hpc-down /home/ops/ecflow/demo/t2.job1 /home/ops/ecflow/demo/t2.1 ]

Notes:

- The password written literally after ``--password`` is masked by the fixed rules, as any value of a credential
  option.
- The ``ERR`` line and the abort reason of the task hold the same masked command. They are written, and masked,
  also when the records are disabled.

Command killed by a signal
--------------------------

The submission of task ``t3`` is killed by signal 9:

.. code-block:: text

    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD spawn pid(9373) path(/demo/t3) cmd[ troika --token *** submit -u ops hpc-hang /home/ops/ecflow/demo/t3.job1 /home/ops/ecflow/demo/t3.1 ]
    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD context pid(9373) path(/demo/t3) {"TROIKA_OPTS":"--token ***","TROIKA_TOKEN":"***","USER":"ops","SCHOST":"hpc-hang","ECF_JOB":"/home/ops/ecflow/demo/t3.job1","ECF_JOBOUT":"/home/ops/ecflow/demo/t3.1"}
    EXT:[08:57:50 05.11.2026] ECF_JOB_CMD signal pid(9373) path(/demo/t3) signal(9)
    ERR:[08:57:50 05.11.2026] ECF_JOB_CMD PID(9373) path(/demo/t3) died of signal 9 [ troika --token *** submit -u ops hpc-hang /home/ops/ecflow/demo/t3.job1 /home/ops/ecflow/demo/t3.1 ]

A secret under a name that is not a credential name
---------------------------------------------------

Task ``t4`` passes a password held in ``DB_PASS``. Since ``pass`` is not one of the parts that make a credential
name, the value is written as it is:

.. code-block:: text

    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD spawn pid(9374) path(/demo/t4) cmd[ troika submit --db-pass pa55w0rd -u ops hpc /home/ops/ecflow/demo/t4.job1 /home/ops/ecflow/demo/t4.1 ]
    EXT:[08:57:49 05.11.2026] ECF_JOB_CMD context pid(9374) path(/demo/t4) {"DB_PASS":"pa55w0rd","USER":"ops","SCHOST":"hpc","ECF_JOB":"/home/ops/ecflow/demo/t4.job1","ECF_JOBOUT":"/home/ops/ecflow/demo/t4.1"}
    EXT:[08:57:50 05.11.2026] ECF_JOB_CMD exit pid(9374) path(/demo/t4) status(0)

.. warning::

    Name every variable holding a secret with a credential name, i.e. with a part, split at ``_``, ``-``, ``.`` or
    ``:``, equal to ``password``, ``passwd``, ``token``, ``secret`` or ``key``. Renaming ``DB_PASS`` to
    ``DB_PASSWORD`` masks its value in every record.

Disabling the records
=====================

Set ``"enabled"`` to ``false``, or remove the ``log`` member, and restart the server. The start-up lines then read
``Log of commands = disabled``, and no ``EXT`` line is written for the commands.

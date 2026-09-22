.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: zombie (tutorial)

.. _tutorial-introducing-zombies:

Introducing zombies
========================

A :term:`zombie` is a running job that fails authentication when communicating with the :term:`ecflow_server`

How are zombies created ?
-------------------------

| There are wide variety of reasons why a :term:`zombie` is created.
| The most common causes are due to user action:

- The :term:`node` tree is deleted, replaced or reloaded whilst jobs are running
- A :term:`task` is rerun, whilst in a :term:`submitted` or :term:`active` state
- A job is forced to new state, i.e :term:`complete`

More rarer causes might be:

- :term:`ecf script` errors, where we have multiple calls to init and complete :term:`child command` s
- The :term:`child command` s in the :term:`ecf script` are placed in the background.
  In this case order in which the :term:`child command` contact the server, may be indeterminate.
- Load leveler submitting a job twice
- Server crash and recovered :term:`check point` file is out of date
- Machine crash

How can zombies be handled?
-----------------------------

The default behaviour for init, complete, abort and wait child commands, is to **block** the job, and for event, label, meter to continue(fob). (With **fob**, the job no longer blocks, but the server will not change the node tree)
When blocking the :term:`child command` command continues attempting to contact the :term:`ecflow_server`.

There are two environment variables that control how :term:`ecflow_client` handles wait times when trying to connect to the server.

- :term:`ECF_TIMEOUT` defines the maximum time the client waits for **any** :term:`child command`, hence including zombies. It is typically applicable when the server is down. It is specified in seconds, and the default value is 1 hour (3600 seconds). See :term:`ecflow_client`.
- :term:`ECF_ZOMBIE_TIMEOUT` is applied to zombies only. It is specified in seconds, and the default value is 30 minutes (1800 seconds). It applies to **each zombie** init, abort, and complete in the script. Since ECF_TIMEOUT also applies, a zombie gives up after the smaller of the two.

.. warning::
   *Changed in version 5.20.0*:
   The default values were reduced from 24 hours (ECF_TIMEOUT) and 12 hours (ECF_ZOMBIE_TIMEOUT).

When any of the above timeouts is exceeded, :term:`ecflow_client` exits with a failure. Depending on the script, this can be caught by a trap, which typically calls the abort child command; this again can wait for up to the timeout before the process exits.
Hence it is worth considering whether this is the appropriate behaviour for the system.

The jobs can also configured, so that if the server denies the communication, then the :term:`child command` can be set to fail immediately. (This can be done setting the environment variable ECF_DENIED in your scripts. See :term:`ecflow_client`). This can be useful to detect network issues early.

:term:`ecflow_ui` provides a tab that lists all the zombies and the actions that can be taken.

.. note::

  The zombies tab is shown, in the info panel when the server node( i.e. topmost) is selected.

The actions include:

- **Terminate**: The :term:`child command` is asked to **fail**.  Depending on your scripts,this may cause the abort :term:`child command` to be called. Which again will be flagged as a :term:`zombie`.

- **Fob**: Allow the job to continue. The :term:`child command` completes and hence no longer blocks the job. Great care should be taken when this action is chosen. If we have two jobs running, they may cause data corruption. Even when we have a single job, issues can arise. i.e if the associated command was an event :term:`child command`, then the :term:`event` would not be set. If this :term:`event` was used in a :term:`trigger` expression, it would never evaluate.

- **Delete**: Remove the :term:`zombie` from the server. The job will continue blocking, hence when the :term:`child command` next contacts the :term:`ecflow_server`, the :term:`zombie` will re-appear. If the job is killed manually, then this option can be used.

- **Rescue**: Adopt the zombie and update the node tree. The unique password (ECF_PASS) on the zombie is copied over to the :term:`task`, so that the next :term:`child command` will continue as normal. This should only be used when the user is sure there are no additional jobs.

- **Kill**: Applies the kill command (ECF_KILL_CMD ) using the process id stored on the :term:`zombie`. If the script has correct signal trapping, this should end up calling abort. Note: path zombies will need to be killed manually.


.. warning::

   Of the five action above, only Rescue will allow :term:`child command` to change the state of the node tree.

**What to do**

#. Create a :term:`zombie` by starting a :term:`task`, and setting it to :term:`complete` immediately via :term:`ecflow_ui`
#. Inspect the log file, it will show you how the zombie has arisen.
#. Inspect the zombie tag in :term:`ecflow_ui` (select the host node, then select the zombie's tab)
#. Experiment with the different actions on the zombie
#. Since the default ECF_ZOMBIE_TIMEOUT is 30 minutes, change this to 1 minute, by editing head.h.

   .. code-block:: shell

    export ECF_ZOMBIE_TIMEOUT=60 # specified in seconds

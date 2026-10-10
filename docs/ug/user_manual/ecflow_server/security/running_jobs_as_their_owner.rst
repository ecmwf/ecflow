.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _running_jobs_as_their_owner:

Running jobs as their owner
///////////////////////////

By default every job is spawned by the account that runs the server, whatever user wrote the
suite. On a server shared by several users this means that a job of one user runs with the
rights, the files and the credentials of the server account, which every other user shares.

A server can instead run each job as the user responsible for it: the *owner* of the task. The
operating system then decides what the job may touch, through the ordinary file permissions of
that account.

The owner of a task
===================

Every task records its owner: the login name of the user whose command put it in a state from
which a job is spawned (``--load``, ``--replace``, ``--requeue``, ``--run``, ``--force`` to
``queued``, or the submission of an edited script). The owner is exposed to the job as the
generated variable ``ECF_OWNER``, and is described in :ref:`ecf_owner`.

Enabling the switch
===================

Set ``spawn_as_owner`` to ``true`` in :ref:`server.cfg` and start the server **as root**. Without
the setting the server behaves as it always did; with the setting, a server that is not root
refuses to start.

What the server does
====================

Before it spawns a job, the server resolves the owner of the task to an account of the host, and
the process it forks switches to that account before executing ``ECF_JOB_CMD``:

* the command runs with the user id, the group id and the supplementary groups of the account;
* ``HOME``, ``USER``, ``LOGNAME`` and ``SHELL`` are set from the account, so that ``~`` and
  ``$HOME`` in the command or the script refer to the owner's home directory; the rest of the
  environment of the server is inherited;
* ``ECF_KILL_CMD`` and ``ECF_STATUS_CMD`` are spawned in the same way, as the user that requested
  the kill or the status, or as the owner when the server issues them itself.

A submission is **refused** when it cannot be attributed: the task has no owner recorded (for
example a task restored from a check point written by an older server, and not queued by any user
since), the owner is unknown to the host, or the owner is root. Nothing runs in that case: the
task is aborted with the reason, and the server log records the refusal, naming the task and the
user.

What the host needs
===================

Nothing that a multi-user machine does not already have:

* an account for every user that queues tasks, known to the host (local accounts, LDAP, ...),
  with a home directory the user controls (mode ``0700`` keeps it private);
* a work area shared by all users for the job files and the job output (``ECF_HOME``,
  ``ECF_OUT``): the server writes the job file, the job writes its output, so the layout must let
  the owner read the job file and write next to it (per-user sub-directories, or a sticky
  world-writable directory);
* the server started as root.

Because the job runs with the owner's rights only, no ``ECF_*_CMD`` needs to be fixed by the
administrator: a user may set ``ECF_JOB_CMD`` per suite or family, submit one family through a
batch system and run another locally, and can never reach the files or credentials of another
user. The configuration a submission needs (for example a ``troika.yml`` and the SSH key it
refers to) lives in the owner's home directory, under the owner's control.

.. warning::

    **Do not enable the switch on a server whose users are not authenticated.**

    The owner is the user name a client presents. Over HTTP the name comes from an
    authentication the server trusts; over a plain TCP connection it is the name the client
    *declares*, checked only by the :ref:`white list <ecflow_white_list_file>` and the
    :ref:`password file <black_list_file>` when they are configured. On a server that accepts an
    unchecked name, anyone able to reach the port can queue a task as any user, and the switch
    would then run the job as that user.

What the switch does not do
===========================

* It does not decide who may act on a task; that is the role of the access control of the server
  (white list, and the permissions of the nodes). When user ``b`` requeues a task of user ``a``,
  ``b`` becomes its owner and the job runs as ``b``.
* It does not control what a user does with the credentials in their own home directory. A user
  who lends a credential to another user does so outside ecFlow, and the submission is still
  recorded under the user that queued the task.
* It does not change how the job authenticates with the server: the child commands of a job
  (``ecflow_client --init``, ...) authenticate as they always did.

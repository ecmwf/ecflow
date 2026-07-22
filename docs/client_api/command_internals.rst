.. _ecflow_command_protocol:

Command internals
*****************

.. warning::

   This is an internal **developer reference**, intended for developers and integrators working on
   the ecFlow client/server protocol itself, not for end users driving ``ecflow_client``.

   It documents the **native client/server serialization** (the on-the-wire request and reply
   payloads) alongside the C++ command classes that implement each command. Unlike the
   :ref:`command line interface <ecflow_cli>` reference, this page is **not auto-generated** from
   ``ecflow_client``; it is maintained by hand and may lag behind the current implementation. Treat
   the payloads and class references as illustrative of a particular server version rather than a
   stable, guaranteed contract.

This page is a reference for the commands understood by the ecFlow server. It provides
a per-command reference, so that the description of each command,
its options, and its wire representation stay consistent with the implementation.

Introduction
============

ecFlow follows a client/server model. The server (``ecflow_server``) holds the suite definition and
the state of every node; the client (``ecflow_client``) issues **commands** that either query the
server or change its state. Every interaction is a single round trip:

- the client sends a **request** — a *client-to-server command*, an instance of a class derived from
  ``ClientToServerCmd`` (``.../libs/base/src/ecflow/base/cts/ClientToServerCmd.hpp``);
- the server processes it and returns a **reply** — a *server-to-client command*, an instance of a
  class derived from ``ServerToClientCmd`` (``.../libs/base/src/ecflow/base/stc/ServerToClientCmd.hpp``).

Command-line model
------------------

The ``ecflow_client`` executable exposes each command as a long option. A single invocation carries
exactly one command, optionally followed by that command's own options and arguments, for example::

  ecflow_client --init=$$
  ecflow_client --alter change variable FOO bar /suite/family

The registry that maps each option to its command is
``.../libs/base/src/ecflow/base/cts/CtsCmdRegistry.cpp``; the registration order there also fixes the
order in which commands appear in ``ecflow_client --help``. A few global options (``--help``,
``--version``, ``--debug``) apply to every command. A single command class frequently implements
several options: for example ``CtsCmd`` covers ``--ping``, ``--restart``, ``--halt``, ``--shutdown``
and ``--terminate``, and ``PathsCmd`` covers ``--suspend``, ``--resume``, ``--kill`` and others.

Context supplied through the command line versus the environment
----------------------------------------------------------------

Command context reaches the client in two ways:

- **Command-line options and arguments**, parsed with Boost.Program_options. User commands take all
  of their context this way (paths, attribute names, values).
- **Environment variables**, read by ``ClientEnvironment``
  (``.../libs/client/src/ecflow/client/ClientEnvironment.cpp``). Task commands rely almost entirely
  on these, because a job script does not know its own path or authentication token; the server
  writes them into the job at submission time.

Two groups of variables exist, matching the footer that ``ecflow_client --help`` prints for every
command. The **connection variables** are used by *every* command, whether a user command or a task
command:

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Variable
     - Meaning
   * - ``ECF_HOST``
     - The main server hostname; default value is ``localhost``.
   * - ``ECF_PORT``
     - The main server port; default value is ``3141``.
   * - ``ECF_SSL``
     - Enable secure (TLS) communication between the client and the server.
   * - ``ECF_HOSTFILE``
     - File that lists alternate hosts to try, if the connection to the main host fails.
   * - ``ECF_HOSTFILE_POLICY``
     - The policy (``task`` or ``all``) that defines which commands consider using alternate hosts.

``ECF_HOST`` and ``ECF_PORT`` (or the equivalent ``--host`` / ``--port`` options) must be set for the
client to reach the server; the others are optional.

The **task-command variables** are used additionally by *every task command*; the server writes them
into the job at submission time:

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Variable
     - Meaning
   * - ``ECF_NAME``
     - Full path name of the task, e.g. ``/suite/family/task``.
   * - ``ECF_PASS``
     - The job password, defined by the server and used to authenticate task commands.
   * - ``ECF_RID``
     - The remote (process) identifier; supports identifying zombies and killing running jobs.
   * - ``ECF_TRYNO``
     - The run number of the job, used in job and output file-name generation.
   * - ``ECF_TIMEOUT``
     - Maximum time, in seconds, for the client to deliver a message to the server; default is 24
       hours.
   * - ``ECF_DENIED``
     - Lets the task exit with an error on connection failure, instead of waiting for ``ECF_TIMEOUT``.
   * - ``NO_ECF``
     - If set, ``ecflow_client`` exits immediately with success; useful for testing scripts without a
       server.

A custom user name may also be supplied through ``ECF_USER`` (or ``--user``), in which case the
matching password is read from the custom password file (``ECF_CUSTOM_PASSWD``); this is what
populates the optional ``pswd_`` and ``cu_`` fields noted in *Reading the payloads*. The complete set
of constants is declared in ``.../libs/core/src/ecflow/core/Environment.hpp``. The per-command tables
in the reference below highlight the variables each command reads directly; the connection variables
above apply to every command throughout.

Task commands versus user commands
----------------------------------

Commands fall into two families, mirrored by the source layout under
``.../libs/base/src/ecflow/base/cts/``:

- **Task commands** (``cts/task/``) are issued from inside a ``.ecf`` job script, through the
  wrapper functions emitted by the job header. They report the progress of a running job and are
  authenticated with ``ECF_NAME`` + ``ECF_PASS`` + ``ECF_RID`` + ``ECF_TRYNO``, all derived from the
  environment. All of them share the base class ``TaskCmd`` (``cts/task/TaskCmd.hpp``). The task
  commands are ``InitCmd``, ``CompleteCmd``, ``AbortCmd``, ``EventCmd``, ``MeterCmd``, ``LabelCmd``,
  ``QueueCmd`` and ``CtsWaitCmd``.
- **User commands** (``cts/user/``) are issued by an operator or administrator to inspect or steer
  the server and its suites (begin, alter, force, delete, suspend, and so on). They share the base
  class ``UserCmd`` (``cts/user/UserCmd.hpp``) and are authenticated by user name.

.. note::
   The request and reply payloads shown in this note are the **native serialization** used on the
   ecFlow client/server connection: the command object encoded as a cereal JSON archive
   (``.../libs/core/src/ecflow/core/Serialization.hpp``). This is distinct from the JSON of the
   optional REST API (``ecflow_http``), which is a separate, hand-written HTTP layer documented in
   :ref:`the REST API reference <rest_api>`.

Reading the payloads
--------------------

On the wire a request is not the bare command but a small envelope. The client wraps the command in
a ``ClientToServerRequest`` (member ``cmd_``) and the server wraps its reply in a
``ServerToClientResponse`` (member ``stc_cmd_``). cereal serialises the outermost object under a key
derived from its mangled C++ type name (for example ``"21ClientToServerRequest"``), and it renders a
polymorphic pointer as a ``polymorphic_name`` (the concrete command class) plus a ``ptr_wrapper``
holding the object's ``data``. Inside ``data``, base-class members appear first as nested ``value0``
objects:

- for **task commands**, ``cl_host_`` comes from ``ClientToServerCmd`` and
  ``path_to_submittable_``, ``jobs_password_``, ``process_or_remote_id_`` and ``try_no_`` come from
  ``TaskCmd``;
- for **user commands**, ``cl_host_`` comes from ``ClientToServerCmd`` and ``user_`` comes from
  ``UserCmd`` (set when the request is dispatched). ``UserCmd`` also carries two *optional* fields,
  serialized only when set: ``pswd_``, the user's password, present when a password file supplies one
  (``ECF_PASSWD``, or ``ECF_CUSTOM_PASSWD`` for a custom user); and ``cu_``, a flag that is ``true``
  when a custom user name is in use (through ``--user``, ``ECF_USER`` or the client API). The
  user-command examples in this note come from a server without password authentication configured,
  so they show ``user_`` only; when a password is in play, a ``pswd_`` field (and, for a custom user,
  ``cu_``) appears alongside it.

The ``cereal_class_version`` and ``polymorphic_id`` fields are cereal bookkeeping. The payloads below
are real serializer output, with host, password and user values replaced by placeholders.

Reference
=========

For each command, the table summarises its type, class, action and inputs; the tabs that follow hold
the command-line **Usage**, the serialized **Request** payload, and the serialized **Reply** payload.
A command whose single class implements several actions adds a table of those actions before the
tabs. A few commands carry an unbounded payload (an embedded definition or node subtree); these are
described in prose instead of the tabbed layout.

.. raw:: html

   <style>.ecf-help label{font-weight:600}.ecf-help select{margin:.2em 0 .6em;padding:.15em .3em}.ecf-help-panel{white-space:pre;overflow-x:auto;background:#f6f8fa !important;color:#1f2328 !important;border:1px solid #d0d7de;border-radius:4px;padding:.6em .8em;margin:0;font-size:.85em}</style>
   <script>function ecfHelp(s){var b=s.closest(".ecf-help");b.querySelectorAll(".ecf-help-panel").forEach(function(p){p.hidden=true;});var t=document.getElementById(s.value);if(t)t.hidden=false;}</script>

Task commands
-------------

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Command
     - Description
   * - `AbortCmd`_
     - Mark the task as aborted, with an optional reason.
   * - `CompleteCmd`_
     - Mark the task as complete, optionally removing variables.
   * - `CtsWaitCmd`_
     - Block the job until a trigger expression becomes true.
   * - `EventCmd`_
     - Set or clear an event on the task.
   * - `InitCmd`_
     - Mark the task as started (active), optionally adding variables.
   * - `LabelCmd`_
     - Update the value of a label on the task.
   * - `MeterCmd`_
     - Update the value of a meter on the task.
   * - `QueueCmd`_
     - Request or update a step of a queue.

.. _proto_AbortCmd:

AbortCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/AbortCmd.hpp``
   * - Action
     - ``--abort``
   * - Parameters
     - - ``arg1`` (optional): a free-text reason explaining why the abort was raised.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Marks the task as ``aborted``. It is for use inside a ``.ecf`` script only, and is normally raised by
the trap installed in the job header. When no reason is supplied the server records
``Trap raised in job file``. The reason is sanitised on construction (newlines are stripped and
semicolons replaced by spaces) so that it cannot corrupt the output of ``--migrate`` or ``--load``.
The zombie handling is the same as for ``InitCmd``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --abort=reasonX
            # a bare --abort is valid; the server substitutes a default reason
            ecflow_client --abort

    .. tab:: Help

        Verbatim ``ecflow_client --abort`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            abort
            -----

            Mark task as aborted. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables
              arg1 = (optional) string(reason)
                     Optionally provide a reason why the abort was raised

            If this child command is a zombie, then the default action will be to *block*.
            The default can be overridden by using zombie attributes.
            Otherwise the blocking period is defined by ECF_TIMEOUT.

            Usage:
              ecflow_client --abort=reasonX

    .. tab:: Request

        ``AbortCmd`` produced by ``--abort="disk full"`` (the reason appears as ``reason_``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AbortCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "reason_": "disk full"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_CompleteCmd:

CompleteCmd
~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/CompleteCmd.hpp``
   * - Action
     - ``--complete``
   * - Parameters
     - - No positional argument.
       - ``--remove`` (optional): a list of variable names to delete from the task.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Marks the task as ``complete``. It is for use inside a ``.ecf`` script only. Where ``--init`` can add
variables to a task, ``--complete`` can remove them, which is convenient for clearing per-run
variables as the job finishes. The zombie handling is the same as for ``InitCmd``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --complete
            # delete variables name1 and name2 from the task as it completes
            ecflow_client --complete --remove name1 name2

    .. tab:: Help

        Verbatim ``ecflow_client --complete`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            complete
            --------

            Mark task as complete. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables

            If this child command is a zombie, then the default action will be to *block*.
            The default can be overridden by using zombie attributes.
            Otherwise the blocking period is defined by ECF_TIMEOUT.
            The init command allows variables to be added, and complete command
            allows for them to be removed.
              arg1(--remove)(optional) = a list of variables to removed from this task

            Usage:
              ecflow_client --complete
              ecflow_client --complete --remove name1 name2 # delete variables name1 and name2 on the task

    .. tab:: Request

        ``CompleteCmd`` with no removed variables:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CompleteCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      }
                    }
                  }
                }
              }
            }

        ``CompleteCmd`` produced by ``--complete --remove A B`` (the names appear as ``var_to_del_``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CompleteCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "var_to_del_": [ "A", "B" ]
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_CtsWaitCmd:

CtsWaitCmd
~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/CtsWaitCmd.hpp``
   * - Action
     - ``--wait``
   * - Parameters
     - - ``arg1`` (mandatory): a trigger expression to evaluate.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Evaluates a trigger expression and blocks while it is false, returning to the job only once it
becomes true. It is for use inside a ``.ecf`` script only, and is the mechanism behind script-level
synchronisation on the state of other nodes.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --wait="/suite/taskx == complete"

    .. tab:: Help

        Verbatim ``ecflow_client --wait`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            wait
            ----

            Evaluates an expression, and block while the expression is false.
            For use in the '.ecf' file *only*, hence the context is supplied via environment variables
              arg1 = string(expression)

            Usage:
              ecflow_client --wait="/suite/taskx == complete"

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsWaitCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "expression_": "/suite/taskx == complete"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        When the expression evaluates to true the server returns the generic ``StcCmd`` reply with
        ``api_`` equal to ``0``:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

        While the expression is false the server returns a *blocking* reply (``StcCmd`` with ``api_``
        equal to ``2``, ``BLOCK_CLIENT_ON_HOME_SERVER``), on which the client waits and retries:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 2
                    }
                  }
                }
              }
            }

.. _proto_EventCmd:

EventCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/EventCmd.hpp``
   * - Action
     - ``--event``
   * - Parameters
     - - ``arg1`` (mandatory): the event name (or number).
       - ``arg2`` (optional): ``set`` or ``clear``; defaults to ``set``.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Sets or clears an event on the task. It is for use inside a ``.ecf`` script only. Unlike ``--init``,
``--complete`` and ``--abort``, the default zombie action is to *fob* (allow the client command to
complete without error); this can be overridden with zombie attributes.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --event=ev         # set the event (the default)
            ecflow_client --event=ev set     # set the event, explicit
            ecflow_client --event=ev clear   # clear the event

    .. tab:: Help

        Verbatim ``ecflow_client --event`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            event
            -----

            Change event. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables
              arg1(string | int)     = event-name

              arg2(string)(optional) = [ set | clear] default value is set

            If this child command is a zombie, then the default action will be to *fob*,
            i.e allow the ecflow client command to complete without an error
            The default can be overridden by using zombie attributes.

            Usage:
              ecflow_client --event=ev       # set the event, default since event initial value is clear
              ecflow_client --event=ev set   # set the event, explicit
              ecflow_client --event=ev clear # clear the event, use when event initial value is set

    .. tab:: Request

        ``--event=ev`` sets the event; ``value_`` is stored only when it differs from the default,
        so the *set* request omits it:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "EventCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "ev"
                    }
                  }
                }
              }
            }

        ``--event=ev clear`` adds ``value_`` set to ``false``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "EventCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "ev",
                      "value_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_InitCmd:

InitCmd
~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/InitCmd.hpp``
   * - Action
     - ``--init``
   * - Parameters
     - - ``arg1`` (mandatory): the process or remote identifier of the job (typically the shell
         ``$$``), which the server stores so the job can later be killed.
       - ``--add`` (optional): a list of ``name=value`` pairs to add to, or update on, the task.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_TRYNO`` — current try number.
       - ``ECF_RID`` — when set, cross-checked against ``arg1``; the two must match.

Marks the task as started (state ``active``). It is for use inside a ``.ecf`` script only. When the
task command is detected as a zombie the default action is to *block*; this can be overridden with
zombie attributes, and the blocking period is bounded by ``ECF_TIMEOUT``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --init=$$
            # add or update variables on the task at the same time
            ecflow_client --init=$$ --add name=value name2=value2

    .. tab:: Help

        Verbatim ``ecflow_client --init`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            init
            ----

            Mark task as started(active). For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables.
              arg1(string)         = process_or_remote_id The process id of the job or remote_id
                                     Using remote id allows the jobs to be killed
              arg2(--add)(optional)= add/update variables as name value pairs

            If this child command is a zombie, then the default action will be to *block*.
            The default can be overridden by using zombie attributes.
            Otherwise the blocking period is defined by ECF_TIMEOUT.

            Usage:
              ecflow_client --init=$$
              ecflow_client --init=$$ --add name=value name2=value2 # add/update variables to task

    .. tab:: Request

        ``InitCmd`` with no added variables:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "InitCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      }
                    }
                  }
                }
              }
            }

        ``InitCmd`` produced by ``--init=$$ --add A=1 B=2`` (the added pairs appear as
        ``var_to_add_``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "InitCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "var_to_add_": [
                        { "n_": "A", "v_": "1" },
                        { "n_": "B", "v_": "2" }
                      ]
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``OK``, i.e. ``0``), returned
        by every task command on success:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_LabelCmd:

LabelCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/LabelCmd.hpp``
   * - Action
     - ``--label``
   * - Parameters
     - - ``arg1`` (mandatory): the label name.
       - ``arg2`` (mandatory): the new label value; may be a single word or a multi-line,
         space-separated set of quoted strings.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Updates the value of a label on the task. It is for use inside a ``.ecf`` script only. The default
zombie action is to *fob*.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --label=progressed merlin

    .. tab:: Help

        Verbatim ``ecflow_client --label`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            label
            -----

            Change Label. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables
              arg1 = label-name
              arg2 = The new label value
                     The labels values can be single or multi-line(space separated quoted strings)

            If this child command is a zombie, then the default action will be to *fob*,
            i.e allow the ecflow client command to complete without an error
            The default can be overridden by using zombie attributes.

            Usage:
              ecflow_client --label=progressed merlin

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "LabelCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "progressed",
                      "label_": "merlin"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_MeterCmd:

MeterCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/MeterCmd.hpp``
   * - Action
     - ``--meter``
   * - Parameters
     - - ``arg1`` (mandatory): the meter name.
       - ``arg2`` (mandatory): the new meter value (integer).
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Updates the value of a meter on the task. It is for use inside a ``.ecf`` script only. The default
zombie action is to *fob*.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --meter=my_meter 20

    .. tab:: Help

        Verbatim ``ecflow_client --meter`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            meter
            -----

            Change meter. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables
              arg1(string) = meter-name
              arg2(int)    = the new meter value

            If this child command is a zombie, then the default action will be to *fob*,
            i.e allow the ecflow client command to complete without an error
            The default can be overridden by using zombie attributes.

            Usage:
              ecflow_client --meter=my_meter 20

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "MeterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "my_meter",
                      "value_": 20
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_QueueCmd:

QueueCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - Task command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/task/QueueCmd.hpp``
   * - Action
     - ``--queue``
   * - Parameters
     - - ``arg1`` (mandatory): the queue name.
       - ``arg2`` (mandatory): the action, one of ``active``, ``aborted``, ``complete``,
         ``no_of_aborted`` or ``reset``.
       - ``arg3`` (step): required by ``complete`` and ``aborted``; the step value returned by a
         previous ``active`` call.
       - ``arg4`` (path, optional): the node holding the queue; by default the queue is searched for
         up the node tree.
   * - Environment variables
     - - ``ECF_NAME`` — absolute path of the task.
       - ``ECF_PASS`` — job password used for authentication.
       - ``ECF_RID`` — remote (process) identifier of the job.
       - ``ECF_TRYNO`` — current try number.

Cooperates with a ``queue`` attribute to hand out steps to a running job. ``active`` returns the
first queued or aborted step and advances the index; ``no_of_aborted`` returns the count of aborted
steps; ``reset`` rewinds the index so steps can be reprocessed; ``complete`` and ``aborted`` mark the
given step. It is for use inside a ``.ecf`` script only, and the default zombie action is to *block*.
The ``active`` and ``no_of_aborted`` actions return a string; the others return the generic reply.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            QNAME=my_queue
            # obtain and activate the next step; returns e.g. "003", or "<NULL>" when exhausted
            step=$(ecflow_client --queue=$QNAME active)
            # after processing, mark the step complete (or aborted on failure)
            ecflow_client --queue=$QNAME complete $step

    .. tab:: Help

        Verbatim ``ecflow_client --queue`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            queue
            -----

            QueueCmd. For use in the '.ecf' script file *only*
            Hence the context is supplied via environment variables
              arg1(string) = queue-name:
              arg2(string) = action: [active | aborted | complete | no_of_aborted | reset ]
                 active: returns the first queued/aborted step, the return string is the queue value from the definition
                 no_of_aborted: returns number of aborted steps as a string, i.e 10
                 reset: sets the index to the first queued/aborted step. Allows steps to be reprocessed for errors
              arg3(string) = step: value returned from step=$(ecflow_client --queue=queue_name active)
                            This is only valid for complete and aborted steps
              arg4(string) = path: (optional). The path where the queue is defined.
                             By default we search for the queue up the node tree.

            If this child command is a zombie, then the default action will be to *block*,
            The default can be overridden by using zombie attributes.If the path to the queue is not defined, then this command will
            search for the queue up the node hierarchy. If no queue found, command fails

            Usage:
            step=""
            QNAME="my_queue_name"
            while [1 == 1 ] ; do
               # this return the first queued/aborted step, then increments to next step, return <NULL> when all steps processed
               step=$(ecflow_client --queue=$QNAME active) # of the form string  i.e "003". this step is now active
               if [[ $step == "<NULL>" ]] ; then
                    break;
               fi
               ...
               ecflow_client --queue=$QNAME complete $step   # tell ecflow this step completed
            done

            trap() { ecflow_client --queue=$QNAME aborted $step # tell ecflow this step failed }

    .. tab:: Request

        ``--queue=my_queue active`` (``step_`` is empty for this action):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "QueueCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "my_queue",
                      "action_": "active",
                      "step_": "",
                      "path_to_node_with_queue_": ""
                    }
                  }
                }
              }
            }

        ``--queue=my_queue complete 003`` (``step_`` carries the step to mark):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "QueueCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "path_to_submittable_": "/suite/family/task",
                        "jobs_password_": "jZ2Vld",
                        "process_or_remote_id_": "12345",
                        "try_no_": 1
                      },
                      "name_": "my_queue",
                      "action_": "complete",
                      "step_": "003",
                      "path_to_node_with_queue_": ""
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``complete``, ``aborted`` and ``reset`` return the generic ``StcCmd`` reply with ``api_``
        equal to ``0``:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

        ``active`` and ``no_of_aborted`` return an ``SStringCmd`` whose ``str_`` field holds the
        result (for ``active``, the step value; for ``no_of_aborted``, the count):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "003"
                    }
                  }
                }
              }
            }

User commands
-------------

Several user commands implement a whole family of operations in a single class, dispatched by an
enumerated discriminator carried in the payload — for example ``api_`` for ``CtsCmd``, ``CtsNodeCmd``,
``PathsCmd``, ``LogCmd``, ``ClientHandleCmd`` and ``CSyncCmd``, the ``add_attr_type_`` /
``change_attr_type_`` / ``del_attr_type_`` fields for ``AlterCmd``, and ``user_action_`` for
``ZombieCmd``. For these, the individual actions differ only in that field and in the accompanying
paths, names and values, so each is documented with a table of its actions followed by one or two
representative payloads rather than a payload per action. ``AlterCmd`` and ``CtsCmd`` are the two
largest. The commands below are listed alphabetically.

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Command
     - Description
   * - `AlterCmd`_
     - Add, change, delete or sort an attribute on nodes.
   * - `BeginCmd`_
     - Begin one or all suites.
   * - `CFileCmd`_
     - Retrieve a node's script, job, output, manual or kill/status file.
   * - `CheckPtCmd`_
     - Checkpoint the server, optionally changing the checkpoint mode.
   * - `ClientHandleCmd`_
     - Manage client handles (per-client suite subsets) used for synchronisation.
   * - `CSyncCmd`_
     - Synchronise a client with the server's state.
   * - `CtsCmd`_
     - Argument-less server signals (ping, restart, halt, stats, and so on).
   * - `CtsNodeCmd`_
     - Query a node or the definition (get, why, get_state, migrate, and so on).
   * - `DeleteCmd`_
     - Delete nodes, or every suite.
   * - `EditScriptCmd`_
     - Retrieve or submit a task's script, with optional preprocessing.
   * - `ForceCmd`_
     - Force a node into a state, or force an event.
   * - `FreeDepCmd`_
     - Free a held dependency so a node may run.
   * - `GroupCTSCmd`_
     - Bundle several commands into a single request.
   * - `LoadDefsCmd`_
     - Load a suite definition into the server.
   * - `LogCmd`_
     - Inspect or manage the server log file.
   * - `LogMessageCmd`_
     - Write a message into the server log.
   * - `MoveCmd`_
     - Transfer a node subtree between locations or servers (internal).
   * - `OrderNodeCmd`_
     - Change a node's position among its siblings.
   * - `PathsCmd`_
     - Node operations (suspend, resume, kill, status, check, archive, and so on).
   * - `PlugCmd`_
     - Move a node subtree to a new parent, possibly on another server.
   * - `QueryCmd`_
     - Read a single piece of node state without changing anything.
   * - `ReplaceNodeCmd`_
     - Replace or add a node using another definition.
   * - `RequeueNodeCmd`_
     - Requeue nodes so they may run again.
   * - `RunNodeCmd`_
     - Run a task immediately, bypassing its time dependencies.
   * - `ServerVersionCmd`_
     - Return the server version string.
   * - `ShowCmd`_
     - Select the client print style (client-side).
   * - `ZombieCmd`_
     - Resolve zombies (fob, fail, adopt, remove, block, kill).

.. _proto_AlterCmd:

AlterCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/AlterCmd.hpp``
   * - Action
     - ``--alter``
   * - Parameters
     - - ``arg1`` (operation): one of ``add``, ``change``, ``delete``, ``sort``, ``set_flag`` or
         ``clear_flag``.
       - ``arg2`` (attribute type): the kind of attribute to operate on (see the *Attribute types*
         table below).
       - ``name`` / ``value`` (operation-dependent): the attribute name and value; several
         attribute types take only a name, and ``delete`` with no name removes every attribute of
         that type.
       - ``paths`` (one or more): the nodes to which the alteration applies.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Adds, changes, deletes or sorts an attribute on one or more nodes. It is the primary command for
editing a running suite. The general form is
``--alter <operation> <attribute-type> [name] [value] <path> [<path> ...]``. The attribute types
accepted by each operation are:

.. list-table::
   :header-rows: 1
   :widths: 18 82

   * - Operation
     - Attribute types
   * - ``add``
     - ``time``, ``today``, ``date``, ``day``, ``zombie``, ``variable``, ``late``, ``limit``,
       ``inlimit``, ``label``, ``aviso``, ``mirror``, ``event``, ``meter``
   * - ``change``
     - ``variable``, ``clock_type``, ``clock_date``, ``clock_gain``, ``clock_sync``, ``event``,
       ``meter``, ``label``, ``trigger``, ``complete``, ``repeat``, ``limit_max``, ``limit_value``,
       ``defstatus``, ``late``, ``time``, ``today``, ``aviso``, ``mirror``
   * - ``delete``
     - ``variable``, ``time``, ``today``, ``date``, ``day``, ``cron``, ``event``, ``meter``,
       ``label``, ``trigger``, ``complete``, ``repeat``, ``limit``, ``limit_path``, ``inlimit``,
       ``zombie``, ``late``, ``queue``, ``generic``, ``aviso``, ``mirror``
   * - ``sort``
     - ``event``, ``meter``, ``label``, ``limit``, ``variable``, ``all``
   * - ``set_flag`` / ``clear_flag``
     - a node flag, carried by ``flag_type_`` and ``flag_`` rather than by an attribute type

Every ``AlterCmd`` payload carries the same fields; the operation is encoded by the discriminator
integers, and any discriminator not in use holds its "not set" sentinel value (``add_attr_type_``
``6``, ``del_attr_type_`` ``16``, ``change_attr_type_`` ``13``, ``flag_type_`` ``19``). For ``add``,
``change`` and ``delete`` exactly one of ``add_attr_type_`` / ``change_attr_type_`` /
``del_attr_type_`` selects the attribute type. For ``set_flag`` and ``clear_flag`` the flag is
carried by ``flag_type_``, and ``flag_`` selects set (``true``) versus clear (``false``). For
``sort`` all attribute discriminators stay at their sentinel and ``name_`` holds the attribute type
(with ``value_`` optionally ``recursive``).

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --alter change variable FOO bar /suite/family
            ecflow_client --alter add label info "hello world" /suite/family/task
            # delete every variable on the node (no name given)
            ecflow_client --alter delete variable /suite
            # sort the variables of the node and its children
            ecflow_client --alter sort variable recursive /suite/family
            # set and clear a node flag
            ecflow_client --alter set_flag late /suite/family
            ecflow_client --alter clear_flag late /suite/family

    .. tab:: Help

        Verbatim ``ecflow_client --alter`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            alter
            -----

            Alter the node according to the options.

            arg1 = [ delete | change | add | set_flag | clear_flag | sort ]

            arg2 = For delete:
                   [ variable | time | today | date  | day | cron | event | meter | late | generic |
                     queue | label | trigger | complete | repeat | limit | inlimit | limit_path |
                     zombie | aviso | mirror ]

                   For change:
                   [ variable | clock_type | clock_gain | clock_date | clock_sync  | event | meter |
                     label | trigger  | complete | repeat | limit_max | limit_value | defstatus |
                     late | time | today | aviso | mirror ]

                   For add:
                   [ variable | time | today | date | day | zombie | event | meter | late | limit |
                     inlimit | label | aviso | mirror ]

                   For set_flag or clear_flag:
                   [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed |
                     statuscmd_failed | killcmd_failed | no_script | killed | status | late |
                     message complete | queue_limit | task_waiting | locked | zombie | archived |
                     restored | threshold | log_error | checkpt_error ]

                   For sort:
                   [ event | meter | label | variable| limit | all ]

            arg3 = [ <name> | <value> ]

            arg4 = <new-value>

            arg5 = <path> (<path> (...)) - at least one node path required.

            *Important Notes*

             * All paths must start with a leading '/' character.

             * To update, create or remove server variables use '/' for path.

             * When updating unnamed attributes (Repeat, Trigger, Complete, ...) the name/arg3 is not necessary.

             * After changing the clock the suite needs to be re-queued for the change to take effect.

             * When adding or updating node attributes (e.g. variable, meter, event, label, limits, late)
               the name (arg3) and value (arg4) must be quoted.

             * When sorting attributes, 'recursive' can be used as the value (arg3).

             * When adding a meter, the value (arg4) is expected to be a comma-separated triplet
               of numerical values the form "<min>,<max>,<value>" (n.b. no spaces are allowed).

             * When adding an event, the non-optional value (arg4) must be either "set" or "clear".

             * When adding or updating aviso and mirror attributes, the value (arg4) is expected to be
               a quoted list of configuration options. For example:
               * for aviso, "--remote_path /s1/f1/t2 --remote_host host --polling 20 --remote_port 3141 --ssl)"
               * for mirror, "--listener '{ \"event\": \"mars\", \"request\": { \"class\": "od" } }'
                              --url http://aviso/ --schema /path/to/schema --polling 60"

             * For both aviso and mirror, the special value "reload" forces reloading the configuration.
               This is typically useful after updating variables used to configure these kind of attributes.

            Usage:

               ecflow_client --alter=add variable <variable-name> "value" /             # add server variable
               ecflow_client --alter=add variable <variable-name> "value" /path/to/node # add node variable

               ecflow_client --alter=add time "+00:20" /path/to/node

               ecflow_client --alter=add date "01.*.*" /path/to/node

               ecflow_client --alter=add day "sunday"  /path/to/node

               ecflow_client --alter=add label <label-name> "label_value" /path/to/node

               ecflow_client --alter=add event <event-name> "set"|"clear" /path/to/node

               ecflow_client --alter=add meter <meter-name> "<min>,<max>,value" /path/to/node

               ecflow_client --alter=add late "-s 00:01 -a 14:30 -c +00:01" /path/to/node

               ecflow_client --alter=add limit mars "100" /path/to/node

               ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name "10" /path/to/node

               ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name "-s 10" /path/to/node

               ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name "-n 10" /path/to/node

               # zombie attributes have the following structure:
                 `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`
                  zombie_type        = "user" | "ecf" | "path" | "ecf_pid" | "ecf_passwd" | "ecf_pid_passwd"
                  client_side_action = "fob" | "fail" | "block"
                  server_side_action = "adopt" | "delete" | "kill"
                  child              = "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete" | "queue"
                  zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60

               ecflow_client --alter=add zombie "ecf:fail::" /path/to/node     # ask system zombies to fail
               ecflow_client --alter=add zombie "user:fail::" /path/to/node    # ask user generated zombies to fail
               ecflow_client --alter=add zombie "path:fail::" /path/to/node    # ask path zombies to fail

               ecflow_client --alter=delete variable FRED /path/to/node  # delete variable FRED
               ecflow_client --alter=delete variable      /path/to/node  # delete *ALL* variables on the given snode

    .. tab:: Request

        ``--alter change variable FOO bar /suite/family`` (``change_attr_type_`` ``0`` selects
        ``variable``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AlterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "name_": "FOO",
                      "value_": "bar",
                      "add_attr_type_": 6,
                      "del_attr_type_": 16,
                      "change_attr_type_": 0,
                      "flag_type_": 19,
                      "flag_": false
                    }
                  }
                }
              }
            }

        ``--alter add label info "hello world" /suite/family/task`` (``add_attr_type_`` ``10``
        selects ``label``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AlterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family/task" ],
                      "name_": "info",
                      "value_": "hello world",
                      "add_attr_type_": 10,
                      "del_attr_type_": 16,
                      "change_attr_type_": 13,
                      "flag_type_": 19,
                      "flag_": false
                    }
                  }
                }
              }
            }

        ``--alter delete variable /suite`` (``del_attr_type_`` ``0`` selects ``variable``; an empty
        ``name_`` deletes every variable):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AlterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite" ],
                      "name_": "",
                      "value_": "",
                      "add_attr_type_": 6,
                      "del_attr_type_": 0,
                      "change_attr_type_": 13,
                      "flag_type_": 19,
                      "flag_": false
                    }
                  }
                }
              }
            }

        ``--alter sort variable recursive /suite/family`` (all attribute discriminators stay at their
        sentinel; ``name_`` holds the attribute type and ``value_`` is ``recursive``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AlterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "name_": "variable",
                      "value_": "recursive",
                      "add_attr_type_": 6,
                      "del_attr_type_": 16,
                      "change_attr_type_": 13,
                      "flag_type_": 19,
                      "flag_": false
                    }
                  }
                }
              }
            }

        ``--alter set_flag late /suite/family`` (``flag_type_`` ``7`` selects the ``late`` flag and
        ``flag_`` is ``true``). The corresponding ``clear_flag`` request is identical except that
        ``flag_`` is ``false``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "AlterCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "name_": "",
                      "value_": "",
                      "add_attr_type_": 6,
                      "del_attr_type_": 16,
                      "change_attr_type_": 13,
                      "flag_type_": 7,
                      "flag_": true
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        On success ``AlterCmd`` returns the generic ``StcCmd`` reply with ``api_`` equal to ``0``:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_BeginCmd:

BeginCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/BeginCmd.hpp``
   * - Action
     - ``--begin``
   * - Parameters
     - - ``arg1`` (optional): the suite to begin; when omitted every suite is begun.
       - ``force`` (optional): begin even when the suite is already begun, discarding its state.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Begins one or all suites, moving them from the definition into a running state so that scheduling
starts. The ``force`` flag is stored in ``force_``; the request below is for ``--begin s1`` and the
``--begin s1 --force`` variant differs only in ``force_`` being ``true``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --begin s1
            ecflow_client --begin s1 --force
            ecflow_client --begin            # begin all suites

    .. tab:: Help

        Verbatim ``ecflow_client --begin`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            begin
            -----

            Begin playing the definition in the server.
            Expects zero or a single quoted string.
              arg1 = suite-name | Nothing | force
                     play the chosen suite, if no arg specified, play all suites, in the definition
                     force means reset the begin status on the suites and bypass checks.
                     This is only required if suite-name is provide as the first argument
                     Using force can cause the creation of zombies
            Usage:
            --begin                     # will begin all suites
            --begin="--force"         # reset and then begin all suites, bypassing any checks. Note: string must be quoted
            --begin="mySuite"         # begin playing suite of name 'mySuite'
            --begin="mySuite --force" # reset and begin playing suite 'mySuite', bypass check

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "BeginCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "suiteName_": "s1",
                      "force_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_CFileCmd:

CFileCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/CFileCmd.hpp``
   * - Action
     - ``--file``
   * - Parameters
     - - ``arg1`` (node path): the node whose file is requested.
       - ``arg2`` (file type): one of ``script``/``ecf`` (``file_`` ``0``), ``job`` (``1``),
         ``jobout`` (``2``), ``manual`` (``3``), ``kill`` (``4``) or ``stat`` (``5``).
       - ``arg3`` (max lines, optional): the maximum number of lines to return.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Retrieves one of the files associated with a node (the script, the generated job, its output, the
manual page, or the output of the kill/status commands). The file type is carried by ``file_`` and
the line limit by ``max_lines_``. The reply is always a string.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --file /suite/family/task jobout 100
            ecflow_client --file /suite/family/task script

    .. tab:: Help

        Verbatim ``ecflow_client --file`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            file
            ----

            Return the chosen file. Select from [ script<default> | job | jobout | manual | kill | stat ]
            By default will return the script.
              arg1 = path to node
              arg2 = (optional) [ script<default> | job | jobout | manual | kill | stat ]
                     kill will attempt to return output of ECF_KILL_CMD, i.e the file %ECF_JOB%.kill
                     stat will attempt to return output of ECF_STATUS_CMD, i.e the file %ECF_JOB%.stat
              arg3 = (optional) max_lines = 10000 <default>

    .. tab:: Request

        ``--file /suite/family/task jobout 100`` (``file_`` ``2`` selects ``jobout``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CFileCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "file_": 2,
                      "pathToNode_": "/suite/family/task",
                      "max_lines_": 100
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        An ``SStringCmd`` whose ``str_`` field holds the file contents:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "..."
                    }
                  }
                }
              }
            }

.. _proto_CheckPtCmd:

CheckPtCmd
~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/CheckPtCmd.hpp``
   * - Action
     - ``--check_pt``
   * - Parameters
     - - ``mode`` (optional): ``never`` (``mode_`` ``0``), ``on_time`` (``1``) or ``always``
         (``2``); ``3`` is the undefined sentinel.
       - ``interval`` (optional): the checkpoint interval in seconds (``check_pt_interval_``).
       - ``alarm`` (optional): the save-time alarm in seconds (``check_pt_save_time_alarm_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Triggers a checkpoint of the server state, and optionally changes the checkpoint mode, interval and
save-time alarm. Called with no argument it checkpoints immediately.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --check_pt
            ecflow_client --check_pt=always:120:30

    .. tab:: Help

        Verbatim ``ecflow_client --check_pt`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            check_pt
            --------

            Forces the definition file in the server to be written to disk *or* allow mode,
            interval and alarm to be changed.
            Whenever the check pt file is written to disk, it is measured.
            If the time to save to disk is greater than the default of 30 seconds,
            then an alarm is raised. This can be seen in the GUI as a late flag on the server.
            Once the late flag has been set it will need to manually cleared in the GUI
            or by using --alter functionality
            Note excessive save times can interfere with job scheduling.
            The alarm threshold can be changed. See below.
               arg1 = (optional) mode [ never | on_time | on_time:<integer> | always | <integer>]
                 never     : Never check point the definition in the server
                 on_time   : Turn on automatic check pointing at interval stored on server
                 on_time<integer> : Turn on automatic check point, with the specified interval in seconds
                 alarm<integer>   : Modify the alarm notification time for check pt saving to disk
                 always    : Check point at any change in node tree, *NOT* recommended for large definitions
                 <integer> : This specifies the interval in seconds when server should automatically check pt.
                             This will only take effect of mode is on_time/CHECK_ON_TIME
                             Should ideally be a value greater than 60 seconds, default is 120 seconds
            Usage:
              --check_pt
                Immediately check point the definition held in the server
              --check_pt=never
                Switch off check pointing
              --check_pt=on_time
                Start automatic check pointing at the interval stored in the server
              --check_pt=180
                Change the check pt interval to 180 seconds
              --check_pt=on_time:90
                Change mode and interval, to automatic check pointing every 90 seconds
              --check_pt=alarm:35
                Change the alarm time for check pt saves. i.e if saving the check pt takes longer than 35 seconds
                set the late flag on the server.

    .. tab:: Request

        ``--check_pt=always:120:30`` (``mode_`` ``2`` is ``always``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CheckPtCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "mode_": 2,
                      "check_pt_interval_": 120,
                      "check_pt_save_time_alarm_": 30
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_ClientHandleCmd:

ClientHandleCmd
~~~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ClientHandleCmd.hpp``
   * - Action
     - one option per handle operation (see the *Actions* table below)
   * - Parameters
     - - ``client_handle`` (most actions): the handle identifier (``client_handle_``).
       - ``suites`` (register/add): the suites to associate with the handle (``suites_``).
       - ``auto_add_new_suites`` (register): whether new suites are added automatically
         (``auto_add_new_suites_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Manages *client handles*: server-side registrations that let a client (typically ``ecflow_ui``)
track a chosen subset of suites and receive only their changes. The operation is carried by ``api_``:

.. list-table::
   :header-rows: 1
   :widths: 26 10 42 22

   * - Option
     - ``api_``
     - Description
     - Reply
   * - ``--ch_register``
     - 0
     - Register a new handle for a set of suites.
     - handle
   * - ``--ch_drop``
     - 1
     - Drop a handle.
     - ``StcCmd`` (OK)
   * - ``--ch_drop_user``
     - 2
     - Drop all handles owned by a user.
     - ``StcCmd`` (OK)
   * - ``--ch_add``
     - 3
     - Add suites to a handle.
     - ``StcCmd`` (OK)
   * - ``--ch_rem``
     - 4
     - Remove suites from a handle.
     - ``StcCmd`` (OK)
   * - ``--ch_auto_add``
     - 5
     - Change the auto-add-new-suites flag of a handle.
     - ``StcCmd`` (OK)
   * - ``--ch_suites``
     - 6
     - List registered handles and their suites.
     - suites

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --ch_register 1 /s1 /s2
            ecflow_client --ch_drop 1

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-clienthandlecmd-ch-register">--ch_register</option>
            <option value="ecf-h-clienthandlecmd-ch-drop">--ch_drop</option>
            <option value="ecf-h-clienthandlecmd-ch-drop-user">--ch_drop_user</option>
            <option value="ecf-h-clienthandlecmd-ch-add">--ch_add</option>
            <option value="ecf-h-clienthandlecmd-ch-rem">--ch_rem</option>
            <option value="ecf-h-clienthandlecmd-ch-auto-add">--ch_auto_add</option>
            <option value="ecf-h-clienthandlecmd-ch-suites">--ch_suites</option>
            </select></label>
            <pre id="ecf-h-clienthandlecmd-ch-register" class="ecf-help-panel">ch_register
            -----------

            Register interest in a set of suites.
            If a definition has lots of suites, but the client. is only interested in a small subset,
            Then using this command can reduce network bandwidth and synchronisation will be quicker.
            This command will create a client handle, which must be used for any other changes.
            The newly created handle can be shown with the --ch_suites command
            Deleted suites will stay registered, and must be explicitly removed/dropped.
            Note: Suites can be registered before they are loaded into the server
            This command affects news() and sync() commands
               arg1 = true | false           # true means add new suites to my list, when they are created
               arg2 = names                  # should be a list of suite names, names not in the definition are ignored
            Usage:
               --ch_register=true s1 s2 s3   # register interest in suites s1,s2,s3 and any new suites
               --ch_register=false s1 s2 s3  # register interest in suites s1,s2,s3 only
               --ch_register=false           # register handle, suites will be added later on
               --ch_register=1 true s1 s2 s3 # drop handle 1 then register interest in suites s1,s2,s3 and any new suites
                                             # The client handle as the first argument is typically used by GUI/python                                 # When the client handle is no zero, then it is dropped first
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-drop" class="ecf-help-panel" hidden>ch_drop
            -------

            Drop/de-register the client handle.
            Un-used handle should be dropped otherwise they will stay, in the server.
               arg1 = handle(integer)  # The handle must be an integer that is &gt; 0
            Usage:
               --ch_drop=10            # drop the client handle 10
            An error is returned if the handle had not previously been registered
            The handle stored on the local client is set to zero
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-drop-user" class="ecf-help-panel" hidden>ch_drop_user
            ------------

            Drop/de-register all handles associated with the given user.
            If no user provided will drop for current user. Client must ensure un-used handle are dropped
            otherwise they will stay, in the server.
               arg1 = user           # The user to be drooped, if left empty drop current user 
            Usage:
               --ch_drop_user=ma0    # drop all handles associated with user ma0
               --ch_drop_user        # drop all handles associated with current user
            An error is returned if no registered handles
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-add" class="ecf-help-panel" hidden>ch_add
            ------

            Add a set of suites, to an existing handle.
               arg1 = handle(integer)  # The handle must be an integer that is &gt; 0
               arg2 = names            # should be a list of suite names, names not in the definition are ignored
            Usage:
               --ch_add=10 s2 s3 s4    # add suites s2 s3,s4 to  handle 10
            An error is returned if the handle had not previously been registered
            The handle is created with --ch_register command
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-rem" class="ecf-help-panel" hidden>ch_rem
            ------

            Remove a set of suites, from an existing handle.
               arg1 = handle(integer)   # The handle must be an integer that is &gt; 0
               arg2 = names             # should be a list of suite names, names not in the definition are ignored
            Usage:
               --ch_rem=10 s2 s3 s4     # remove suites s2 s3,s4 from handle 10
            The handle is created with --ch_register command
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-auto-add" class="ecf-help-panel" hidden>ch_auto_add
            -----------

            Change an existing handle so that new suites can be added automatically.
               arg1 = handle(integer)  # The handle must be an integer that is &gt; 0
               arg2 = true | false     # true means add new suites to my list, when they are created
            Usage:
             --ch_auto_add=10 true     # modify handle 10 so that new suites, get added automatically to it
             --ch_auto_add=10 false    # modify handle 10 so that no new suites are added
            The handle is created with --ch_register command
            To list all suites and handles use --ch_suites</pre>
            <pre id="ecf-h-clienthandlecmd-ch-suites" class="ecf-help-panel" hidden>ch_suites
            ---------

            Shows all the client handles, and the suites they reference</pre>
            </div>

    .. tab:: Request

        ``--ch_register`` for suites ``/s1`` and ``/s2`` with auto-add enabled (``api_`` ``0``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "ClientHandleCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 0,
                      "client_handle_": 0,
                      "drop_user_": "",
                      "suites_": [ "/s1", "/s2" ],
                      "auto_add_new_suites_": true
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``--ch_register`` returns an ``SClientHandleCmd`` whose ``handle_`` field is the newly
        allocated handle:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SClientHandleCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "handle_": 42
                    }
                  }
                }
              }
            }

        ``--ch_suites`` returns a string listing of the handles; the remaining actions return the
        generic ``StcCmd`` reply with ``api_`` equal to ``0``.

.. _proto_CSyncCmd:

CSyncCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/CSyncCmd.hpp``
   * - Action
     - one option per synchronisation mode (see the *Actions* table below)
   * - Parameters
     - - ``client_handle`` (optional): the handle whose suites are synchronised (``client_handle_``).
       - ``client_state_change_no`` / ``client_modify_change_no``: the change numbers the client
         already holds, so the server can compute an incremental update.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Keeps a client in step with the server. It is issued repeatedly by clients such as ``ecflow_ui``
rather than typed by an operator. The mode is carried by ``api_``:

.. list-table::
   :header-rows: 1
   :widths: 26 10 42 22

   * - Option
     - ``api_``
     - Description
     - Reply
   * - ``--news``
     - 0
     - Ask whether anything has changed since the given change numbers.
     - news
   * - ``--sync``
     - 1
     - Return an incremental update.
     - sync
   * - ``--sync_full``
     - 2
     - Return the full definition.
     - sync
   * - ``--sync_clock``
     - 3
     - Return a clock-only update.
     - sync

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            # issued internally by clients; not normally typed directly
            ecflow_client --sync 1 10 5

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-csynccmd-news">--news</option>
            <option value="ecf-h-csynccmd-sync">--sync</option>
            <option value="ecf-h-csynccmd-sync-full">--sync_full</option>
            <option value="ecf-h-csynccmd-sync-clock">--sync_clock</option>
            </select></label>
            <pre id="ecf-h-csynccmd-news" class="ecf-help-panel">news
            ----

            Returns true if state of server definition changed.
            *Important* for use with c++/python interface only.
            Requires Given a client handle, change and modify number determine if server changed since last call
            This relies on user calling sync after news to update the locally stored modify and change numbers.
            These numbers are then used in the next call to news.</pre>
            <pre id="ecf-h-csynccmd-sync" class="ecf-help-panel" hidden>sync
            ----

            Incrementally synchronise the local definition with the one in the server.
            *Important* for use with c++/python interface only.
            Preference should be given to this method as only the changes are returned.
            This reduces the network bandwidth required to keep in sync with the server
            Requires a client handle, change and modify number, to get the incremental changes from server.
            The change in server state is then and merged with the client definition.</pre>
            <pre id="ecf-h-csynccmd-sync-full" class="ecf-help-panel" hidden>sync_full
            ---------

            Returns the full definition from the server.
            *Important* for use with c++/python interface only.
            Requires a client_handle. The returned definition is stored on the client.</pre>
            <pre id="ecf-h-csynccmd-sync-clock" class="ecf-help-panel" hidden>sync_clock
            ----------

            Incrementally synchronise the local definition with the one in the server.
            *Important* for use with c++/python interface only.
            Same as sync, but will *always* sync with suite clock if it has changed.
            Preference should be given to this method as only the changes are returned.
            This reduces the network bandwidth required to keep in sync with the server
            Requires a client handle, change and modify number, to get the incremental changes from server.
            The change in server state is then and merged with the client definition.</pre>
            </div>

    .. tab:: Request

        ``--sync`` for handle ``1`` at state change ``10`` and modify change ``5`` (``api_`` ``1``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CSyncCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 1,
                      "client_handle_": 1,
                      "client_state_change_no_": 10,
                      "client_modify_change_no_": 5
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The reply is an ``SSyncCmd`` (for ``--sync``, ``--sync_full`` and ``--sync_clock``) or an
        ``SNewsCmd`` (for ``--news``). These carry the incremental changes, or the full serialized
        definition, needed to bring the client up to date; their payload is unbounded and is not
        reproduced here.

.. _proto_CtsCmd:

CtsCmd
~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/CtsCmd.hpp``
   * - Action
     - one option per server signal (see the *Actions* table below)
   * - Parameters
     - - none; each action is a bare signal to the server.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Encapsulates the many simple, argument-less signals to the server in a single class, to limit the
number of global serialisation symbols. Each command-line option maps to one value of the ``Api``
enumeration, carried in the payload as ``api_``. Most actions return the generic success reply; a few
return a string or a structured reply, as noted in the table:

.. list-table::
   :header-rows: 1
   :widths: 26 12 40 22

   * - Option
     - ``api_``
     - Description
     - Reply
   * - ``--restore_from_checkpt``
     - 1
     - Restore the definition from the checkpoint file.
     - ``StcCmd`` (OK)
   * - ``--restart``
     - 2
     - Restart the server (resume scheduling).
     - ``StcCmd`` (OK)
   * - ``--shutdown``
     - 3
     - Stop scheduling new jobs; keep serving requests.
     - ``StcCmd`` (OK)
   * - ``--halt``
     - 4
     - Stop scheduling and stop most request handling.
     - ``StcCmd`` (OK)
   * - ``--terminate``
     - 5
     - Terminate the server process.
     - ``StcCmd`` (OK)
   * - ``--reloadwsfile``
     - 6
     - Reload the white-list file.
     - ``StcCmd`` (OK)
   * - ``--force-dep-eval``
     - 7
     - Force an immediate dependency evaluation.
     - ``StcCmd`` (OK)
   * - ``--ping``
     - 8
     - Check that the server is reachable.
     - ``StcCmd`` (OK)
   * - ``--zombie_get``
     - 9
     - Retrieve the list of zombies.
     - zombie list
   * - ``--stats``
     - 10
     - Return server statistics as text.
     - string
   * - ``--suites``
     - 11
     - Return the list of suites as text.
     - string
   * - ``--debug_server_on``
     - 12
     - Enable server-side debug output.
     - ``StcCmd`` (OK)
   * - ``--debug_server_off``
     - 13
     - Disable server-side debug output.
     - ``StcCmd`` (OK)
   * - ``--server_load``
     - 14
     - Return the path of the log file for load plotting.
     - server load
   * - ``--stats_reset``
     - 15
     - Reset the server statistics.
     - ``StcCmd`` (OK)
   * - ``--reloadpasswdfile``
     - 16
     - Reload the password file.
     - ``StcCmd`` (OK)
   * - ``--stats_server``
     - 17
     - Return the statistics structure (test use only).
     - stats structure
   * - ``--reloadcustompasswdfile``
     - 18
     - Reload the custom-user password file.
     - ``StcCmd`` (OK)

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --ping
            ecflow_client --restart
            ecflow_client --suites

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-ctscmd-ping">--ping</option>
            <option value="ecf-h-ctscmd-restore-from-checkpt">--restore_from_checkpt</option>
            <option value="ecf-h-ctscmd-restart">--restart</option>
            <option value="ecf-h-ctscmd-shutdown">--shutdown</option>
            <option value="ecf-h-ctscmd-halt">--halt</option>
            <option value="ecf-h-ctscmd-terminate">--terminate</option>
            <option value="ecf-h-ctscmd-reloadwsfile">--reloadwsfile</option>
            <option value="ecf-h-ctscmd-force-dep-eval">--force-dep-eval</option>
            <option value="ecf-h-ctscmd-zombie-get">--zombie_get</option>
            <option value="ecf-h-ctscmd-stats">--stats</option>
            <option value="ecf-h-ctscmd-suites">--suites</option>
            <option value="ecf-h-ctscmd-debug-server-on">--debug_server_on</option>
            <option value="ecf-h-ctscmd-debug-server-off">--debug_server_off</option>
            <option value="ecf-h-ctscmd-server-load">--server_load</option>
            <option value="ecf-h-ctscmd-stats-reset">--stats_reset</option>
            <option value="ecf-h-ctscmd-reloadpasswdfile">--reloadpasswdfile</option>
            <option value="ecf-h-ctscmd-stats-server">--stats_server</option>
            <option value="ecf-h-ctscmd-reloadcustompasswdfile">--reloadcustompasswdfile</option>
            </select></label>
            <pre id="ecf-h-ctscmd-ping" class="ecf-help-panel">ping
            ----

            Check if server is running on given host/port. Result reported to standard output.
            Usage:
              --ping --host=mach --port=3144  # Check if server alive on host mach &amp; port 3144
              --ping --host=fred              # Check if server alive on host fred and port ECF_PORT,
                                              # otherwise default port of 3141
              --ping                          # Check if server alive by using environment variables
                                              # ECF_HOST and ECF_PORT
            If ECF_HOST not defined uses &#x27;localhost&#x27;, if ECF_PORT not defined assumes 3141</pre>
            <pre id="ecf-h-ctscmd-restore-from-checkpt" class="ecf-help-panel" hidden>restore_from_checkpt
            --------------------

            Ask the server to load the definition from an check pt file.
            The server must be halted and the definition in the server must be deleted
            first, otherwise an error is returned</pre>
            <pre id="ecf-h-ctscmd-restart" class="ecf-help-panel" hidden>restart
            -------

            Start job scheduling, communication with jobs, and respond to all requests.
            The following table shows server behaviour in the different states.
            |----------------------------------------------------------------------------------|
            | Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |
            |--------------|--------------|--------------|---------------|---------------------|
            |     RUNNING  |    yes       |      yes     |      yes      |      yes            |
            |    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |
            |      HALTED  |    yes       |      no      |      no       |      no             |
            |--------------|--------------|--------------|---------------|---------------------|</pre>
            <pre id="ecf-h-ctscmd-shutdown" class="ecf-help-panel" hidden>shutdown
            --------

            Stop server from scheduling new jobs.
              arg1 = yes(optional) # use to bypass confirmation prompt,i.e
              --shutdown=yes
            The following table shows server behaviour in the different states.
            |----------------------------------------------------------------------------------|
            | Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |
            |--------------|--------------|--------------|---------------|---------------------|
            |     RUNNING  |    yes       |      yes     |      yes      |      yes            |
            |    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |
            |      HALTED  |    yes       |      no      |      no       |      no             |
            |--------------|--------------|--------------|---------------|---------------------|</pre>
            <pre id="ecf-h-ctscmd-halt" class="ecf-help-panel" hidden>halt
            ----

            Stop server communication with jobs, and new job scheduling.
            Also stops automatic check pointing
              arg1 = yes(optional) # use to bypass confirmation prompt,i.e.
              --halt=yes
            The following table shows server behaviour in the different states.
            |----------------------------------------------------------------------------------|
            | Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |
            |--------------|--------------|--------------|---------------|---------------------|
            |     RUNNING  |    yes       |      yes     |      yes      |      yes            |
            |    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |
            |      HALTED  |    yes       |      no      |      no       |      no             |
            |--------------|--------------|--------------|---------------|---------------------|</pre>
            <pre id="ecf-h-ctscmd-terminate" class="ecf-help-panel" hidden>terminate
            ---------

            Terminate the server.
              arg1 = yes(optional) # use to bypass confirmation prompt.i.e
              --terminate=yes</pre>
            <pre id="ecf-h-ctscmd-reloadwsfile" class="ecf-help-panel" hidden>reloadwsfile
            ------------

            Reload the white list file.

            The white list file (authorisation) is used to verify if a &#x27;user&#x27; is allowed to perform a
            specific command.

            The file path is specified as the ECF_LISTS variable, and loaded only once by the server
            (on *startup*). This means that the file contents can be updated, but the file location
            cannot change during the server execution.

            The ECF_LISTS variable can be used as follows:
              - if ECF_LISTS is not specified, or if it is specified with value `ecf.lists`,
                then the server will use the value `&lt;host&gt;.&lt;port&gt;.ecf.lists`
              - if ECF_LISTS is specified to be a path, such as /var/tmp/ecf.lists,
                then the server will use this path to reload the white list file

            The server automatically loads the white list file content as part of the startup procedure,
            considering that if the file is not present or is empty (i.e., just contains the version
            number) then all users have read/write access.

            The reload operation will fail if file does not exist or if the content is invalid.

            Expected format for this file is:


            # all characters after the first # in a line are considered comments and are discarded
            # empty lines are also discarded

            4.4.14  # the version number is mandatory, even if no users are specified
            # Users with read/write access
            user1
            user2   # comment
            *       # use this form if you want all users to have read/write access

            # Users with read  access, must have - before user name
            -user3  # comment
            -user4
            -*      # use this form if you want all users to have read access


            Usage:
             --reloadwsfile</pre>
            <pre id="ecf-h-ctscmd-force-dep-eval" class="ecf-help-panel" hidden>force-dep-eval
            --------------

            Force dependency evaluation. Used for DEBUG only.</pre>
            <pre id="ecf-h-ctscmd-zombie-get" class="ecf-help-panel" hidden>zombie_get
            ----------

            Returns the list of zombies from the server.
            Results reported to standard output.</pre>
            <pre id="ecf-h-ctscmd-stats" class="ecf-help-panel" hidden>stats
            -----

            Returns the server statistics as a string.</pre>
            <pre id="ecf-h-ctscmd-suites" class="ecf-help-panel" hidden>suites
            ------

            Returns the list of suites, in the order defined in the server.</pre>
            <pre id="ecf-h-ctscmd-debug-server-on" class="ecf-help-panel" hidden>debug_server_on
            ---------------

            Enables debug output from the server</pre>
            <pre id="ecf-h-ctscmd-debug-server-off" class="ecf-help-panel" hidden>debug_server_off
            ----------------

            Disables debug output from the server</pre>
            <pre id="ecf-h-ctscmd-server-load" class="ecf-help-panel" hidden>server_load
            -----------

            Generates gnuplot files that show the server load graphically.
            This is done by parsing the log file. If no log file is provided,
            then the log file path is obtained from the server. If the returned
            log file path is not accessible an error is returned
            This command produces a three files in the CWD.
                o &lt;host&gt;.&lt;port&gt;.gnuplot.dat
                o &lt;host&gt;.&lt;port&gt;.gnuplot.script
                o &lt;host&gt;.&lt;port&gt;.png

            The generated script can be manually changed, to see different rendering
            effects. i.e. just run &#x27;gnuplot &lt;host&gt;.&lt;port&gt;.gnuplot.script&#x27;

              arg1 = &lt;optional&gt; path to log file

            If the path to log file is known, it is *preferable* to use this,
            rather than requesting the log path from the server.

            Usage:
               --server_load=/path/to_log_file  # Parses log and generate gnuplot files
               --server_load                    # Log file path is requested from server
                                                # which is then used to generate gnuplot files
                                                # *AVOID* if log file path is accessible

            Now use any png viewer to see the output i.e

            &gt; display   &lt;host&gt;.&lt;port&gt;.png
            &gt; feh       &lt;host&gt;.&lt;port&gt;.png
            &gt; eog       &lt;host&gt;.&lt;port&gt;.png
            &gt; xdg-open  &lt;host&gt;.&lt;port&gt;.png
            &gt; w3m       &lt;host&gt;.&lt;port&gt;.png</pre>
            <pre id="ecf-h-ctscmd-stats-reset" class="ecf-help-panel" hidden>stats_reset
            -----------

            Resets the server statistics.</pre>
            <pre id="ecf-h-ctscmd-reloadpasswdfile" class="ecf-help-panel" hidden>reloadpasswdfile
            ----------------

            Reload the server password file.

            The password file (authentication) is used by the server to authenticate a &#x27;user&#x27; by
            verifying if the password provided by the user matches the one held by the server.
            The password file is also used on the client to automatically load the password for the
            &#x27;user&#x27; when connecting to the server.

            When the server is configured to use a password file, then ALL users must have a password.

            The file path is specified as the ECF_PASSWD environment variable, both for the client and
            server, and is loaded only by the server on *startup*. This means that the file contents
            can be updated (i.e., add/remove users), but the file location cannot change during the
            server execution.

            The server automatically loads the password file content as part of the startup procedure.

            The ECF_PASSWD environment variable is used to specify the password file location,
            considering that
             - On the server, the default file name is &lt;host&gt;.&lt;port&gt;.ecf.passwd
             - On the client, the default file name is ecf.passwd

            The format of the file is same for client and server:


            4.5.0
            # comment
            &lt;user&gt; &lt;host&gt; &lt;port&gt; &lt;passwd&gt; # comment

            The following is an example

            4.5.0 # the version
            fred machine1 3142 xxyyyd
            fred machine2 3133 xxyyyd # comment
            bill machine2 3133 xxyggyyd


            Notice that the same user may appear multiple times (associated with different host/port).
            This allows the client to use the same password file to contact multiple servers.

            For the password authentication to work, ensure the following:
             - The password is defined for the client and server
             - On the server, add at least the server administrator to the password file
               Note: If an empty password file (i.e., containing just the version) is used,
                     no user is allowed access.
             - On the client, the password file should be readable only by the &#x27;user&#x27; itself

            Usage:
             --reloadpasswdfile</pre>
            <pre id="ecf-h-ctscmd-stats-server" class="ecf-help-panel" hidden>stats_server
            ------------

            Returns the server statistics as a struct and string. For test use only.</pre>
            <pre id="ecf-h-ctscmd-reloadcustompasswdfile" class="ecf-help-panel" hidden>reloadcustompasswdfile
            ----------------------

            Reload the server custom password file.

            The custom password file (authentication) is used by the server to authenticate a &#x27;user&#x27; by
            verifying if the password provided by the user matches the one held by the server. This
            particular file is used for authentication of users that explicitly specify the user name
            (either via the environment variable ECF_USER or the --user option).

            This mechanism should be used when most users use the machine login name, but a few users
            specify their own user name, in which case the password must also be explicitly provided.

            The file path is specified as the ECF_CUSTOM_PASSWD environment variable, both for the
            client and server, and is loaded only by the server on *startup*. This means that the file
            contents can be updated (i.e., add/remove users), but the file location cannot change during
            the server execution.

            The server automatically loads the password file content as part of the startup procedure.

            The ECF_CUSTOM_PASSWD environment variable is used to specify the password file location,
            considering that
             - On the server the default file name is &lt;host&gt;.&lt;port&gt;.ecf.custom_passwd
             - On the client the default file name is ecf.custom_passwd

            The format of the file is same for client and server:


            4.5.0
            # comment
            &lt;user&gt; &lt;host&gt; &lt;port&gt; &lt;passwd&gt; # comment

            The following is an example

            4.5.0 # the version
            fred machine1 3142 xxyyyd
            fred machine2 3133 xxyyyd # comment
            bill machine2 3133 xxyggyyd

            Notice that the same user may appear multiple times (associated with different host/port).
            This allows the client to use the same password file to contact multiple servers.

            For the password authentication to work, ensure the following:
             - The password is defined for the client and server
             - On the server, add at least the server administrator to the password file
               Note: If an empty password file (i.e., containing just the version) is used,
                     no user is allowed access.
             - On the client, the password file should be readable only by the &#x27;user&#x27; itself

            Usage:
             --reloadcustompasswdfile</pre>
            </div>

    .. tab:: Request

        ``--ping`` (``api_`` ``8``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 8
                    }
                  }
                }
              }
            }

        ``--restart`` (``api_`` ``2``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 2
                    }
                  }
                }
              }
            }

        ``--suites`` (``api_`` ``11``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 11
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        Actions such as ``--ping`` and ``--restart`` return the generic success reply (``StcCmd``
        with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

        Actions that return text, such as ``--stats`` and ``--suites``, reply with an ``SStringCmd``
        whose ``str_`` field holds the result:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "a string reply"
                    }
                  }
                }
              }
            }

.. _proto_CtsNodeCmd:

CtsNodeCmd
~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/CtsNodeCmd.hpp``
   * - Action
     - one option per query (see the *Actions* table below)
   * - Parameters
     - - ``arg1`` (optional node path): the node to query; when omitted the whole definition is
         used.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Queries the server about a node or the whole definition. The query is carried by ``api_`` and the
target by ``absNodePath_``:

.. list-table::
   :header-rows: 1
   :widths: 26 10 42 22

   * - Option
     - ``api_``
     - Description
     - Reply
   * - ``--job_gen``
     - 1
     - Force job generation under the node.
     - ``StcCmd`` (OK)
   * - ``--check_job_gen_only``
     - 2
     - Test job generation without submitting.
     - ``StcCmd`` (OK)
   * - ``--get``
     - 3
     - Return the definition (or node subtree).
     - definition
   * - ``--why``
     - 4
     - Explain why a node is not running.
     - string
   * - ``--get_state``
     - 5
     - Return the definition with node states.
     - definition
   * - ``--migrate``
     - 6
     - Return a representation suitable for migration.
     - string

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --why=/suite/family/task
            ecflow_client --get=/suite/family/task

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-ctsnodecmd-get">--get</option>
            <option value="ecf-h-ctsnodecmd-why">--why</option>
            <option value="ecf-h-ctsnodecmd-get-state">--get_state</option>
            <option value="ecf-h-ctsnodecmd-migrate">--migrate</option>
            <option value="ecf-h-ctsnodecmd-job-gen">--job_gen</option>
            <option value="ecf-h-ctsnodecmd-checkjobgenonly">--checkJobGenOnly</option>
            </select></label>
            <pre id="ecf-h-ctsnodecmd-get" class="ecf-help-panel">get
            ---

            Get the suite definition or node tree in form that is re-parse able
            Get all suite node tree&#x27;s from the server and write to standard out.
            The output is parse-able, and can be used to re-load the definition
              arg = NULL | arg = node path
            Usage:
              --get     # gets the definition from the server,and writes to standard out
              --get=/s1 # gets the suite from the server,and writes to standard out</pre>
            <pre id="ecf-h-ctsnodecmd-why" class="ecf-help-panel" hidden>why
            ---

            Show the reason why a node is not running.
            Can only be used with the group command. The group command must include a 
            &#x27;get&#x27; command(i.e returns the server defs)
            The why command take a optional string argument representing a node path
            Will return reason why the node is holding and for all its children.
            If no arguments supplied will report on all nodes
              arg = node path | arg = NULL
            Usage:
              --group=&quot;get; why&quot;               # returns why for all holding nodes
              --group=&quot;get; why=/suite/family&quot; # returns why for a specific node</pre>
            <pre id="ecf-h-ctsnodecmd-get-state" class="ecf-help-panel" hidden>get_state
            ---------

            Get state data. For the whole suite definition or individual nodes.
            This will include event, meter, node state, trigger and time state.
            The output is written to standard out.
              arg = NULL | arg = node path
            Usage:
              --get_state     # gets the definition from the server,and writes to standard out
              --get_state=/s1 # gets the suite from the server,and writes to standard out</pre>
            <pre id="ecf-h-ctsnodecmd-migrate" class="ecf-help-panel" hidden>migrate
            -------

            Used to print state of the definition returned from the server to standard output.
            The node state is shown in the comments.
            This is the format used in the check point file, but with indentation.
            Since this is essentially the defs format with state in the comments,it allows the definition to be migrated to future version of ecflow.
            The output includes edit history but excludes externs.
            When the definition is reloaded *NO* checking is done.

            The following shows a summary of the features associated with each choice:
                                   --get  --get_state  --migrate
            Auto generate externs    Yes    Yes          No
            Checking on reload       Yes    Yes          No
            Edit History             No     No           Yes
            Show trigger AST         No     Yes          No

            Usage:
                --migrate         # show all suites
                --migrate=/s1     # show state for suite s1</pre>
            <pre id="ecf-h-ctsnodecmd-job-gen" class="ecf-help-panel" hidden>job_gen
            -------

            Job submission for chosen Node *based* on dependencies.
            The server traverses the node tree every 60 seconds, and if the dependencies are free
            does job generation and submission. Sometimes the user may free time/date dependencies
            to avoid waiting for the server poll, this commands allows early job generation
              arg = node path | arg = NULL
                 If no node path specified generates for full definition.</pre>
            <pre id="ecf-h-ctsnodecmd-checkjobgenonly" class="ecf-help-panel" hidden>checkJobGenOnly
            ---------------

            Test hierarchical Job generation only, for chosen Node.
            The jobs are generated independent of the dependencies
            This will generate the jobs *only*, i.e. no job submission. Used for checking job generation only
              arg = node path | arg = NULL
                 If no node path specified generates for all Tasks in the definition. For Test only</pre>
            </div>

    .. tab:: Request

        ``--why=/suite/family/task`` (``api_`` ``4``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsNodeCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 4,
                      "absNodePath_": "/suite/family/task"
                    }
                  }
                }
              }
            }

        ``--get=/suite/family/task`` differs only in ``api_`` being ``3``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "CtsNodeCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 3,
                      "absNodePath_": "/suite/family/task"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``--job_gen`` and ``--check_job_gen_only`` return the generic ``StcCmd`` reply with ``api_``
        equal to ``0``:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

        ``--why`` and ``--migrate`` return an ``SStringCmd`` (the same string-reply shape used by the
        other string-returning commands). ``--get``
        and ``--get_state`` return a ``DefsCmd`` whose payload embeds the serialized definition or
        node subtree; it is omitted here because its size is unbounded.

.. _proto_DeleteCmd:

DeleteCmd
~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/DeleteCmd.hpp``
   * - Action
     - ``--delete``
   * - Parameters
     - - ``arg1`` (mandatory): ``_all_`` to delete every suite, or one or more node paths.
       - ``force`` (optional): delete even nodes that are active or submitted.
       - ``yes`` (optional): skip the interactive confirmation prompt.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Removes nodes from the server. The paths are carried in ``paths_`` and the ``force`` flag in
``force_``. Deleting every suite (``--delete _all_``) returns a distinct reply.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --delete=/suite/family
            ecflow_client --delete force yes /suite/family/task
            ecflow_client --delete _all_

    .. tab:: Help

        Verbatim ``ecflow_client --delete`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            delete
            ------

            Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.
              arg1 = [ force | yes ](optional)  # Use this parameter to bypass checks, i.e. for active or submitted tasks
              arg2 = yes(optional)              # Use 'yes' to bypass the confirmation prompt
              arg3 = node paths | _all_         # _all_ means delete all suites
                                                # node paths must start with a leading '/'
            Usage:
              --delete=_all_                    # Delete all suites in server. Use with care.
              --delete=/suite/f1/t1             # Delete node at /suite/f1/t1. This will prompt
              --delete=force /suite/f1/t1       # Delete node at /suite/f1/t1 even if active or submitted
              --delete=force yes /s1 /s2        # Delete suites s1,s2 even if active or submitted, bypassing prompt

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "DeleteCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "force_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        Deleting specific nodes returns the generic ``StcCmd`` reply with ``api_`` equal to ``0``.
        Deleting every suite (``--delete _all_``) instead returns ``StcCmd`` with ``api_`` equal to
        ``3`` (``DELETE_ALL``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 3
                    }
                  }
                }
              }
            }

.. _proto_EditScriptCmd:

EditScriptCmd
~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/EditScriptCmd.hpp``
   * - Action
     - ``--edit_script``
   * - Parameters
     - - ``arg1`` (node path): the task whose script is edited (``path_to_node_``).
       - ``arg2`` (sub-command): selects the operation, encoded in ``edit_type_`` — ``edit`` (``0``),
         ``pre_process`` (``1``), ``submit`` (``2``), ``pre_process_file`` (``3``) or
         ``submit_file`` (``4``).
       - additional arguments carry a user file (``user_file_contents_``), user variables
         (``user_variables_``), and the ``alias_`` / ``run_`` flags.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Retrieves or submits the script of a task, with optional preprocessing and variable substitution. It
underlies the "edit script" workflow in ``ecflow_ui``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --edit_script /suite/family/task pre_process > script.pp

    .. tab:: Help

        Verbatim ``ecflow_client --edit_script`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            edit_script
            -----------

            Allows user to edit, pre-process and submit the script.
            Will allow pre-processing of arbitrary file with 'pre_process_file' option
             arg1 = path to task  # The path to the task/alias
             arg2 = [ edit | pre_process | submit | pre_process_file | submit_file ]
                edit : will return the script file to standard out. The script will
                       include used variables enclosed between %comment/%end at the
                       start of the file
                pre_process: Will return the script file to standard out.The script will
                             include used variables enclosed between %comment/%end at the
                             start of the file and with all %include expanded
                submit: Will extract the used variables from the supplied file, i.e
                        between the %comment/%end and use these them to generate the
                        job using the ecf file accessible from the server
                pre_process_file: Will pre process the user supplied file.
                                  Will expand includes,variable substitution,
                                  remove manual & comment sections.
                submit_file: Like submit, but the supplied file, is submitted by the server
                             The last 2 options allow complete freedom to debug the script file
             arg3 = [ path_to_script_file ]
                      needed for option [  pre_process_file | submit_file ]
             arg4 = create_alias (optional) default value is false, for use with 'submit_file' option
             arg5 = no_run (optional) default value is false, i.e immediately run the alias
                    is no_run is specified the alias in only created
            Usage:
            --edit_script=/path/to/task edit > script_file
               server returns script with the used variables to standard out
               The user can choose to edit this file

            --edit_script=/path/to/task pre_process > pre_processed_script_file
              server will pre process the ecf file accessible from the server
              (i.e expand all %includes) and return the file to standard out

            --edit_script=/path/to/task submit script_file
              Will extract the used variables in the 'script_file' and will uses these
              variables during variable substitution of the ecf file accessible by the
              server. This is then submitted as a job

            --edit_script=/path/to/task pre_process_file file_to_pre_process
              The server will pre-process the user supplied file and return the contents
              to standard out. This pre-processing is the same as job file processing,
              but on a arbitrary file

            --edit_script=/path/to/task submit_file file_to_submit
              Will extract the used variables in the 'file_to_submit' and will uses these
              variables during variable substitution, the file is then submitted for job
              generation by the server

            --edit_script=/path/to/task submit_file file_to_submit create_alias
              Like the the previous example but will create and run as an alias

    .. tab:: Request

        ``--edit_script /suite/family/task pre_process`` (``edit_type_`` ``1``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "EditScriptCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "edit_type_": 1,
                      "path_to_node_": "/suite/family/task",
                      "user_file_contents_": [],
                      "user_variables_": [],
                      "alias_": false,
                      "run_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``edit`` and ``pre_process`` return an ``SStringCmd`` whose ``str_`` field holds the script;
        the ``submit`` variants return the generic ``StcCmd`` reply with ``api_`` equal to ``0``.

.. _proto_ForceCmd:

ForceCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ForceCmd.hpp``
   * - Action
     - ``--force``
   * - Parameters
     - - ``arg1`` (state or event): a node state (``unknown``, ``complete``, ``queued``,
         ``submitted``, ``active``, ``aborted``) or a ``<path>:<event>`` with ``set`` / ``clear``.
       - ``recursive`` (optional): apply to the node and all of its children.
       - ``full`` (optional): together with ``recursive``, set repeats to their last value.
       - ``paths`` (one or more): the nodes to force.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Forces a node into a given state, or forces an event, regardless of dependencies. ``stateOrEvent_``
holds the requested state or event, and ``recursive_`` / ``setRepeatToLastValue_`` carry the
``recursive`` and ``full`` modifiers.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --force complete /suite/family/t1 /suite/family/t2
            ecflow_client --force recursive full complete /suite/family
            ecflow_client --force clear /suite/family/t1:myevent

    .. tab:: Help

        Verbatim ``ecflow_client --force`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            force
            -----

            Force a node to a given state, or set its event.
            When a task is set to complete, it may be automatically re-queued if it has
            multiple future time dependencies. However each time we force a complete it will
            expire any time based attribute on that node. When the last time based attribute
            expires, the node will stay in a complete state.
            This behaviour allow Repeat values to be incremented interactively.
            A repeat attribute is incremented when all the child nodes are complete
            in this case the child nodes are automatically re-queued.
              arg1 = [ unknown | complete | queued | submitted | active | aborted | clear | set ]
              arg2 = (optional) recursive
                     Applies state to node and recursively to all its children
              arg3 = (optional) full
                     Set repeat variables to last value, only works in conjunction
                     with recursive option
              arg4 = path_to_node or path_to_node:<event>: paths must begin with '/'
            Usage:
              --force=complete /suite/t1 /suite/t2   # Set task t1 & t2 to complete
              --force=clear /suite/task:ev           # Clear the event 'ev' on task /suite/task
              --force=complete recursive /suite/f1   # Recursively set complete all children of /suite/f1
            Effect:
              Consider the effect of forcing complete when the current time is at 09:00
              suite s1
                 task t1; time 12:00             # will complete straight away
                 task t2; time 10:00 13:00 01:00 # will complete on fourth attempt

              --force=complete /s1/t1 /s1/t2
              When we have a time range(i.e as shown with task t2), it is re-queued and the
              next time slot is incremented for each complete, until it expires, and the task completes.
              Use the Why command, to show next run time (i.e. next time slot)

    .. tab:: Request

        ``--force complete /suite/family``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "ForceCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "stateOrEvent_": "complete",
                      "recursive_": false,
                      "setRepeatToLastValue_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_FreeDepCmd:

FreeDepCmd
~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/FreeDepCmd.hpp``
   * - Action
     - ``--free-dep``
   * - Parameters
     - - ``paths`` (one or more): the nodes whose dependencies are freed.
       - ``trigger`` / ``all`` / ``date`` / ``time`` (optional flags): which dependencies to free;
         ``trigger`` is the default (``trigger_``), ``all`` frees every kind, and ``date`` / ``time``
         free the respective time dependencies.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Frees a node from a held dependency so that it may run, without changing the dependency itself. The
flags select which dependency kinds are freed.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --free-dep /suite/family/task
            ecflow_client --free-dep trigger date time /suite/family/task

    .. tab:: Help

        Verbatim ``ecflow_client --free-dep`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            free-dep
            --------

            Free dependencies for a node. Defaults to triggers
            After freeing the time related dependencies (i.e time,today,cron)
            the next time slot will be missed.
              arg1 = (optional) trigger
              arg2 = (optional) all
                     Free trigger, date and all time dependencies
              arg3 = (optional) date
                     Free date dependencies
              arg4 = (optional) time
                     Free all time dependencies i.e time, day, today, cron
              arg5 = List of paths. At least one required. Must start with a leading '/'
            Usage:
              --free-dep=/s1/t1 /s2/t2   # free trigger dependencies for task's t1,t2
              --free-dep=all /s1/f1/t1   # free all dependencies of /s1/f1/t1
              --free-dep=date /s1/f1     # free holding date dependencies of /s1/f1

    .. tab:: Request

        ``--free-dep /suite/family/task`` (the default frees the trigger dependency):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "FreeDepCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family/task" ],
                      "trigger_": true,
                      "all_": false,
                      "date_": false,
                      "time_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_GroupCTSCmd:

GroupCTSCmd
~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/GroupCTSCmd.hpp``
   * - Action
     - ``--group``
   * - Parameters
     - - ``arg1`` (mandatory): a semicolon-separated list of commands to run as one unit
         (``cmdVec_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Bundles several commands into a single request so that they are sent and executed together. Each
member command is serialized as a polymorphic entry of ``cmdVec_``; ``cli_`` records whether the
group originated on the command line.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --group="ping; get=/suite"

    .. tab:: Help

        Verbatim ``ecflow_client --group`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            group
            -----

            Allows a series of ';' separated commands to be grouped and executed as one.
            Some commands like halt, shutdown and terminate will prompt the user. To bypass the prompt
            provide 'yes' as an additional parameter. See example below.
              arg = string
            Usage:
               --group="halt=yes; reloadwsfile; restart;"
                                             # halt server,bypass the confirmation prompt,
                                             # reload white list file, restart server
               --group="get; show"           # get server defs, and write to standard output
               --group="get=/s1; show state" # get suite 's1', and write state to standard output

    .. tab:: Request

        ``--group`` containing a single ``--ping`` (``CtsCmd`` with ``api_`` ``8``); further members
        appear as additional entries of ``cmdVec_``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "GroupCTSCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "cmdVec_": [
                        {
                          "polymorphic_id": 2147483650,
                          "polymorphic_name": "CtsCmd",
                          "ptr_wrapper": {
                            "id": 2147483650,
                            "data": {
                              "cereal_class_version": 0,
                              "value0": {
                                "value0": { "cl_host_": "host.example.com" },
                                "user_": "operator"
                              },
                              "api_": 8
                            }
                          }
                        }
                      ],
                      "cli_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The reply is a ``GroupSTCCmd`` bundling the reply of each member command in order; its shape
        depends on the members and is not reproduced here.

.. _proto_LoadDefsCmd:

LoadDefsCmd
~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/LoadDefsCmd.hpp``
   * - Action
     - ``--load``
   * - Parameters
     - - ``arg1`` (mandatory): the definition file to load.
       - ``force`` (optional): overwrite suites that already exist (``force_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Loads a suite definition into the server. The client reads the file and embeds its serialized form
in ``defs_``; ``defs_filename_`` records the original file name. The ``defs_`` field is a complete
serialized definition and can be large; it is truncated below.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --load=my_suite.def
            ecflow_client --load=my_suite.def force

    .. tab:: Help

        Verbatim ``ecflow_client --load`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            load
            ----

            Check and load definition or checkpoint file into server.
            The loaded definition will be checked for valid trigger and complete expressions,
            additionally in-limit references to limits will be validated.
            If the server already has the 'suites' of the same name, then a error message is issued.
            The suite's can be overwritten if the force option is used.
            To just check the definition and not send to server, use 'check_only'
            This command can also be used to load a checkpoint file into the server
              arg1 = path to the definition file or checkpoint file
              arg2 = (optional) [ force | check_only | print | stats ]  # default = false for all
            Usage:
            --load=/my/home/exotic.def               # will error if suites of same name exists
            --load=/my/home/exotic.def force         # overwrite suite's of same name in the server
            --load=/my/home/exotic.def check_only    # Just check, don't send to server
            --load=/my/home/exotic.def stats         # Show defs statistics, don't send to server
            --load=host1.3141.check                  # Load checkpoint file to the server
            --load=host1.3141.check print            # print definition to standard out in defs format

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "LoadDefsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "force_": false,
                      "defs_": "#5.18.0\nsuite s1\n  family f1\n    task t1\n  endfamily\nendsuite\n# ...",
                      "defs_filename_": "my_suite.def"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_LogCmd:

LogCmd
~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/LogCmd.hpp``
   * - Action
     - ``--log``
   * - Parameters
     - - ``arg1`` (sub-command): ``get`` (``api_`` ``0``), ``clear`` (``1``), ``flush`` (``2``),
         ``new`` (``3``) or ``path`` (``4``).
       - ``get_last_n_lines`` (optional, ``get`` only): how many trailing lines to return
         (``get_last_n_lines_``, default ``100``).
       - ``new_path`` (optional, ``new`` only): the path of the new log file (``new_path_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Inspects or manages the server log file. ``get`` and ``path`` return a string; ``clear``, ``flush``
and ``new`` return the generic reply.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --log=get 200
            ecflow_client --log=clear
            ecflow_client --log=path

    .. tab:: Help

        Verbatim ``ecflow_client --log`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            log
            ---

            Get,clear,flush or create a new log file.
            The user must ensure that a valid path is specified.
            Specifying '--log=get' with a large number of lines from the server,
            can consume a lot of **memory**. The log file can be a very large file,
            hence we use a default of 100 lines, optionally the number of lines can be specified.
             arg1 = [ get | clear | flush | new | path ]
              get -   Outputs the log file to standard out.
                      defaults to return the last 100 lines
                      The second argument can specify how many lines to return
              clear - Clear the log file of its contents.
              flush - Flush and close the log file. (only temporary) next time
                      server writes to log, it will be opened again. Hence it best
                      to halt the server first
              new -   Flush and close the existing log file, and start using the
                      the path defined for ECF_LOG. By changing this variable
                      a new log file path can be used
                      Alternatively an explicit path can also be provided
                      in which case ECF_LOG is also updated
              path -  Returns the path name to the existing log file
             arg2 = [ new_path | optional last n lines ]
                     if get specified can specify lines to get. Value must be convertible to an integer
                     Otherwise if arg1 is 'new' then the second argument must be a path
            Usage:
              --log=get                        # Write the last 100 lines of the log file to standard out
              --log=get 200                    # Write the last 200 lines of the log file to standard out
              --log=clear                      # Clear the log file. The log is now empty
              --log=flush                      # Flush and close log file, next request will re-open log file
              --log=new /path/to/new/log/file  # Close and flush log file, and create a new log file, updates ECF_LOG
              --log=new                        # Close and flush log file, and create a new log file using ECF_LOG variable

    .. tab:: Request

        ``--log=get`` (``api_`` ``0``; ``get_last_n_lines_`` defaults to ``100``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "LogCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 0,
                      "get_last_n_lines_": 100,
                      "new_path_": ""
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``get`` and ``path`` return an ``SStringCmd`` whose ``str_`` field holds the requested lines
        or the log path:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "..."
                    }
                  }
                }
              }
            }

        ``clear``, ``flush`` and ``new`` return the generic ``StcCmd`` reply with ``api_`` equal to
        ``0``.

.. _proto_LogMessageCmd:

LogMessageCmd
~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/LogMessageCmd.hpp``
   * - Action
     - ``--msg``
   * - Parameters
     - - ``arg1`` (mandatory): the message to write to the server log (``msg_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Writes an arbitrary message into the server log file, useful for annotating operations.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --msg="starting the morning cycle"

    .. tab:: Help

        Verbatim ``ecflow_client --msg`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            msg
            ---

            Writes the input string to the log file.
              arg1 = string
            Usage:
              --msg="place me in the log file"

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "LogMessageCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "msg_": "a user message"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_MoveCmd:

MoveCmd
~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command (internal)
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/MoveCmd.hpp``
   * - Action
     - none (internal)
   * - Parameters
     - - ``src_node_``: the serialized source node subtree.
       - ``src_host_`` / ``src_port_`` / ``src_path_``: the origin server and path.
       - ``dest_``: the destination parent path.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Carries a node subtree from one location (or server) to another. It has no command-line option of
its own: it is generated by ``PlugCmd`` and by drag-and-drop in ``ecflow_ui``. The ``src_node_``
field embeds the serialized subtree, so the payload is unbounded; its structure mirrors the other
user commands, with the base-class ``cl_host_`` and ``user_`` followed by the fields above. The reply
is the generic ``StcCmd`` success reply with ``api_`` equal to ``0``.

.. _proto_OrderNodeCmd:

OrderNodeCmd
~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/OrderNodeCmd.hpp``
   * - Action
     - ``--order``
   * - Parameters
     - - ``arg1`` (node path): the node to reorder among its siblings (``absNodepath_``).
       - ``arg2`` (order): ``top`` (``option_`` ``0``), ``bottom`` (``1``), ``alpha`` (``2``),
         ``order`` (``3``), ``up`` (``4``), ``down`` (``5``) or ``runtime`` (``6``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Changes the position of a node among its siblings, which determines display order and, for some
attributes, evaluation order. The ordering is carried by ``option_``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --order=/suite/family/task top

    .. tab:: Help

        Verbatim ``ecflow_client --order`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            order
            -----

            Re-orders the nodes held by the server
              arg1 = node path
              arg2 = [ top | bottom | alpha | order | up | down | runtime]
            It should be noted that in the absence of triggers and time/date dependencies,
            the tasks are submitted in order.
            This changes the order and hence affects the submission order::

               o top     raises the node within its parent, so that it is first
               o bottom  lowers the node within its parent, so that it is last
               o alpha   Arranges for all the peers of selected note to be sorted alphabetically (case-insensitive)
               o order   Arranges for all the peers of selected note to be sorted in reverse alphabet(case-insensitive)
               o up      Moves the selected node up one place amongst its peers
               o down    Moves the selected node down one place amongst its peers

               o runtime Orders the nodes according to state change runtime
                         for families by accumulated runtime of its children
                         useful to submit the task that take longer earlier

            This command can fail because:
            - The node path does not exist in the server
            - The order_type is not does not match one of arg2
            Usage:
              --order=/suite/f1 top  # move node f1 to the top

    .. tab:: Request

        ``--order=/suite/family/task top`` (``option_`` ``0``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "OrderNodeCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "absNodepath_": "/suite/family/task",
                      "option_": 0
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_PathsCmd:

PathsCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/PathsCmd.hpp``
   * - Action
     - one option per node operation (see the *Actions* table below)
   * - Parameters
     - - ``paths`` (one or more): the nodes to operate on.
       - ``force`` (optional, ``--kill`` only): kill even when already being killed.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Applies a common node operation to one or more paths. The operation is carried by ``api_`` and the
targets by ``paths_``:

.. list-table::
   :header-rows: 1
   :widths: 24 10 44 22

   * - Option
     - ``api_``
     - Description
     - Reply
   * - ``--suspend``
     - 1
     - Suspend the node, preventing it from being scheduled.
     - ``StcCmd`` (OK)
   * - ``--resume``
     - 2
     - Resume a suspended node.
     - ``StcCmd`` (OK)
   * - ``--kill``
     - 3
     - Run ``ECF_KILL_CMD`` for the node's active jobs.
     - ``StcCmd`` (OK)
   * - ``--status``
     - 4
     - Run ``ECF_STATUS_CMD`` to query job status.
     - string
   * - ``--check``
     - 5
     - Check trigger and other expressions resolve.
     - string
   * - ``--edit_history``
     - 6
     - Return the edit history of the node.
     - string
   * - ``--archive``
     - 7
     - Archive the node's children to disk.
     - ``StcCmd`` (OK)
   * - ``--restore``
     - 8
     - Restore previously archived children.
     - ``StcCmd`` (OK)

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --suspend /suite/family
            ecflow_client --check /suite/family

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-pathscmd-suspend">--suspend</option>
            <option value="ecf-h-pathscmd-resume">--resume</option>
            <option value="ecf-h-pathscmd-kill">--kill</option>
            <option value="ecf-h-pathscmd-status">--status</option>
            <option value="ecf-h-pathscmd-check">--check</option>
            <option value="ecf-h-pathscmd-edit-history">--edit_history</option>
            <option value="ecf-h-pathscmd-archive">--archive</option>
            <option value="ecf-h-pathscmd-restore">--restore</option>
            </select></label>
            <pre id="ecf-h-pathscmd-suspend" class="ecf-help-panel">suspend
            -------

            Suspend the given node. This prevents job generation for the given node, or any child node.
            Usage::
               --suspend=/s1/f1/t1   # suspend task s1/f1/t1
               --suspend=/s1 /s2     # suspend suites /s1 and /s2</pre>
            <pre id="ecf-h-pathscmd-resume" class="ecf-help-panel" hidden>resume
            ------

            Resume the given node. This allows job generation for the given node, or any child node.
            Usage::
               --resume=/s1/f1/t1   # resume task s1/f1/t1
               --resume=/s1 /s2     # resume suites /s1 and /s2</pre>
            <pre id="ecf-h-pathscmd-kill" class="ecf-help-panel" hidden>kill
            ----

            Kills the job associated with the node.
            If a family or suite is selected, will kill hierarchically.
            Kill uses the ECF_KILL_CMD variable. After variable substitution it is invoked as a command.
            The command should be written in such a way that the output is written to %ECF_JOB%.kill
            as this allow the --file command to report the output: .e.e.
             /home/ma/emos/bin/ecfkill %USER% %HOST% %ECF_RID% %ECF_JOB% &gt; %ECF_JOB%.kill 2&gt;&amp;1::
            Usage::
               --kill=/s1/f1/t1 /s1/f2/t2 # kill the jobs for tasks t1 and t2
               --file=/s1/f1/t1 kill      # write to standard out the &#x27;.kill&#x27; file for task /s1/f1/t1</pre>
            <pre id="ecf-h-pathscmd-status" class="ecf-help-panel" hidden>status
            ------

            Shows the status of a job associated with a task, in %ECF_JOB%.stat file
            If a family or suite is selected, will invoke status command hierarchically.
            Status uses the ECF_STATUS_CMD variable. After variable substitution it is invoked as a command.
            The command should be written in such a way that the output is written to %ECF_JOB%.stat
            This will allow the output of status command to be shown by the --file command
            i.e /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% &gt; %ECF_JOB%.stat 2&gt;&amp;1::
            If the process id cannot be found on the remote system, then the status command can also
            arrange for the task to be aborted
            The status command can fail for the following reasons:
             - ECF_STATUS_CMD not found
             - variable substitution fails
             - state is active but it can&#x27;t find process id, i.e. ECF_RID
             - the status command does not exit cleanly
            When this happens a flag is set, STATUSCMD_FAILED, which is visible in the GUI
            Usage::
               --status=/s1/f1/t1     # ECF_STATUS_CMD should output to %ECF_JOB%.stat
               --file=/s1/f1/t1 stat  # Return contents of %ECF_JOB%.stat file</pre>
            <pre id="ecf-h-pathscmd-check" class="ecf-help-panel" hidden>check
            -----

            Checks the expression and limits in the server. Will also check trigger references.
            Trigger expressions that reference paths that don&#x27;t exist, will be reported as errors.
            (Note: On the client side unresolved paths in trigger expressions must
            have an associated &#x27;extern&#x27; specified)
              arg = [ _all_ | / | list of node paths ]
            Usage:
              --check=_all_           # Checks all the suites
              --check=/               # Checks all the suites
              --check=/s1 /s2/f1/t1   # Check suite /s1 and task t1</pre>
            <pre id="ecf-h-pathscmd-edit-history" class="ecf-help-panel" hidden>edit_history
            ------------

            Returns the edit history associated with a Node.
            Can also be used to clear the edit history.
            Usage::
               --edit_history=/s1/f1/t1  # return history of changes for the given node
               --edit_history=clear      # clear/purge *ALL* edit history from all nodes.</pre>
            <pre id="ecf-h-pathscmd-archive" class="ecf-help-panel" hidden>archive
            -------

            Archives suite or family nodes *IF* they have child nodes(otherwise does nothing).
            Saves the suite/family nodes to disk, and then removes the child nodes from the definition.
            This saves memory in the server, when dealing with huge definitions that are not needed.
            It improves time taken to checkpoint and reduces network bandwidth.
            If the node is re-queued or begun, the child nodes are automatically restored.
            Use --restore to reload the archived nodes manually
            Care must be taken if you have trigger reference to the archived nodes
            The nodes are saved to the file ECF_HOME/&lt;hostname&gt;.&lt;port&gt;.&lt;ECF_NAME&gt;.check,
            where &#x27;/&#x27; has been replaced with &#x27;:&#x27; in ECF_NAME

            Nodes like (family and suites) can also to automatically archived by using,
            the &#x27;autoarchive&#x27; attribute. The attribute is only applied once the node is complete

            suite autoarchive
             family f1
                autoarchive +01:00 # archive one hour after complete
                task t1
             endfamily
             family f2
                 autoarchive 01:00 # archive at 1 am in morning after complete
                task t1
             endfamily
             family f3
                autoarchive 10     # archive 10 days after complete
                task t1
             endfamily
             family f4
                autoarchive 0      # archive immediately (upto 60s) after complete
                task t1
              endfamily
            endsuite

            Usage::
               --archive=/s1           # archive suite s1
               --archive=/s1/f1 /s2    # archive family /s1/f1 and suite /s2
               --archive=force /s1 /s2 # archive suites /s1,/s2 even if they have active tasks</pre>
            <pre id="ecf-h-pathscmd-restore" class="ecf-help-panel" hidden>restore
            -------

            Manually restore archived nodes.
            Restore will fail if:
              - Node has not been archived
              - Node has children, (since archived nodes have no children)
              - If the file ECF_HOME/&lt;host&gt;.&lt;port&gt;.&lt;ECF_NAME&gt;.check does not exist
            Nodes can be restored manually(as in this command) but also automatically

            Automatic restore is done using the &#x27;autorestore&#x27; attribute.
            Once the node containing the &#x27;autorestore&#x27; completes, the attribute is applied

             suite s
               family farchive_now
                 autoarchive 0      # archive immediately after complete
                 task tx
               endfamily
               family frestore_from_task
                 task t1
                    trigger ../farchive_now&lt;flag&gt;archived
                    autorestore ../farchive_now  # call autorestore when t1 completes
               endfamily
             endsuite

            In this example task &#x27;/s/frestore_from_task/t1&#x27; is only triggered if &#x27;farchive_now&#x27;
            is archived, then when t1 completes it will restore family &#x27;farchive_now&#x27;
            Usage::
               --restore=/s1/f1   # restore family /s1/f1
               --restore=/s1 /s2  # restore suites /s1 and /s2</pre>
            </div>

    .. tab:: Request

        ``--suspend /suite/family`` (``api_`` ``1``):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "PathsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 1,
                      "paths_": [ "/suite/family" ],
                      "force_": false
                    }
                  }
                }
              }
            }

        ``--check /suite/family`` differs only in ``api_`` being ``5``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "PathsCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "api_": 5,
                      "paths_": [ "/suite/family" ],
                      "force_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        ``--suspend``, ``--resume``, ``--kill``, ``--archive`` and ``--restore`` return the generic
        ``StcCmd`` reply with ``api_`` equal to ``0``:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

        ``--status``, ``--check`` and ``--edit_history`` return an ``SStringCmd`` whose ``str_`` field
        holds the text result:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "..."
                    }
                  }
                }
              }
            }

.. _proto_PlugCmd:

PlugCmd
~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/PlugCmd.hpp``
   * - Action
     - ``--plug``
   * - Parameters
     - - ``arg1`` (source): the node subtree to move (``source_``).
       - ``arg2`` (destination): the target parent, optionally on another server as
         ``//host:port/path`` (``dest_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Moves a node subtree to a new parent, possibly on a different server. Internally the server uses
``MoveCmd`` to transfer the subtree to the destination.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --plug /suite/family //host:3141/dest

    .. tab:: Help

        Verbatim ``ecflow_client --plug`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            plug
            ----

            Plug command is used to move nodes.
            The destination node can be on another server In which case the destination
            path should be of the form '<host>:<port>/suite/family/task
              arg1 = path to source node
              arg2 = path to the destination node
            This command can fail because:
            - Source node is in a 'active' or 'submitted' state
            - Another user already has an lock
            - source/destination paths do not exist on the corresponding servers
            - If the destination node path is empty, i.e. only host:port is specified,
              then the source node must correspond to a suite.
            - If the source node is added as a child, then its name must be unique
              amongst its peers
            Usage:
              --plug=/suite macX:3141  # move the suite to ecFlow server on host(macX) and port(3141)

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "PlugCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "source_": "/suite/family",
                      "dest_": "//host:3141/dest"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_QueryCmd:

QueryCmd
~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/QueryCmd.hpp``
   * - Action
     - ``--query``
   * - Parameters
     - - ``arg1`` (query type): one of ``state``, ``dstate``, ``event``, ``meter``, ``label``,
         ``variable``, ``trigger``, ``repeat``, ``limit`` or ``limit_max`` (``query_type_``).
       - ``arg2`` (path): the node, or ``<path>:<attribute>``, being queried (``path_to_attribute_``
         and ``attribute_``).
       - ``arg3`` (optional task path): the task the query is made on behalf of (``path_to_task_``),
         used for logging.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Reads a single piece of state from the server without changing anything — for example the state of a
node, or the value of an event, meter or variable. The reply is always a string. It is often used
inside scripts to branch on a node's state.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --query state /suite/family/task
            ecflow_client --query event /suite/family/task:myevent

    .. tab:: Help

        Verbatim ``ecflow_client --query`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            query
            -----

            Query the status of attributes
             i.e state,dstate,repeat,event,meter,label,variable or trigger expression without blocking
             - state     return [unknown | complete | queued |             aborted | submitted | active] to standard out
             - dstate    return [unknown | complete | queued | suspended | aborted | submitted | active] to standard out
             - repeat    returns current value as a string to standard out
             - event     return 'set' | 'clear' to standard out
             - meter     return value of the meter to standard out
             - limit     return current value of limit to standard out
             - limit_max return limit max value to standard out
             - label     return new value otherwise the old value
             - variable  return value of the variable, repeat or generated variable to standard out,
                         will search up the node tree. When path is '/', the variable is looked up on the
                         server itself, i.e. 'ecflow_client --query variable /:name'
             - trigger   returns 'true' if the expression is true, otherwise 'false'

            If this command is called within a '.ecf' script we will additionally log the task calling this command
            This is required to aid debugging for excessive use of this command
            The command will fail if the node path to the attribute does not exist in the definition and if:
             - repeat   The repeat is not found
             - event    The event is not found
             - meter    The meter is not found
             - limit/limit_max The limit is not found
             - label    The label is not found
             - variable No user or generated variable or repeat of that name found on node or its parents,
                        or (when path is '/') no user or server variable of that name found on the server
             - trigger  Trigger does not parse, or reference to nodes/attributes in the expression are not valid
            Arguments:
              arg1 = [ state | dstate | repeat | event | meter | label | variable | trigger | limit | limit_max ]
              arg2 = <path> | <path>:name where name is name of a event, meter, label, limit or variable.
                     path '/' represents the server itself, and can only be used with 'state' or 'variable'
              arg3 = trigger expression | prev | next # prev,next only used when arg1 is repeat

            Usage:
             ecflow_client --query state /                                     # return top level state to standard out
             ecflow_client --query state /path/to/node                         # return node state to standard out
             ecflow_client --query dstate /path/to/node                        # state that can included suspended
             ecflow_client --query repeat /path/to/node                        # return the current value as a string
             ecflow_client --query repeat /path/to/node prev                   # return the previous value as a string
             ecflow_client --query repeat /path/to/node next                   # return the next value as a string
             ecflow_client --query event /path/to/task/with/event:event_name   # return set | clear to standard out
             ecflow_client --query meter /path/to/task/with/meter:meter_name   # returns the current value of the meter to standard out
             ecflow_client --query limit /path/to/task/with/limit:limit_name   # returns the current value of the limit to standard out
             ecflow_client --query limit_max /path/to/task/with/limit:limit_name # returns the max value of the limit to standard out
             ecflow_client --query label /path/to/task/with/label:label_name   # returns the current value of the label to standard out
             ecflow_client --query variable /path/to/task/with/var:var_name    # returns the variable value to standard out
             ecflow_client --query variable /:var_name                         # returns the server variable value to standard out
             ecflow_client --query trigger /path/to/node/with/trigger "/suite/task == complete" # return true if expression evaluates false otherwise

    .. tab:: Request

        ``--query state /suite/family/task``:

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "QueryCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "query_type_": "state",
                      "path_to_attribute_": "/suite/family/task",
                      "attribute_": "",
                      "path_to_task_": "/suite/family/task"
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        An ``SStringCmd`` whose ``str_`` field holds the queried value:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "complete"
                    }
                  }
                }
              }
            }

.. _proto_ReplaceNodeCmd:

ReplaceNodeCmd
~~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ReplaceNodeCmd.hpp``
   * - Action
     - ``--replace``
   * - Parameters
     - - ``arg1`` (node path): the node to replace (``pathToNode_``).
       - ``arg2`` (definition file): the source definition (``path_to_defs_``); its serialized form
         is embedded in ``clientDefs_``.
       - ``parent`` (optional): create parent nodes as needed (``createNodesAsNeeded_``).
       - ``force`` (optional): replace even active or submitted nodes (``force_``).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Replaces a node (or adds it) using a node of the same path taken from another definition. Like
``LoadDefsCmd``, the source definition is embedded as a serialized string, here in ``clientDefs_``,
which can be large. The payload therefore has the following structure (``clientDefs_`` elided):

.. code-block:: json

   {
     "21ClientToServerRequest": {
       "cmd_": {
         "polymorphic_id": 2147483649,
         "polymorphic_name": "ReplaceNodeCmd",
         "ptr_wrapper": {
           "id": 2147483649,
           "data": {
             "cereal_class_version": 0,
             "value0": {
               "cereal_class_version": 0,
               "value0": { "cereal_class_version": 0, "cl_host_": "host.example.com" },
               "user_": "operator"
             },
             "createNodesAsNeeded_": true,
             "force_": false,
             "pathToNode_": "/suite/family",
             "path_to_defs_": "my_suite.def",
             "clientDefs_": "#5.18.0\nsuite suite\n  family family\n    task t1\n  endfamily\nendsuite\n# ..."
           }
         }
       }
     }
   }

The reply is the generic ``StcCmd`` success reply with ``api_`` equal to ``0``.

Verbatim ``ecflow_client --replace`` output (the common environment-variable footer is omitted):

.. code-block:: none

   replace
   -------

   Replaces a node in the server, with the given path
   Can also be used to add nodes in the server
     arg1 = path to node
            must exist in the client defs(arg2). This is also the node we want to
            replace in the server
     arg2 = path to client definition file
            provides the definition of the new node
     arg3 = (optional) [ parent | false ] (default = parent)
            create parent families or suite as needed, when arg1 does not
            exist in the server
     arg4 = (optional) force (default = false) 
            Force the replacement even if it causes zombies to be created
   Replace can fail if:
   - The node path(arg1) does not exist in the provided client definition(arg2)
   - The client definition(arg2) must be free of errors
   - If the third argument is not provided, then node path(arg1) must exist in the server
   - Nodes to be replaced are in active/submitted state, in which case arg4(force) can be used

   Replace will preserve the suspended status, if this is not required please re-queue first
   After replace is done, we check trigger expressions. These are reported to standard output.
   It is up to the user to correct invalid trigger expressions, otherwise the tasks will *not* run.
   Please note, you can use --check to check trigger expression and limits in the server.
   For more information use --help check.

   Usage:
     --replace=/suite/f1/t1 /tmp/client.def  parent      # Add/replace node tree /suite/f1/t1
     --replace=/suite/f1/t1 /tmp/client.def  false force # replace t1 even if its active or submitted

.. _proto_RequeueNodeCmd:

RequeueNodeCmd
~~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/RequeueNodeCmd.hpp``
   * - Action
     - ``--requeue``
   * - Parameters
     - - ``option`` (optional): ``abort`` requeues only aborted tasks; ``force`` requeues even
         active or submitted tasks. The default is no option (``option_`` ``0``).
       - ``paths`` (one or more): the nodes to requeue.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Requeues nodes so that they may run again. ``option_`` encodes the modifier (``0`` none, ``1``
``abort``, ``2`` ``force``).

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --requeue /suite/family
            ecflow_client --requeue abort /suite/family
            ecflow_client --requeue force /suite/family/task

    .. tab:: Help

        Verbatim ``ecflow_client --requeue`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            requeue
            -------

            Re queues the specified node(s)
              If any child of the specified node(s) is in a suspended state, this state is cleared
            Repeats are reset to their starting values, relative time attributes are reset.
              arg1 = (optional) [ abort | force ]
                     abort  = re-queue only aborted tasks below node
                     force  = Force the re-queueing even if there are nodes that are active or submitted
                     <null> = Checks if any tasks are in submitted or active states below the node
                              if so does nothing. Otherwise re-queues the node.
              arg2 = list of node paths. The node paths must begin with a leading '/' character

            Usage:
              --requeue=abort /suite/f1  # re-queue all aborted tasks of /suite/f1
              --requeue=force /suite/f1  # forcibly re-queue /suite/f1 and all its children.May cause zombies.
              --requeue=/s1/f1/t1 /s1/t2 # Re-queue node '/suite/f1/t1' and '/s1/t2'

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "RequeueNodeCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family" ],
                      "option_": 0
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_RunNodeCmd:

RunNodeCmd
~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/RunNodeCmd.hpp``
   * - Action
     - ``--run``
   * - Parameters
     - - ``paths`` (one or more): the tasks to run immediately.
       - ``force`` (optional): run even when a dependency would hold the task.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Runs a task immediately, bypassing its scheduling time. ``force_`` records whether dependencies are
ignored.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --run /suite/family/task
            ecflow_client --run force /suite/family/task

    .. tab:: Help

        Verbatim ``ecflow_client --run`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            run
            ---

            Ignore triggers, limits, time or date dependencies, just run the Task.
            When a job completes, it may be automatically re-queued if it has a cron
            or multiple time dependencies. If we have multiple time based attributes,
            then each run, will expire the time.
            When we run before the time, we want to avoid re-running the task then
            a flag is set, so that it is not automatically re-queued.
            A repeat attribute is incremented when all the child nodes are complete
            in this case the child nodes are automatically re-queued.
            Hence this command can be aid, in allowing a Repeat attribute to be incremented
              arg1 = (optional)force
                     Forcibly run, even if there are nodes that are active or submitted
                     This can result in zombie creation
              arg2 = node path(s). The paths must begin with a leading '/' character.
                     If the path is /suite/family will recursively run all tasks
                     When providing multiple paths avoid running the same task twice
            Example:
              --run=/suite/t1                    # run task t1
            Effect:
                 task t1; time 12:00             # will complete if run manually
                 task t2; time 10:00 13:00 01:00 # will run 4 times before completing
             When we have a time range(i.e as shown with task t2), then next time slot
             is incremented for each run, until it expires, and the task completes.
             Use the Why command, to show next run time (i.e. next time slot)

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "RunNodeCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "paths_": [ "/suite/family/task" ],
                      "force_": false
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_ServerVersionCmd:

ServerVersionCmd
~~~~~~~~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ServerVersionCmd.hpp``
   * - Action
     - ``--server_version``
   * - Parameters
     - - none.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Returns the version string of the running server. The command carries no fields of its own, so the
payload holds only the base-class ``cl_host_`` and ``user_``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --server_version

    .. tab:: Help

        Verbatim ``ecflow_client --server_version`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            server_version
            --------------

            Returns the version number of the server
            Usage:
              --server_version
                Writes the version to standard output

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "ServerVersionCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      }
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        An ``SStringCmd`` whose ``str_`` field holds the server version:

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "SStringCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "str_": "5.14.1"
                    }
                  }
                }
              }
            }

.. _proto_ShowCmd:

ShowCmd
~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command (client-side)
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ShowCmd.hpp``
   * - Action
     - ``--show``
   * - Parameters
     - - ``arg1`` (optional style): the print style to apply to the output (``defs``, ``state``,
         ``migrate``, and so on).
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Selects the print style used when the client renders a definition, and is normally combined with a
retrieval command such as ``--get`` in a group. The chosen style is applied on the client side and is
not part of the serialized payload, so the request carries only the base-class ``cl_host_`` and
``user_``.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --group="get=/suite; show=state"

    .. tab:: Help

        Verbatim ``ecflow_client --show`` output (the common environment-variable footer is
        omitted):

        .. code-block:: none

            show
            ----

            Used to print state of the definition returned from the server to standard output.
            This command can *only* be used in a group command, and will only work if it is
            preceded with a get command. See examples below.
               arg1 = [ defs | state | migrate ] 
            The output of show has several options: i.e
              o no arguments: With no arguments, print the definition structure to standard output
                Extern's are automatically added, allowing the output to be reloaded into the server
                i.e --group="get ; show"
              o state:
                This will output definition structure along with all the state information.
                This will include the trigger expressions, abstract syntax tree as comments.
                Excludes the edit history
              o migrate:
                This will output definition structure along with all the state information.
                The node state is shown in the comments.
                This format allows the definition to be migrated to future version of ecflow.
                The output includes edit history but excludes externs.
                When the definition is reloaded *NO* checking is done.

            The following shows a summary of the features associated with each choice
                                    DEFS          STATE      MIGRATE
            Auto generate externs   Yes           Yes        No
            Checking on reload      Yes           Yes        No
            Edit History            No            No         Yes
            trigger AST             No            Yes        No

            Usage:
                --group="get ; show"
                --group="get ; show defs"    # same as the previous example
                --group="get ; show state"   # Show all state for the node tree
                --group="get ; show migrate" # Shows state and allows migration
                --group="get=/s1; show"      # show state for the node only
                --group="get=/s1; show state"

    .. tab:: Request

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "ShowCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      }
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

.. _proto_ZombieCmd:

ZombieCmd
~~~~~~~~~

.. list-table::
   :stub-columns: 1
   :widths: 20 80

   * - Type
     - User command
   * - C++ class
     - ``.../libs/base/src/ecflow/base/cts/user/ZombieCmd.hpp``
   * - Action
     - one option per zombie resolution (see the *Actions* table below)
   * - Parameters
     - - ``paths`` (one or more): the zombies to act on.
       - ``process_id`` / ``password`` (internal): populated when the action originates from a task
         command rather than the command line.
   * - Environment variables
     - - ``ECF_USER`` (optional): overrides the authenticated user name.

Resolves zombies: mismatches between a running job and the server's record of a task. The action is
carried by ``user_action_``:

.. list-table::
   :header-rows: 1
   :widths: 26 14 60

   * - Option
     - ``user_action_``
     - Description
   * - ``--zombie_fob``
     - 0
     - Allow the task command to complete without changing state.
   * - ``--zombie_fail``
     - 1
     - Make the task command fail.
   * - ``--zombie_adopt``
     - 2
     - Adopt the zombie, applying its state change to the task.
   * - ``--zombie_remove``
     - 3
     - Remove the zombie from the server's list.
   * - ``--zombie_block``
     - 4
     - Block the task command until a decision is made.
   * - ``--zombie_kill``
     - 5
     - Run the kill command against the zombie's process.

.. tabs::

    .. tab:: Usage

        .. code-block:: shell

            ecflow_client --zombie_fob /suite/family/task
            ecflow_client --zombie_kill /suite/family/task

    .. tab:: Help

        Verbatim ``ecflow_client --help=<action>`` output (the common environment-variable
        footer shown on every page is omitted). Select an action:

        .. raw:: html

            <div class="ecf-help">
            <label>Action: <select onchange="ecfHelp(this)">
            <option value="ecf-h-zombiecmd-zombie-fob">--zombie_fob</option>
            <option value="ecf-h-zombiecmd-zombie-fail">--zombie_fail</option>
            <option value="ecf-h-zombiecmd-zombie-adopt">--zombie_adopt</option>
            <option value="ecf-h-zombiecmd-zombie-remove">--zombie_remove</option>
            <option value="ecf-h-zombiecmd-zombie-block">--zombie_block</option>
            <option value="ecf-h-zombiecmd-zombie-kill">--zombie_kill</option>
            </select></label>
            <pre id="ecf-h-zombiecmd-zombie-fob" class="ecf-help-panel">zombie_fob
            ----------

            Locates the task in the servers list of zombies, and sets to fob.
            This default behaviour of the child commands(label,event,meter) is to fob
            Next time the child commands (init,event,meter,label,abort,complete,wait,queue) communicate
            with the server, they will complete successfully (but without updating the node tree)
            allowing the job to finish.
            The references to the zombie in the server is automatically deleted after 1 hour
              args = list of task paths, at least one expected
              --zombie_fob=/path/to/task1 /path/to/task2</pre>
            <pre id="ecf-h-zombiecmd-zombie-fail" class="ecf-help-panel" hidden>zombie_fail
            -----------

            Locates the task in the servers list of zombies, and sets to fail.
            Next time a child command (init,event,meter,label,abort,complete) which matches zombie, communicates with the server, will be set to fail.
            Depending on the job setup this may force a abort, the abort will also fail.
            Hence job structure should use &#x27;set -e&#x27; in the error trapping functions to prevent
            infinite recursion.
            The references to the zombie in the server is automatically deleted after 1 hour
              args = list of task paths, at least one expected
              --zombie_fail=/path/to/task  /path/to/task2</pre>
            <pre id="ecf-h-zombiecmd-zombie-adopt" class="ecf-help-panel" hidden>zombie_adopt
            ------------

            Locates the task in the servers list of zombies, and sets to adopt.
            Next time a child command (init,event,meter,label,abort,complete,wait queue) communicates with the server, the password on the zombie is adopted by the task.
            This is only allowed if the process id matches, otherwise an error is issued.
            The zombie reference stored in the server is then deleted.
              args = list of task paths, at least one expected
              --zombie_adopt=/path/to/task  /path/to/task2</pre>
            <pre id="ecf-h-zombiecmd-zombie-remove" class="ecf-help-panel" hidden>zombie_remove
            -------------

            Locates the task in the servers list of zombies, and removes it.
            Since a job typically has many child commands (i.e init, complete, event, meter, label, wait, queue)
            the zombie may reappear
              args = list of task paths, at least one expected
              --zombie_remove=/path/to/task  /path/to/task2</pre>
            <pre id="ecf-h-zombiecmd-zombie-block" class="ecf-help-panel" hidden>zombie_block
            ------------

            Locates the task in the servers list of zombies, and sets flags to block it.
            This is default behaviour of the child commands(init,abort,complete,wait,queue)
            when the server cannot match the passwords. Each child commands will continue
            attempting to connect to the server for 24 hours, and will then return an error.
            The connection timeout can be configured with environment ECF_TIMEOUT
              args = list of task paths, at least one expected
              --zombie_block=/path/to/task  /path/to/task2</pre>
            <pre id="ecf-h-zombiecmd-zombie-kill" class="ecf-help-panel" hidden>zombie_kill
            -----------

            Locates the task in the servers list of zombies, and sets flags to kill
            The kill is done using ECF_KILL_CMD, but using the process_id from the zombie
            The job is allowed to continue until the kill is received
            Can only kill zombies that have an associated Task, hence path zombies
            must be killed manually.
              arg = list of task paths, at least one expected
              --zombie_kill=/path/to/task  /path/to/task2</pre>
            </div>

    .. tab:: Request

        ``--zombie_fob /suite/family`` (``user_action_`` ``0``; other actions differ only in that
        value):

        .. code-block:: json

            {
              "21ClientToServerRequest": {
                "cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "ZombieCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": {
                        "cereal_class_version": 0,
                        "value0": {
                          "cereal_class_version": 0,
                          "cl_host_": "host.example.com"
                        },
                        "user_": "operator"
                      },
                      "user_action_": 0,
                      "process_id_": "",
                      "password_": "",
                      "paths_": [ "/suite/family" ]
                    }
                  }
                }
              }
            }

    .. tab:: Reply

        The generic success reply (``StcCmd`` with ``api_`` equal to ``0``):

        .. code-block:: json

            {
              "22ServerToClientResponse": {
                "stc_cmd_": {
                  "polymorphic_id": 2147483649,
                  "polymorphic_name": "StcCmd",
                  "ptr_wrapper": {
                    "id": 2147483649,
                    "data": {
                      "cereal_class_version": 0,
                      "value0": { "cereal_class_version": 0 },
                      "api_": 0
                    }
                  }
                }
              }
            }

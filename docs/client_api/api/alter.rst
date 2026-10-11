.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0


.. _alter_cli:

alter
*****


.. rubric:: Description




The CLI command :code:`alter` enables the manipulation of node attributes in
an ecFlow suite, namely it allows to:

 - add new node attributes
 - remove existing node attributes
 - update some characteristics of existing node attributes
 - set/clear node flags
 - sort node attributes

The arguments to the :code:`alter` command are specified as a sequence of space
separated options:

.. code-block:: shell

    ecflow_client --alter <operation> <type> <name> <value> <path>

where

 - :code:`<operation>`, determines what operation is performed (one of :code:`delete`, :code:`change`, :code:`add`, :code:`set_flag`, :code:`clear_flag`, or :code:`sort`)
 - :code:`<type>`, specifies which type of attribute is being manipulated (the list of types possible depends on the type of command selected previously)
 - :code:`<name>`, is the name of the attribute being manipulated (in the case where the attribute is unnamed, e.g. Repeat, Complete, Trigger, this option should be omitted)
 - :code:`<value>`, is the value to be used in the operation (this option is not required for all commands, e.g. when deleting an attribute)
 - :code:`<path> (<path> (...))`, one or more paths to the nodes where the operation is to be performed

See more details about each argument in the next section.

.. _alter_cli_change_repeat:

Changing a Repeat
-----------------

A node holds at most one Repeat, so the Repeat is identified by the node
path alone, and no name is given (the same applies to the Trigger and the
Complete expressions). The value is either a plain value, which becomes
the current value of the Repeat, or a list of options, applied together:

-  ``--begin <value>``, the first value of the Repeat;

-  ``--end <value>``, the last value of the Repeat;

-  ``--step <value>``, the increment;

-  ``--current <value>``, the current value.

The value is a single argument: a list of options must be quoted on the
command line, so that it reaches the server as one value.

.. code-block:: shell

    # change the current value
    ecflow_client --alter change repeat 20260115 /suite/family

    # change the range, the current value being kept within it
    ecflow_client --alter change repeat "--begin 20260101 --end 20261231" /suite/family

    # change the range and the current value together
    ecflow_client --alter change repeat "--begin 20260101 --end 20261231 --current 20260115" /suite/family

    # change the step
    ecflow_client --alter change repeat "--step 7" /suite/family

The options accepted, and the format of their values, depend on the kind
of Repeat:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Repeat
     - Options and values
   * - ``date``
     - ``--begin``, ``--end`` and ``--current`` as ``yyyymmdd``;
       ``--step`` as a number of days
   * - ``datetime``
     - ``--begin``, ``--end`` and ``--current`` as ``yyyymmddTHHMMSS``;
       ``--step`` as a duration ``H[:MM[:SS]]``
   * - ``integer``
     - all options as integers
   * - ``datelist``, ``datetimelist``
     - only ``--current``, with a member of the list
   * - ``enumerated``, ``string``
     - only ``--current``, with a member of the list or its index; a
       value equal to a member selects that member, even when it reads as
       an option or an index
   * - ``day``
     - only ``--step``, as a number of days; a plain value is ignored,
       since a ``repeat day`` has no current value

An option that does not apply to the kind of Repeat is refused, as is a
change that leaves the Repeat invalid, for example an end before the
begin, or a step of zero; a refused change leaves the Repeat unchanged.
When several paths are given, each Repeat is changed on its own: the
change applies to every Repeat that accepts it, and an error names each
Repeat that refused it.

When ``--current`` is not given, the current value is kept within the new
range and on the new step grid:

-  a current value outside the new range is moved to the nearest limit of
   the range;

-  a current value off the new step grid is moved to the previous value
   on the grid, towards the begin;

-  a completed Repeat (whose current value lies past its end) stays
   completed, unless the new range continues it, as when the end of the
   Repeat is extended.

When ``--current`` is given, its value must lie within the new range and
on the new step grid; it is never moved.

Changing a Repeat does not change the state of its node. A running node
whose current value is moved keeps running, and its completion moves the
Repeat to the next value: the moved value itself is not run. To run it,
set it with ``--current`` once the node is no longer running.

.. warning::

   **The options require an ecFlow server of version 5.20 or later.** An
   older server refuses a list of options as an invalid value, leaving
   the Repeat unchanged, except for a ``repeat day``, on which the value
   is silently ignored. A plain value is accepted by every version.



.. rubric:: Output of :code:`--help=alter`



The following help text is generated by :code:`ecflow_client --help=alter`

::


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

    * When updating unnamed attributes (Repeat, Trigger, Complete, ...) the name/arg3 is not necessary,
      since a node holds at most one attribute of each of these kinds.

    * After changing the clock the suite needs to be re-queued for the change to take effect.

    * When adding or updating node attributes (e.g. variable, meter, event, label, limits, late)
      the name (arg3) and value (arg4) must be quoted.

    * When sorting attributes, 'recursive' can be used as the value (arg3).

    * When adding a meter, the value (arg4) is expected to be a comma-separated triplet
      of numerical values the form "<min>,<max>,<value>" (n.b. no spaces are allowed).

    * When adding an event, the non-optional value (arg4) must be either "set" or "clear".

    * When adding or updating aviso and mirror attributes, the value (arg4) is expected to be
      a quoted list of configuration options. For example:
      * for aviso, "--listener '{ \"event\": \"mars\", \"request\": { \"class\": \"od\" } }'
                    --url https://aviso.ecmwf.int --auth /path/to/auth.json --collapse"
      * for mirror, "--remote_path /s1/f1/t2 --remote_host host --polling 20 --remote_port 3141 --ssl"

    * For both aviso and mirror, the special value "reload" forces reloading the configuration.
      This is typically useful after updating variables used to configure these kind of attributes.

    * Updating an aviso attribute replaces it, starting from the revision given by --revision (0 when
      omitted, i.e. only the notifications published after the update are considered). To continue
      after the last notification consumed, give the current revision of the attribute with --revision.

    * When changing limit_value, the value (arg4) is expected to be an integer, or the special
      value "reset". The special value "reset" re-synchronises the limit with the nodes that are
      currently consuming it, i.e. the value and the consumed paths of the limit are recomputed
      from the nodes that are currently submitted or active. This is the recommended way to
      recover a limit whose value no longer reflects the nodes it is limiting. Note that the
      recomputed value is allowed to exceed the maximum value of the limit.

    * When changing a repeat, the value (arg4) is either:
        - a plain value, which becomes the current value; or
        - a quoted list of options, applied together:

            --begin <v>     the first value
            --end <v>       the last value
            --step <v>      the increment
            --current <v>   the current value

      The options accepted depend on the kind of repeat:

        - date:           --begin, --end, --current as yyyymmdd; --step in days
        - datetime:       --begin, --end, --current as yyyymmddTHHMMSS; --step as H[:MM[:SS]]
        - integer:        all options as integers
        - datelist,
          datetimelist:   only --current, with a member of the list
        - enumerated,
          string:         only --current, with a member of the list, or its index
        - day:            only --step, in days (a plain value is ignored)

      Notes:

        - An invalid change (e.g. an end before the begin, a zero step) is refused,
          and the repeat is left unchanged.
        - Without --current, the current value is kept within the new range,
          and on the new step grid, moving towards the begin.
        - A completed repeat stays completed, unless the new range continues it.
        - A running node keeps running; its completion moves to the next value,
          so the moved value itself is not run.
        - The options require a server of version 5.20 or later.

   Usage:

       --alter=add variable <variable-name> "value" /             # add server variable
       --alter=add variable <variable-name> "value" /path/to/node # add node variable

       --alter=add time "+00:20" /path/to/node

       --alter=add date "01.*.*" /path/to/node

       --alter=add day "sunday"  /path/to/node

       --alter=add label <label-name> "label_value" /path/to/node

       --alter=add event <event-name> "set"|"clear" /path/to/node

       --alter=add meter <meter-name> "<min>,<max>,value" /path/to/node

       --alter=add late "-s 00:01 -a 14:30 -c +00:01" /path/to/node

       --alter=add limit mars "100" /path/to/node

       --alter=add inlimit /path/to/node/withlimit:limit_name "10" /path/to/node

       --alter=add inlimit /path/to/node/withlimit:limit_name "-s 10" /path/to/node

       --alter=add inlimit /path/to/node/withlimit:limit_name "-n 10" /path/to/node

       # zombie attributes have the following structure:
         `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`
          zombie_type        = "user" | "ecf" | "path" | "ecf_pid" | "ecf_passwd" | "ecf_pid_passwd"
          client_side_action = "fob" | "fail" | "block"
          server_side_action = "adopt" | "delete" | "kill"
          child              = "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete" | "queue"
          zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60

       --alter=add zombie "ecf:fail::" /path/to/node     # ask system zombies to fail
       --alter=add zombie "user:fail::" /path/to/node    # ask user generated zombies to fail
       --alter=add zombie "path:fail::" /path/to/node    # ask path zombies to fail

       --alter=change repeat "20260115" /path/to/node                    # current value only

       --alter=change repeat "--begin 20260101 --end 20261231" /path/to/node

       --alter=change repeat "--begin 20260101 --end 20261231 --current 20260115" /path/to/node

       --alter=change repeat "--step 7" /path/to/node

       --alter=change limit_max <limit-name> "10" /path/to/node

       --alter=change limit_value <limit-name> "5" /path/to/node

       # re-synchronise the limit with the nodes currently consuming it
       --alter=change limit_value <limit-name> "reset" /path/to/node

       --alter=delete variable FRED /path/to/node  # delete variable FRED
       --alter=delete variable      /path/to/node  # delete *ALL* variables on the given snode

   The client considers, for both user and task commands, the following environment variables:

     ECF_HOST <string> [mandatory*]
       The main server hostname
       Default: localhost
     ECF_PORT <int> [mandatory*]
       The main server port
       Default: 3141
     ECF_SSL <any> [optional*]
       Enable secure communication. Empty or '1': server.crt first; other values: <host>.<port>.crt first.
     ECF_SSL_DIR <string> [optional]
       Directory of the SSL certificates, with or without a trailing '/'. Does not enable SSL.
       Default: $HOME/.ecflowrc/ssl/
     ECF_HOSTFILE <string> [optional]
       File that lists alternate hosts to try, if connection to main host fails
     ECF_HOSTFILE_POLICY <string> [optional]
       The policy ('task' or 'all') to define which commands consider using alternate hosts.
       Default: task
     ECF_TIMEOUT <int> [optional]
       Maximum time for a task command to deliver a message to the server,
       retrying the main and alternate hosts until the limit is reached.
       Also applies to user commands, when ECF_HOSTFILE_POLICY is 'all'.
       Default: 3600 seconds; accepted range: 60 to 86400 seconds
     ECF_CONNECT_TIMEOUT <int> [optional]
       Maximum time for each step of an exchange with the server: connecting,
       sending the request, and receiving the reply (for HTTP/HTTPS, each wait
       for data). The value 0 applies the timeout of the command being executed,
       for example:
         - 10 seconds for --ping
         - 60 seconds for most user commands
         - 190 seconds for task commands
       Default: 0 seconds

   The options marked with (*) must be specified in order for the client to communicate
   with the server, either by setting the environment variables or by specifying the
   command line options.



.. seealso::

   :ref:`Protocol reference for AlterCmd <proto_AlterCmd>` — the request/reply
   payloads exchanged with the server and the C++ command class that implements this command.

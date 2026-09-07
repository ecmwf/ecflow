.. _definition_file_format:

Definition file format
//////////////////////

The following is a technical specification of the plain-text suite definition format.

This specification was derived directly from the current source code, and it covers both the pure definition file
(``.def``) and the runtime-state overlay used for checkpoint files (``.check``).

.. _ch-overview:

Overview
========

A *definition file* is the plain-text representation of an ecflow suite definition tree, containing one or more
``suite`` blocks, each holding nested ``family`` and ``task`` nodes decorated with attributes such as ``trigger``,
``time``, ``edit``, or ``limit``.

Because it contains suites, the extended technical name *suite definition file* is also used, and the file is
colloquially referred to as *defs*. A definition file typically uses the extension ``.def``.

Output styles
-------------

.. implementation::

    A definition file is represented by a ``Defs`` object.

    The ``Defs`` object is written by ``DefsWriter.hpp`` and read back by the per-keyword ``DefsParser.hpp``
    recursive-descent parsers.

    The writer supports four output styles, selected by the ``PrintStyle::Type_t`` enum.

The following four output styles share the same underlying grammar, differing only in whether the *state* information
is included in the output alongside the *structure* information.

The ``DEFS`` style is used for definition files (i.e. pure structural format), while ``STATE``, ``MIGRATE``, and ``NET``
include state information in various degrees of strictness. The ``MIGRATE`` style is used for checkpoint files.

.. list-table:: PrintStyle output styles
   :header-rows: 1
   :widths: 12 38 50

   * - Style
     - Contains
     - Typical use
   * - ``DEFS``
     - Structure only — no state
     - Authored ``.def`` files (manually or otherwise)
   * - ``STATE``
     - Structure + State — strict check on load
     - ``ecflow_client --show state`` for inspection
   * - ``MIGRATE``
     - Structure + State — fault-tolerant
     - Server checkpoint files (``.check`` / ``.checkpt``)
   * - ``NET``
     - Structure + State — wire-transfer format
     - Server ↔ client wire transfer

A *checkpoint file* typically uses extension ``.check`` or ``.checkpt``, and is in practice a definition file generated
with ``MIGRATE``-style output format (i.e. combines both structure and state information).

.. _ch-scope:

Scope and Conformance
=====================

This specification documents both layers of the ecflow definition-file grammar:

 - the **structural information**, the format a user hand-authors and loads using ``ecflow_client --load forecast.def``

 - the **state information**, added to the previous structural information, covered in :ref:`ch-runtime-state`.

.. important::

   This specification does **not** cover the following aspects:

    - ``trigger``/``complete`` AST operator grammar
    - ``aviso``/``mirror`` JSON listener payload schema
    - cereal-based binary serialisation

.. _ch-lexical:

Lexical Conventions
===================

Line orientation
----------------

The format is fundamentally line-oriented, with the parser reading one physical line at a time and
splitting it into whitespace-separated tokens.

A semicolon (``;``) may be used to place multiple statements on one line — ``task a; task b; endfamily``.
The parser will split on ``;`` before tokenizing, with care taken not to split inside quoted values
(so a ``;`` inside an ``edit`` or ``label`` value is safe).

.. note::

   The use of multiple statements on one line is **not** recommended.

   It is not guaranteed to work in all cases.
   In practice there are known issues (e.g. handling values with mixed/embedded semicolons and quotes);
   it is only supported for backward compatibility with legacy files, and should be avoided in new files.


Whitespace and Indentation
--------------------------

The general approach is that whitespace between tokens is only a separator, with consecutive spaces or tabs
being equivalent to a single space. However, white space is retained in some elements (e.g. variable, label)
that have values which might contain spaces, and in this case, the quoted value is considered as *the token*.

While the writer indents nested nodes per nesting level purely for human readability, the parser does not
require any indentation and does not consider column position at all.

Comments
--------

A ``#`` character marks the beginning of a comment that runs to the end of the physical line.
Comments may appear on their own line or trail any structural line, and are discarded by the parser.

If the ``#`` character appears inside a quoted value, it is treated as part of the value and not a comment.

.. code-block:: shell

    # this whole line is a comment
    task t1   # trailing comment, discarded by the parser
    label foo "this is not a comment # as it is part of the value"

Identifiers
-----------

Node names (suite, family, task, alias) and attribute names must follow the same pattern:

- non-empty sequence of characters
- a leading letter, digit, or underscore
- followed by zero or more letters, digits, underscores, or dots

The regex pattern is ``\w(\w|\.)*``.

Quoting and value escaping
--------------------------

Values that may contain spaces are quoted with single or double quotes, depending on the
attribute (``edit`` uses ``'...'``; ``label`` uses ``"..."``).

Because the format is strictly line-oriented, a literal newline embedded in an ``edit`` or ``label``
value is never written raw — it is escaped to the two-character sequence ``\n`` before quoting,
and un-escaped again on read. This guarantees exactly one physical line per attribute, which the parser requires.

Node Path
---------

Nodes can be referenced by their path, which follows the \*nix convention: a slash-separated
sequence of node names from the definition root to the target node.

Many attributes (triggers, ``inlimit``, ``autorestore``) reference other nodes by path:
``/suite/family/task`` (absolute), ``./sibling`` (relative to the current node's parent), or
``../uncle`` (relative, one level up).

Time/Date Values
----------------

Several attributes reuse the same primitive time/date formats:

  - **hh:mm**

    Zero-padded 24-hour clock, e.g. ``09:05``, ``23:59``. A leading ``+`` marks the value as
    *relative* (to node activation / suite begin) rather than an absolute time of day.

  - **dd.mm.yyyy**

    Day, month, and year separated by dots; any field may be ``*`` to mean "any value" (only in
    ``date``, not in ``clock``).

.. _ch-file-structure:

File Structure
==============

A definition file has exactly this outer shape, in this order:

.. code-block:: shell

    #X.YY.Z                          # version comment — informational only, see below

    extern /some/path[:attr-name]    # zero or more extern declarations
                                     # n.b. `extern`s are defined at the root/top level only

    suite name-a
      ...
    endsuite

    suite name-b
      ...
    endsuite

    # enddef                         # trailing footer comment, always written

Version comment
---------------

The very first line in a definition file always starts with ``#``, immediately followed by the
ecflow version string (e.g. ``#5.18.0``).

This line, because it starts with a ``#``, is technically a comment.
It exists purely so a human user can identify which ecflow release wrote the file.

External references block
-------------------------

An extern (``extern <path>[:<attr-name>]``) declares that a trigger elsewhere in the file may
reference a node or attribute that is *not* defined in this file. The ``extern`` entry
suppresses "unresolved reference" errors.

This block is composed of zero or more ``extern`` entries, one per external reference, appearing
at the definition root (before the first suite).

Externs are written for ``DEFS`` and ``STATE`` style, but deliberately *not* for ``MIGRATE``/``NET``
(checkpoint/wire transfer). See :ref:`attr-extern` in the Attribute Reference.

Suites block
------------

Each suite is enclosed in a ``suite ... endsuite`` block, and one or more suites may appear in the
order they should be evaluated for display purposes. See :ref:`ch-node-hierarchy`.

Trailing footer
---------------

An ``# enddef`` footer is written unconditionally as the last line. Like the version header, it
is a comment with no effect on parsing — a visual "end of file" marker.

.. note::

   What is deliberately **absent** from ``DEFS``-style output: a ``defs_state`` header line, and any
   server-wide ``edit`` variables. Both exist only in ``STATE``/``MIGRATE``/``NET`` output — see
   :ref:`ch-scope`.

.. _ch-node-hierarchy:

Node Hierarchy
==============

Every attribute in the :ref:`ch-attributes` below attaches to one of four node types. A fifth,
``Alias``, exists but is never emitted in ``DEFS`` style.

``Defs`` acts as a meta-container for the whole file, and is the only place where ``extern``
declarations may appear. A ``Suite`` is a top-level container that may hold any number of
``Family`` or ``Task`` children.

The node tree hierarchy is strictly: ``Defs`` → ``Suite`` → ``Family`` (nestable) → ``Task``.

.. _node-defs:

Definition — the root, implicit
-------------------------------

The *Definition* represents the whole file. Think of it as the root node that holds zero-or-more ``extern``
declarations and zero-or-more suites. It has no explicit opening/closing keyword — it *is* everything in the file.

When generating ``STATE``/``MIGRATE``/``NET`` output, it also carries a :ref:`state-header` header, server variables,
and optionally ``history`` lines, all before any suites.

.. _node-suite:

Suite — the top-level container
-------------------------------

.. code-block:: shell

    suite name
      ...
    endsuite

Suites cannot nest inside one another.

A suite may contain any of the common ``Node`` attributes, plus two suite-only attributes:
``clock`` and ``endclock`` (see :ref:`attr-clock`).

Children may be ``family`` or ``task`` blocks, in any mixture and order.

When generating ``STATE``/``MIGRATE``/``NET`` output, the header line may carry a trailing
``begun:1`` plus the common :ref:`state-node`, and a :ref:`state-calendar` line follows the clock.

.. note::

   A suite does *not* accept ``time``, ``today``, ``date``, or ``day`` directly.
   These time-dependencies are only legal on family/task/alias.

.. _node-family:

Family — a nestable container
-----------------------------

.. code-block:: shell

    family name
      ...
    endfamily

Families nest freely inside suites or other families.

Children may be further ``family`` or ``task`` blocks.

An ``endfamily`` token is written by the writer and required to close the block unambiguously.

.. _node-task:

Task — a leaf node
------------------

.. code-block:: shell

    task name
      ...

Tasks are leaves in the ``Node`` hierarchy.

Tasks hold attributes, but no child nodes (with the exception of aliases).

.. note::

   The writer **never emits** ``endtask`` in any style, even though the parser accepts an optional
   ``endtask`` for symmetry/readability if a human writes one by hand.

   A task's extent is simply "until the next sibling ``task``/``family`` or an enclosing
   ``endfamily``/``endsuite``".

When generating ``STATE``/``MIGRATE``/``NET`` output the header line may carry ``try:``, ``passwd:``, ``rid:``,
an abort reason, and the common :ref:`state-node`.

A task may also be followed by :ref:`state-alias` blocks.

.. _node-alias:

Alias — an ephemeral clone
--------------------------

An alias is a child of a task, created by cloning the parent task and its attributes.

Aliases have a distinct identity (i.e. different name, unique path) and are very useful
for debugging purposes as they allow one to easily create multiple independent instances of an existing task.

Aliases never appear in ``DEFS``-style output, but they are included when generating
``STATE``/``MIGRATE``/``NET`` output, closed by ``endalias`` when they do appear.
Full syntax is in :ref:`state-alias`.

Attribute attachment matrix
---------------------------

The following table shows where each keyword may legally appear.
"Node (Suite, Family, Task, and Alias)" = suite, family, task, and alias alike.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Attaches to
     - Keywords
   * - Defs
     - - ``extern``
   * - Suite
     - - ``clock``
       - ``endclock``
   * - Family / Task / Alias
     - - ``time``
       - ``today``
       - ``date``
       - ``day``
   * - Node

       (Suite, Family, Task, and Alias)
     - - ``edit``
       - ``trigger``
       - ``complete``
       - ``repeat``
       - ``limit``
       - ``inlimit``
       - ``label``
       - ``meter``
       - ``event``
       - ``cron``
       - ``aviso``
       - ``mirror``
       - ``defstatus``
       - ``late``
       - ``autocancel``
       - ``autoarchive``
       - ``autorestore``
       - ``zombie``
       - ``verify``
       - ``queue``
       - ``generic``

Canonical attribute emission order
----------------------------------

.. implementation::

   The writer emits attributes in a fixed order, regardless of the order they were read in.

   While the parser acts purely on each line's leading keyword, so it accepts the attributes in
   *any* order, the writer always emits them in one fixed sequence per node.

   This can be useful to know when comparing a hand-authored file against what a server would print back.

   .. code-block:: text

         > defstatus
         > late
         > complete
         > trigger
         > repeat
         > edit
         > limit
         > inlimit
         > label
         > meter
         > event
         > time
         > today
         > date
         > day
         > cron
         > aviso
         > mirror
         > autocancel
         > autoarchive
         > autorestore
         > zombie
         > verify
         > queue
         > generic

   For a ``suite`` specifically, all of the above (plus any suite-level ``edit``/``limit``/etc.) are
   written *before* ``clock``/``endclock`` and the calendar line, which in turn come before any
   nested ``family``/``task`` blocks — see :ref:`ch-runtime-state` for the calendar line itself.

.. _ch-attributes:

Attribute Reference
===================

The following entries are ordered alphabetically, for ease of random-access lookup.

.. implementation::

   All examples in this section were extracted from ecflow's own parser test fixtures
   (``libs/node/test/parser/data/good_defs/<keyword>/``) or derived from the writer source itself.

.. _attr-autoarchive:

autoarchive
-----------

Schedules a node to be automatically archived (i.e. moves its in-memory subtree to an on-disk
``.archive`` file) a given time after it completes.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    autoarchive (days | [+]hh:mm) [-i]

**Parameters**

   days
      Unsigned integer — whole days after completion.

   +hh:mm
      Relative time-of-day after completion.

   hh:mm
      (no leading ``+``) — absolute time-of-day.

   -i
      Idle flag: also archive if the node ends up queued/aborted, not only complete.

**Semantics**

Shares its time grammar with :ref:`attr-autocancel`, but archiving is recoverable via
:ref:`attr-autorestore`, whereas cancelling is not.

**Examples**

.. code-block:: shell

    autoarchive +00:02
    autoarchive 01:00     # archive at 1am after complete
    autoarchive 1        # archive 1 day after complete
    autoarchive 0 -i     # archive immediately, even if queued/aborted

.. _attr-autocancel:

autocancel
----------

Schedules a node for permanent removal a given time after it completes.
Unlike ``autoarchive``, there is no way to bring it back.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    autocancel (days | [+]hh:mm)

**Parameters**

   days
      Unsigned integer — whole days after completion.

   +hh:mm
      Relative time-of-day after completion.

   hh:mm
      Absolute time-of-day.

**Examples**

.. code-block:: shell

    autocancel +01:00   # cancel one hour after complete
    autocancel 01:00    # cancel at 1am after complete
    autocancel 10       # cancel 10 days after complete

.. _attr-autorestore:

autorestore
-----------

Lists one or more previously auto-archived node paths to restore once this node runs.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    autorestore path-1 [path-2 ...]

**Parameters**

   path-N
      Absolute or relative node path; must reference a suite or family that was previously archived.

**Semantics**

Typically paired with a trigger on the node's ``<flag>archived`` pseudo-attribute.

**Examples**

.. code-block:: shell

    family frestore_from_family_now
      trigger ./farchive_now<flag>archived
      autorestore ./farchive_now
      task t1
        edit SLEEP 60

.. _attr-aviso:

aviso
-----

Subscribes to an external Aviso notification as a trigger — the node is queued when a
matching notification is received.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    aviso --name name --listener 'json'
      [--url url] [--schema schema] [--polling seconds]
      [--revision n] [--auth auth] [--reason reason]

**Parameters**

   --name
      Identifier, unique among this node's aviso attributes.

   --listener
      Single-quoted opaque JSON payload; the value may embed ``%VARIABLE%`` placeholders.
      The JSON schema is the Aviso event-listener specification, and is not included here.

   --url
      Aviso service URL. Default: ``%ECF_AVISO_URL%``.

   --schema
      Path to the listener JSON schema. Default: ``%ECF_AVISO_SCHEMA%``.

   --polling
      Polling interval in seconds. Default: ``%ECF_AVISO_POLLING%``.

   --revision
      Last processed revision marker (unsigned integer). Default: ``0``.

   --auth
      Path to an auth token file. Default: ``%ECF_AVISO_AUTH%``.

   --reason
      Informational; last recorded failure reason.

**Examples**

.. code-block:: shell

    aviso --name A --listener '{ "event": "mars", "request": { "class": "od", "expver": "0001", "domain": "g", "stream": "enfo", "step": [0, 6, 12, 18] } }' --url %ECF_AVISO_URL% --schema %ECF_AVISO_SCHEMA% --auth %ECF_AVISO_AUTH% --polling %ECF_AVISO_POLLING%

**Notes**

See also :ref:`attr-mirror` for polling a remote ecflow node's status instead of an external
event feed.

.. _attr-clock:

clock (and endclock)
--------------------

Initialises the suite's calendar — real vs. hybrid clock, optional start date, optional gain (an
offset applied on every calendar update).

**Attaches to:** Suite only

**Syntax**

.. code-block:: shell

    clock (real|hybrid) [dd.mm.yyyy] [[+]hh:mm | seconds] [-s]
    endclock [dd.mm.yyyy] [[+]hh:mm | seconds]

**Parameters**

   real | hybrid
      ``real`` tracks wall-clock time continuously; ``hybrid`` only advances the date when the suite
      begins/re-begins (used for suites without an always-on server).

   dd.mm.yyyy
      Optional start date (no ``*`` wildcards here, unlike ``date``). Omitted means "use today".

   [+]hh:mm | seconds
      Gain: an offset applied to the calendar, as a time-of-day (``+`` = positive gain) or as a raw
      signed integer count of seconds.

   -s (clock only)
      Lets the clock stop and start together with the server, so time-dependencies are honoured
      across server restarts (simulator/testing feature).

**Semantics**

``endclock`` sets a simulator-only "stop" bound on the calendar and is written immediately after
``clock`` when present; it never appears without a preceding ``clock``.

When generating ``STATE``/``MIGRATE``/``NET`` output, a :ref:`state-calendar` line follows immediately after,
with the live snapshot this clock initialised.

**Examples**

.. code-block:: shell

    clock real
    clock hybrid
    clock real 20.1.2007 +01:00
    endclock   21.1.2007

.. _attr-complete:

complete
--------

A boolean expression that, once true, forces the node to be treated as complete regardless of its
own job outcome. Shares its expression grammar entirely with :ref:`attr-trigger`.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    complete expression
    complete -a expression   # AND-continuation: a second "complete" line
    complete -o expression   # OR-continuation

**Parameters**

   expression
      Free-form boolean expression text (see :ref:`ch-grammar`). May end the physical line with a
      trailing backslash ``\`` to continue onto the next line.

   -a / -o
      Placed immediately after the keyword on a *second* (or later) ``complete`` line attached to the
      same node, logically AND/OR-ing it with the expression(s) already given. This is distinct from
      writing ``and``/``or`` as plain words *inside* a single expression string — both forms are
      accepted.

**Examples**

.. code-block:: shell

    complete /o/main/12/fc eq complete or /o/main/12/fc/model:step gt 42
    complete /ealadin:REAL_TIME eq 0

**Notes**

When generating ``STATE`` style only, a currently-free expression additionally gets a trailing ``# (free)``
marker plus a full AST evaluation-tree dump — see :ref:`state-attrs`.

.. _attr-cron:

cron
----

A repeating time series, optionally filtered by weekday, day-of-month, and/or month, that
re-fires indefinitely (unlike ``time``, a cron never "runs out").

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    cron [-w d[,d...][,nL]] [-d d[,d...][,L]] [-m m[,m...]]
         hh:mm [hh:mm hh:mm]

**Parameters**

   -w
      Comma-separated weekday numbers, ``0``\ =Sunday … ``6``\ =Saturday. A trailing ``L`` on a value
      (e.g. ``1L``) means "the last such weekday of the month".

   -d
      Comma-separated days-of-month (``1``–``31``); the literal ``L`` means "last day of the month".

   -m
      Comma-separated months, ``1``\ =January … ``12``\ =December.

   hh:mm [hh:mm hh:mm]
      Either a single time-of-day, or a start/end/increment time series (same series grammar as
      :ref:`attr-time`).

**Examples**

.. code-block:: shell

    cron 00:25 23:59 00:30
    cron -w 0, 1, 2 -m 5, 6, 7, 8, 9, 10  10:00 20:00 01:00  # every Sun, May-Oct, hourly 10am-8pm
    cron -d 22  12:00

**Notes**

Shares its trailing time-series state comment with :ref:`attr-time`/:ref:`attr-today` — see
:ref:`state-attrs`.

.. _attr-date:

date
----

A date dependency keeps the node from *executing* until the suite's calendar date matches.
A field value of ``*`` means "any value".

**Attaches to:** Family / Task / Alias

**Syntax**

.. code-block:: shell

    date (dd|*).(mm|*).(yyyy|*)

**Parameters**

   dd . mm . yyyy
      Day, month, year, each independently either a concrete number or ``*``.

.. implementation::

   The wildcard is stored, internally, as ``0``.

**Examples**

.. code-block:: shell

    date *.*.2009
    date 1.*.2009
    date 01.*.*

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output a currently-free date gets a trailing ``# free`` marker — see
:ref:`state-attrs`.

.. _attr-day:

day
---

A day-of-week dependency keeps the node from *executing* until the suite's calendar day-of-week matches.

**Attaches to:** Family / Task / Alias

**Syntax**

.. code-block:: shell

    day weekday-name

**Parameters**

   weekday-name
      One of ``sunday``, ``monday``, ``tuesday``, ``wednesday``, ``thursday``, ``friday``,
      ``saturday`` (lower-case).

**Examples**

.. code-block:: shell

    day wednesday
    day monday
    day tuesday

**Notes**

Multiple ``day`` lines on one node are logically ORed — the node is executed on any listed day.

When generating ``STATE``/``MIGRATE``/``NET`` output it may also carry a trailing ``# free`` and/or ``# expired date:...``
marker — see :ref:`state-attrs`.

.. _attr-defstatus:

defstatus
---------

Sets the state a node is reset to at begin/re-queue time, overriding the implicit default of ``queued``.

The defstatus value :code:`suspended` is particular, in the sense that, at begin/re-queue time, it sets the node to
state :code:`queued` but requires an explicit resume instruction by the user to eventually allow the node to move to
the :code:`submitted` state (the node is effectively *suspended* in the meantime).

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    defstatus state

**Parameters**

   state
      One of ``unknown``, ``complete``, ``queued``, ``aborted``, ``submitted``, ``active``,
      ``suspended``. Not every value is meaningful on every node type; see :ref:`defstatus`
      for guidance.

**Semantics**

The writer only emits this line when the state differs from the built-in default (``queued``) —
a contract the source marks *"NEVER change, or will break client/server"*. At most one
``defstatus`` is allowed per node.

**Examples**

.. code-block:: shell

    defstatus complete
    defstatus suspended

.. _attr-edit:

edit
----

Defines a user variable on the node, inherited by all descendants unless shadowed/overridden by a
same-named variable lower in the tree.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    edit name 'value'

**Parameters**

   name
      Variable identifier, conventionally upper-case (not enforced).

   value
      Any text; quoted with ``'...'``, ``"..."``, or left unquoted if it contains no whitespace.
      Embedded newlines are escaped to a literal two-character ``\n`` (see :ref:`ch-lexical`).

**Examples**

.. code-block:: shell

    edit QUEUE 'ts'
    edit ECFCMD 'smssubmit %USER% %SCHOST% %ECFJOB% %ECFJOBOUT%'
    edit ACCOUNT '%ACCOUNT_FRA%'

**Notes**

Server-wide ``edit`` variables, suffixed by ``# server``, are included when generating ``STATE``/``MIGRATE``/``NET`` output.
These are *not* part of ``DEFS``-style output — see :ref:`state-header`.

Generated/loop variables (from :ref:`attr-repeat`) additionally appear as ``# NAME 'value'`` lines,
but only when generating ``STATE`` style output.

.. _attr-event:

event
-----

A boolean flag associated with a node that can be set via ``ecflow_client --event`` by a job
and used in triggers by other nodes.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    event [number] name [set]
    event number

**Parameters**

   number
      (Optional) unsigned integer identifier; a job can refer to the event by number or by name
      interchangeably.

   name
      (Optional) identifier; at least one of number/name must be given.

   set
      If present, the event's initial value is ``true`` (set) instead of the default ``false``
      (clear).

**Examples**

.. code-block:: shell

    event 0
    event name
    event 1 eventName

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a trailing ``# set``/``# clear`` is added if the current value
differs from the initial value — see :ref:`state-attrs`.

.. _attr-extern:

extern
------

Declares that a path (and optionally a specific attribute on it) referenced by a trigger
elsewhere in this file is defined outside of it. This suppresses "unresolved reference" errors.

**Attaches to:** Defs (root only)

**Syntax**

.. code-block:: shell

    extern /absolute/path[:attr-name]

**Examples**

.. code-block:: shell

    extern /limits:mars
    extern /limits:localhost
    extern /o/main/12/fc/model:step

**Notes**

See :ref:`ch-file-structure` for placement rules and the ``DEFS``/``STATE``-only vs. ``MIGRATE``/``NET``
asymmetry.

.. _attr-generic:

generic
-------

An open-ended, catch-all ``name value...`` attribute for data that doesn't fit an existing
keyword.

.. note::

   This is used to future-proof the system, and allow older servers to load newer definition files
   by treating unknown keywords as generic attributes (and effectively ignoring them).

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    generic name [value ...]

**Examples**

.. code-block:: shell

    generic a
    generic b c f

.. _attr-inlimit:

inlimit
-------

Consumes tokens from a named :ref:`attr-limit` while this node is active/submitted, capping
concurrency/resource usage.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    inlimit [-n] [-s] [path:]name [tokens]

**Parameters**

   path:name
      Name of the referenced limit, optionally qualified by the path to the node that owns it (if
      omitted, the parent hierarchy is searched).

   tokens
      Unsigned integer, number of tokens consumed. Default: ``1``.

   -n
      "Limit this node only" — on a suite/family, one token is consumed regardless of how many
      descendant tasks are active (controls how many *families* are active, not tasks). Mutually
      exclusive with ``-s``.

   -s
      "Limit submission" — the token is consumed only while the job is in the submitted state, not
      for its whole active lifetime.

**Semantics**

Multiple ``inlimit``\ s on the same node are logically ANDed (all must have capacity). An
``inlimit`` of the same name on a task takes priority over one on an enclosing family.

**Examples**

.. code-block:: shell

    inlimit /limits:localhost
    inlimit -n fam
    inlimit -s sub

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output a currently-held token gets a trailing ``# incremented:1``; ``STATE``
style additionally shows the referenced limit's live value — see :ref:`state-attrs`.

.. _attr-label:

label
-----

A named, free-text string a job can update at runtime (via ``ecflow_client --label``) for
display purposes. A label has no effect on scheduling.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    label name "value"

**Parameters**

   name
      Identifier.

   value
      Double-quoted initial text, may be empty (``""``).

**Examples**

.. code-block:: shell

    label foo ""
    label fred ""

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a job-updated label gets a trailing ``# "<new-value>"`` — see
:ref:`state-attrs`.

.. _attr-late:

late
----

This attribute flags a task as "late" if it spends too long submitted, or becomes active too late in the day, or
completes too late. This is purely advisory and does not alter scheduling.

**Attaches to:** Node (Suite, Family, Task, and Alias)

This attribute is semantically only meaningful on a task, but when set higher in the tree it is inherited.

**Syntax**

.. code-block:: shell

    late [-s +hh:mm] [-a hh:mm] [-c [+]hh:mm]

**Parameters**

   -s (submitted)
      Maximum time the task may remain submitted; always relative (a leading ``+`` is accepted but
      has no extra meaning).

   -a (active)
      Absolute time-of-day by which the task must have become active; if queued/submitted at
      the given time the task is flagged late.

   -c (complete)
      Time by which the task must complete. If prefixed with ``+``, the time is relative to when it became
      active; otherwise an absolute time-of-day.

**Semantics**

The three options may appear in any order; at least one must be given.

**Examples**

.. code-block:: shell

    late -s +00:15 -a 20:00 -c +02:00
    late -a 20:00 -c +02:00 -s +00:15

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a trailing ``# late`` is added once the late flag is actually set —
see :ref:`state-attrs`.

.. _attr-limit:

limit
-----

Defines a named, zero-based resource pool with a maximum capacity, to be referenced by
:ref:`attr-inlimit` elsewhere.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    limit name max

**Parameters**

   name
      Identifier, unique within the node it's defined on.

   max
      Unsigned integer capacity. E.g. if the limit is ``10``, up to 10 tokens may be consumed
      concurrently.

**Examples**

.. code-block:: shell

    limit fam 2
    limit task 4

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a limit in use gets a trailing ``# <value> <path-1> ...`` listing
which node paths currently hold tokens — see :ref:`state-attrs`.

.. _attr-meter:

meter
-----

A named integer progress gauge that a job can update at runtime (via ``ecflow_client --meter``).
A meter can be used as part of a trigger expression.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    meter name min max [colorchange]

**Parameters**

   name
      Identifier.

   min, max
      Integer bounds (may be negative); the meter's value always starts at ``min``.

   colorchange
      Optional integer within ``[min, max]``; used only as a GUI display hint (colour flips once the
      value passes it).

**Examples**

.. code-block:: shell

    meter myMeter  0 4 4
    meter myMeter1 0 5
    meter step -1 42 42

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a value away from its minimum gets a trailing ``# <value>`` — see
:ref:`state-attrs`.

.. _attr-mirror:

mirror
------

Polls a remote ecflow node's status, via a mirror server connection, and reflects it locally.
This allows other local nodes to be triggered by a remote node's completion.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    mirror --name name --remote_path path
      [--remote_host host] [--remote_port port] [--polling seconds]
      [--ssl] [--remote_auth auth] [--reason reason] [--propagate]

**Parameters**

   --name
      Identifier, unique among this node's mirror attributes.

   --remote_path
      Absolute path of the node on the remote server whose status is mirrored.

   --remote_host
      Default: ``%ECF_MIRROR_REMOTE_HOST%`` (falls back to ``localhost``).

   --remote_port
      Default: ``%ECF_MIRROR_REMOTE_PORT%`` (falls back to ``3141``).

   --polling
      Polling interval in seconds. Default: ``%ECF_MIRROR_REMOTE_POLLING%`` (falls back to ``120``).

   --ssl
      Flag — connect to the remote server over TLS.

   --remote_auth
      Path to an auth token file. Default: ``%ECF_MIRROR_REMOTE_AUTH%``.

   --reason
      Informational; last recorded failure reason.

   --propagate
      Flag — also propagate the remote node's own trigger-evaluation "why" reasoning locally.

**Examples**

.. code-block:: shell

    mirror --name operationsX --remote_path /s1/f1/t1 --ssl
    mirror --name operationsY --remote_path /s1/f1/t2 --remote_host %ECF_MIRROR_REMOTE_HOST% --polling 20 --remote_port %ECF_MIRROR_REMOTE_PORT% --ssl

.. _attr-queue:

queue
-----

A named list of string "steps" consumed sequentially by repeated job submissions. This attribute is a
lighter-weight alternative to ``repeat`` for driving a job through a fixed work list.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    queue name step-1 [step-2 ...]

**Parameters**

   name
      Identifier.

   step-N
      One or more opaque string tokens; the queue's current position advances via
      ``ecflow_client --queue`` from the job script.

**Examples**

.. code-block:: shell

    queue name 001 002 003
    queue name1 001 002 003

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a trailing ``# <current-index> <state-1> ...`` lists the current
position and each step's observed state — see :ref:`state-attrs`.

.. _attr-repeat:

repeat
------

This attribute turns a node into a "for loop", allowing it to re-queue itself and advance a generated variable
through a range or list of values each time it completes. A node may have at most one ``repeat``.

**Attaches to:** Node (Suite, Family, Task, and Alias)

Eight variants, each with its own value grammar:

.. list-table::
   :header-rows: 1
   :widths: 16 42 42

   * - Variant
     - Syntax
     - Example
   * - .. _attr-repeat-integer:

       ``integer``
     - ``repeat integer name start end [delta]``
     - ``repeat integer ITER 0 10 2``
   * - .. _attr-repeat-date:

       ``date``
     - ``repeat date name start end [delta]``
     - ``repeat date YMD 20090331 20121212 1``
   * - .. _attr-repeat-datelist:

       ``datelist``
     - ``repeat datelist name yyyymmdd [yyyymmdd ...]``
     - ``repeat datelist YMD 20090331 20121212 20121213``
   * - .. _attr-repeat-datetime:

       ``datetime``
     - ``repeat datetime name instant-start instant-end [delta]``
     - ``repeat datetime NAME 20250101T000000 20250102T000000 06:00:00``
   * - .. _attr-repeat-datetimelist:

       ``datetimelist``
     - ``repeat datetimelist name instant-1 [instant-2 ...]``
     - ``repeat datetimelist NAME 20250101T000000 20250102T060000``
   * - .. _attr-repeat-enumerated:

       ``enumerated``
     - ``repeat enumerated name "value-1" ["value-2" ...]``
     - ``repeat enumerated number "first" "second" "third"``
   * - .. _attr-repeat-string:

       ``string``
     - ``repeat string name "value-1" ["value-2" ...]``
     - ``repeat string STR str1 str2 str3``
   * - .. _attr-repeat-day:

       ``day``
     - ``repeat day step``
     - ``repeat day 1``

**Parameters**

   name
      Name of the generated loop variable. This name is used to compose the names of the variables exposed to the job.

   - For ``integer``: ``start``, ``end``, and ``delta`` are integers.
   - For ``date``: ``start``, ``end`` are yyyymmdd integers, and ``delta`` is a signed integer number of days (default: 1).
   - For ``datelist``: each ``yyyymmdd`` is an integer.
   - For ``datetime``: ``instant-start`` and ``instant-end`` are ISO 8601 instants (``yyyymmddTHHMMSS``), and ``delta`` is an ``hh:mm:ss`` duration.
   - For ``datetimelist``: each ``instant`` is a yyyymmddTHHMMSS (ISO 8601) string.
   - For ``enumerated``: each ``value`` is an opaque string (quoted if it contains whitespace).
   - For ``string``: each ``value`` is an opaque string (quoted if it contains whitespace).
   - For ``day``: ``step`` is an unsigned integer number of days.

**Semantics**

A ``repeat`` attribute re-queues the associated node after each completion, advancing the generated variable
through the specified range or list. The job can access the current value of the loop variable via ecflow variables.

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, every variant gets a trailing ``# <current-value-or-index>`` once it
has advanced past its start — see :ref:`state-attrs`. The generated loop variable(s) also appear
as separate ``# NAME 'value'`` lines, but only in ``STATE`` style (never ``MIGRATE``/``NET``).

.. _attr-time:

time
----

A time-of-day dependency keeps a node from *executing* before a single time of day, or before each slot of a
start/end/increment series that repeats within a day and then is exhausted. Unlike ``cron``, a ``time`` attribute does not repeat
across days — that can be achieved by using a ``repeat`` on the node to re-queue it for the next day.

**Attaches to:** Family / Task / Alias

**Syntax**

.. code-block:: shell

    time [+]hh:mm
    time [+]hh:mm hh:mm hh:mm   # start end increment

**Parameters**

   +hh:mm (single)
      A leading ``+`` makes the single time relative to node activation instead of an absolute
      time-of-day.

   start end increment
      A repeating series within the day: fires at ``start``, then every ``increment`` thereafter, up
      to and including ``end``.

**Examples**

.. code-block:: shell

    time 15:00
    time +15:00
    time 10:00 20:00 01:00

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a trailing ``# free isValid:false nextTimeSlot/... relativeDuration/...``
comment may follow (any subset of these four tokens) — see :ref:`state-attrs`.

.. _attr-today:

today
-----

A ``today`` attribute uses identical grammar and single/series forms as :ref:`attr-time`,
but is only ever valid for the day the enclosing suite began.

**Attaches to:** Family / Task / Alias

**Syntax**

.. code-block:: shell

    today [+]hh:mm
    today [+]hh:mm hh:mm hh:mm   # start end increment

**Parameters**

   +hh:mm (single)
      A leading ``+`` makes the single time relative to node activation instead of an absolute
      time-of-day.

   start end increment (series)
      A repeating series within the suite-begin day: fires at ``start``, then every ``increment``
      thereafter, up to and including ``end``.

**Examples**

.. code-block:: shell

    today 15:00
    today 10:00 20:00 01:00

**Notes**

Same trailing state comment as :ref:`attr-time` — see :ref:`state-attrs`.

.. _attr-trigger:

trigger
-------

A boolean expression keeping the node from *executing* until the expression evaluates to true.

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    trigger expression
    trigger -a expression   # AND-continuation
    trigger -o expression   # OR-continuation

**Parameters**

   expression
      text expression evaluated by ecflow, considering node-path state comparisons
      (``== complete``, ``eq``, ``ne``, ...), event/meter references (``path:event_name``,
      ``path:meter_name ge 10``), and boolean combinators ``and``/``or``/``not``.

   -a / -o
      See :ref:`attr-complete` — identical continuation mechanism.

**Examples**

.. code-block:: shell

    trigger init == complete
    trigger ./901 == complete
    trigger ref eq complete

**Notes**

The full trigger/complete abstract-syntax-tree operator set (arithmetic, function-call forms,
every comparison spelling) is out of scope for this file-format specification.

When generating ``STATE`` style, a currently-free expression additionally gets a trailing ``# (free)``
marker, plus a full AST evaluation-tree dump — see :ref:`state-attrs`.

.. _attr-verify:

verify (test tooling)
---------------------

.. important::

   DO NOT use this attribute!

   This attribute is **ONLY** for ecflow testing, and is not part of the public API.
   It may be removed or changed at any time.

Embeds an expected state-transition count directly in the definition file, so that the ecflow test suite
can assert behaviour without separate validation procedures. *Not intended for public use!*

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    verify state:expected-count

**Parameters**

   state
      One of ``unknown``, ``complete``, ``queued``, ``aborted``, ``submitted``, ``active``.

   expected-count
      Unsigned integer — how many times the node is expected to pass through that state.

**Examples**

.. code-block:: shell

    verify complete:3
    verify aborted:3

**Notes**

When generating ``STATE``/``MIGRATE``/``NET`` output, a trailing ``# <actual-count>`` shows the count observed so far —
see :ref:`state-attrs`.

.. _attr-zombie:

zombie
------

This attribute specifies how the server should react to a *zombie* job
(i.e. a running job whose process identifier, password, or path no longer matches what the server expects).

**Attaches to:** Node (Suite, Family, Task, and Alias)

**Syntax**

.. code-block:: shell

    zombie type:action:child-cmds:lifetime

**Parameters**

   type
      One of ``user``, ``ecf``, ``ecf_pid``, ``ecf_passwd``, ``ecf_pid_passwd``, ``path``.

   action
      One of ``fob``, ``fail``, ``adopt``, ``remove``, ``block``, ``kill``.

   child-cmds
      Optional comma-list restricting which child commands trigger the action, from ``init``,
      ``event``, ``meter``, ``label``, ``wait``, ``queue``, ``abort``, ``complete``. Empty means
      "all".

   lifetime
      Optional unsigned integer, seconds the zombie record survives in the server. Defaults if
      omitted: ``300`` (user), ``3600`` (ecf/ecf_pid/ecf_passwd/ecf_pid_passwd), ``900`` (path);
      minimum ``60``.

**Examples**

.. code-block:: shell

    zombie ecf:fob:init:100
    zombie path:fail:init:100
    zombie user:adopt:meter:100
    zombie ecf:remove:init,event,meter,label,wait,queue,complete:2000

**Notes**

The full behavioural semantics of each action (i.e. what the server does exactly to handle the *zombie*)
belong to a separate zombie-handling operations guide.

.. _ch-example:

Definition File Example
=======================

The following presents a compact example of a definition file (``.def``) exercising a
representative cross-section of the grammar above.

.. code-block:: shell

    #5.14.0
    extern /operations/limits:disk_io

    suite forecast_pipeline

      edit ECF_HOME '/home/msops/ecflow_home'
      edit ECF_INCLUDE '/home/msops/include'
      edit STREAM 'oper'
      limit concurrent_runs 4
      inlimit concurrent_runs
      autoarchive +24:00

      clock real 1.1.2026

      family acquisition
        inlimit /operations/limits:disk_io 2
        time 00:15

        task fetch_observations
          label status ""
          meter progress 0 100 100

        task fetch_boundary_conditions
          trigger fetch_observations == complete
          event 1 data_ready

      endfamily

      family forecast
        trigger acquisition == complete
        repeat integer MEMBER 1 50 1

        task run_member
          late -s +00:15 -a 06:00 -c +04:00
          inlimit concurrent_runs
          autocancel +48:00

      endfamily

      family dissemination
        trigger forecast == complete

        task publish
          aviso --name publish_ready --listener '{ "event": "mars", "request": { "class": "od", "stream": "oper", "step": [0,6,12] } }' --url %ECF_AVISO_URL% --schema %ECF_AVISO_SCHEMA% --polling %ECF_AVISO_POLLING%

      endfamily

    endsuite
    # enddef

.. Both _grammar and _ch-grammar are included here for cross-reference convenience.
   While _grammar is used across the documentation to refer to the grammar section,
   _ch-grammar is used inside this document itself.

.. _grammar:
.. _ch-grammar:

Definition File Grammar
=======================

The formal grammar of a definition file is as follows.

.. list-table:: Meta-notation
   :header-rows: 1
   :widths: 20 80

   * - Token
     - Meaning
   * - ``*``
     - zero or more
   * - ``!``
     - zero or one
   * - ``+``
     - one or more
   * - ``>>``
     - sequence / concatenation
   * - ``|``
     - alternation

.. code-block:: text

    defs            : *( extern | nextline ) >> *suite >> EOF
    extern          : "extern" >> absolutepath >> !( ":" >> identifier )
    suite           : "suite" >> node_name >> *node_attr >> clock_block >> *( family | task ) >> "endsuite"
    family          : "family" >> node_name >> *node_attr >> *( family | task ) >> "endfamily"
    task            : "task"  >> node_name >> *node_attr >> !"endtask"

    clock_block     : !( "clock" >> ( "real" | "hybrid" ) >> !clock_date >> !clock_gain >> !"-s"
                    :     >> !( "endclock" >> !clock_date >> !clock_gain ) )
    clock_date      : integer_day >> "." >> integer_month >> "." >> integer_year
    clock_gain      : ( "+" >> hh_mm ) | hh_mm | integer

    node_attr       : variable | trigger | complete | repeat | limit | inlimit | label
                    : | meter | event | late | defstatus | autocancel | autoarchive
                    : | autorestore | zombie | verify | queue | generic | aviso | mirror | cron
                    : | time_dep                                    -- time/today/date/day: family/task/alias only, not suite
    time_dep        : time | today | date | day

    variable        : "edit" >> identifier >> varvalue
    trigger         : "trigger" >> !("-a" | "-o") >> expression
    complete        : "complete" >> !("-a" | "-o") >> expression
    expression      : printable_chars >> !("\" >> nextline >> expression)
    repeat          : "repeat" >> repeat_type
    repeat_type     : ("integer" >> identifier >> integer >> integer >> !integer)
                    : | ("date" >> identifier >> ymd >> ymd >> !integer)
                    : | ("datelist" >> identifier >> +ymd)
                    : | ("datetime" >> identifier >> instant >> instant >> !duration)
                    : | ("datetimelist" >> identifier >> +instant)
                    : | ("enumerated" >> identifier >> +quotedstring)
                    : | ("string" >> identifier >> +quotedstring)
                    : | ("day" >> integer)
    limit           : "limit" >> identifier >> unsigned_int
    inlimit         : "inlimit" >> !"-n" >> !"-s" >> ( ( nodePath >> ":" >> identifier ) | identifier ) >> !unsigned_int
    label           : "label" >> identifier >> quotedstring
    meter           : "meter" >> identifier >> integer >> integer >> !unsigned_int
    event           : "event" >> ( eventnumber >> !eventname | eventname ) >> !"set"
    late            : "late" >> late_option >> !late_option >> !late_option
    late_option     : ("-s" >> !"+" >> hh_mm) | ("-a" >> hh_mm) | ("-c" >> !"+" >> hh_mm)
    defstatus       : "defstatus" >> dstate
    autocancel      : "autocancel" >> ( ("+" >> hh_mm) | hh_mm | unsigned_int )
    autoarchive     : "autoarchive" >> ( ("+" >> hh_mm) | hh_mm | unsigned_int ) >> !"-i"
    autorestore     : "autorestore" >> +nodePath
    zombie          : "zombie" >> zombie_type >> ":" >> !zombie_action >> ":" >> !child_cmd_list >> ":" >> !unsigned_int
    zombie_type     : "user" | "ecf" | "ecf_pid" | "ecf_passwd" | "ecf_pid_passwd" | "path"
    zombie_action   : "fob" | "fail" | "adopt" | "remove" | "block" | "kill"
    child_cmd_list  : child_cmd >> *( "," >> child_cmd )
    child_cmd       : "init" | "event" | "meter" | "label" | "wait" | "queue" | "abort" | "complete"
    verify          : "verify" >> nstate >> ":" >> unsigned_int
    queue           : "queue" >> identifier >> +string
    generic         : "generic" >> identifier >> *string
    aviso           : "aviso" >> "--name" >> identifier >> "--listener" >> "'" >> jsonstring >> "'"
                    : >> *( "--url" >> string | "--schema" >> string | "--polling" >> string
                    :     | "--revision" >> unsigned_int | "--auth" >> string | "--reason" >> string )
    mirror          : "mirror" >> "--name" >> identifier >> "--remote_path" >> string
                    : >> *( "--remote_host" >> string | "--remote_port" >> string | "--polling" >> string
                    :     | "--ssl" | "--remote_auth" >> string | "--reason" >> string | "--propagate" )
    cron            : "cron" >> *( ("-w" >> weekday_list) | ("-d" >> dom_list) | ("-m" >> month_list) ) >> timeseries
    time            : "time" >> !"+" >> timeseries
    today           : "today" >> !"+" >> timeseries
    date            : "date" >> (integer_day|'*') >> "." >> (integer_month|'*') >> "." >> (integer_year|'*')
    day             : "day" >> weekday_name

    timeseries      : hh_mm | ( hh_mm >> hh_mm >> hh_mm )
    hh_mm           : two_int >> ":" >> two_int
    node_name       : (alnum|"_") >> *(alnum|"_"|".")
    identifier      : (alnum|"_") >> *(alnum|"_"|".")
    nodePath        : absolutepath | dotpath | dotdotpath
    absolutepath    : "/" >> identifier >> *( "/" >> identifier )
    dotpath         : "." >> +( "/" >> identifier )
    dotdotpath      : ".." >> +( "/" >> identifier )
    varvalue        : quotedstring | tickquotedstring | identifier
    quotedstring    : '"' >> *printable_chars >> '"'
    tickquotedstring: "'" >> *printable_chars >> "'"
    jsonstring      : -- as per RFC 8259 JSON grammar --
    comment         : "#" >> *printable_chars >> newline
    nextline        : newline | comment
    dstate          : "unknown"|"complete"|"queued"|"aborted"|"submitted"|"active"|"suspended"
    nstate          : "unknown"|"complete"|"queued"|"aborted"|"submitted"|"active"
    weekday_name    : "sunday"|"monday"|"tuesday"|"wednesday"|"thursday"|"friday"|"saturday"
    two_int         : 2-digit integer
    integer_day     : integer in range 1-31 (day of month)
    integer_month   : integer in range 1-12 (month)
    integer_year    : 4-digit integer (yyyy)
    ymd             : 8-digit integer (yyyymmdd)
    instant         : ymd >> "T" >> two_int >> two_int >> two_int
    duration        : integer >> ":" >> two_int >> ":" >> two_int

.. _ch-runtime-state:

State and Checkpoint Format
===========================

.. important::

   The Checkpoint format is **not** considered a human-readable interchange format,
   and should **NOT** be edited by hand.

   It is not considered as part of the public API, and is, technically, for ecflow's internal use only.

The three non-``DEFS`` output styles — ``STATE``, ``MIGRATE``, ``NET`` — share the same mechanism for
layering runtime information on top of the structural grammar from the preceding sections.

The extra state information is appended after a ``#`` on the *same physical line* as the structural item
it describes. Additionally, a handful of new top-level keywords (``defs_state``, ``calendar``, ``history``)
and one new block (``alias``/``endalias``) are introduced.

A checkpoint (``.checkpt``) file is written using the ``MIGRATE`` style, with indentation disabled
purely to save time and disk space when storing large trees.

.. list-table:: What each style adds on top of the DEFS grammar (✅ = present, ❓ = if requested, — = absent)
   :header-rows: 1
   :widths: 40 12 12 15 12

   * - Addition
     - DEFS
     - STATE
     - MIGRATE
     - NET
   * - ``defs_state`` header + server variables
     - —
     - ✅
     - ✅
     - ✅
   * - ``history`` lines
     - —
     - ❓
     - ❓
     - ❓
   * - ``# server state:`` comment
     - —
     - ✅
     - —
     - —
   * - ``extern`` lines
     - ✅
     - ✅
     - —
     - —
   * - Per-node state suffix (``state:``, ``flag:``, …)
     - —
     - ✅
     - ✅
     - ✅
   * - ``calendar`` line
     - —
     - ✅
     - ✅
     - ✅
   * - ``alias``/``endalias`` blocks
     - —
     - ✅
     - ✅
     - ✅
   * - Attribute-level state (limit, meter, repeat, …)
     - —
     - ✅
     - ✅
     - ✅
   * - Trigger/complete AST dump + ``# (free)``
     - —
     - ✅
     - —
     - —
   * - Generated/loop variables (``# NAME 'value'``)
     - —
     - ✅
     - —
     - —

The following sections describe the new top-level keywords and the per-node state suffixes in detail.

.. _state-header:

Global state
------------

A set of global state information is included at the top of the file written in any of ``STATE``, ``MIGRATE``, or ``NET`` styles.

The following entries detail the syntax and semantics of this global state information.

defs_state
~~~~~~~~~~

The ``defs_state`` line effectively switches the parser out of ``DEFS`` mode,
and configures/enables the collection of the definition + (server-wide) state.

**Syntax**

.. code-block:: shell

    defs_state (STATE|MIGRATE|NET) [state>:state] [flag:flags] [state_change:n] [modify_change:n] [server_state:state] cal_count:n

**Parameters**

   ``STATE`` | ``MIGRATE`` | ``NET``
      Defines the parse mode for every subsequent line. If this line is absent, the whole file is treated as plain ``DEFS``.

   state>:
      Defines the Defs-level state (one of ``complete``, ``queued``, ``aborted``, ``submitted``, ``active``).
      Notice the use of a ``>`` inside the token ``state>:``, done deliberately to avoid collision with a
      per-node ``state:``.

   flag:
      Comma-separated server level flags — see :ref:`state-flags`.

   state_change: / modify_change:
      Internal monotonically increasing counters, used by clients to detect whether their cached
      copy of the definition is stale.

   server_state:
      Defines the server state (one of ``HALTED``, ``SHUTDOWN``, ``RUNNING``).

   cal_count:
      A calendar-update counter.

**Examples**

.. code-block:: shell

    -- real MIGRATE checkpoint, version 5.18.0, all optional fields at their default:
    defs_state MIGRATE cal_count:0

    -- constructed to show every optional field populated:
    defs_state STATE flag:message state_change:1042 modify_change:1038 server_state:RUNNING cal_count:3

Server variables
~~~~~~~~~~~~~~~~

Immediately after the ``defs_state`` line, two distinct groups of ``edit`` lines are
written:

  - User-defined server variables

    Plain ``edit NAME 'value'`` lines, syntactically identical to a normal ``edit`` — these are
    variables an operator set directly on the server (not inside any suite).

  - Built-in server variables

    The same syntax with a trailing ``# server`` suffix, e.g. ``edit ECF_HOME '.' # server``.

    This list will include ecflow generated server-level default variables
    (``ECF_MICRO``, ``ECF_HOME``, ``ECF_JOB_CMD``, ``ECF_HOST``, …).

Server state comment
~~~~~~~~~~~~~~~~~~~~

When generating a ``STATE`` style output, the following line is added immediately after the ``defs_state`` line:

.. code-block:: shell

    # server state: <HALTED|SHUTDOWN|RUNNING>

This is purely redundant considering the ``server_state:`` above, but is intended to help quick visual inspection.

History
~~~~~~~

When edit-history saving is requested (always true for a ``.checkpt`` save),
one line per historical edit follows, before any suite:

.. code-block:: shell

    history /node/path \bMSG:[13:42:07 1.1.2026] message text\bMSG:[...] ...

The backspace character (``\b``) separates individual messages on the line.

This special character was chosen specifically to avoid clash with more obvious separators
(space, ``%``, ``:``, ``[]``, digits, ``-``) which are already meaningful elsewhere in the grammar.

The format of the message begins with ``MSG:[HH:MM:SS D.M.YYYY]`` to allow pruning entries on reload.

.. _state-node:

Node specific state
-------------------

Node specific state information is included when written in any of ``STATE``, ``MIGRATE``, or ``NET`` styles.
This information is appended to the end of each node's header line, after a ``#`` separator,
or in subsequent lines. Each attribute may also have its own state information appended after
a ``#`` separator on the same line.

The following entries detail the syntax and semantics of this node specific state information.

Per-node state suffix
~~~~~~~~~~~~~~~~~~~~~

Every ``suite``/``family``/``task``/``alias`` header line is appended with a trailing, single
``#``. This introduces a block of the following labelled tokens.

**For Suite, Family, Task and Alias**

state:
   The current node state (``unknown``/``complete``/``queued``/``aborted``/``submitted``/``active``).

dur:
   The suite-calendar duration at the moment this state was entered (e.g. ``01:30:00``).

flag:
   A list of comma-separated flags — see :ref:`state-flags`.

suspended:1
   An indication only present if the node is currently suspended.

rt:
   The accumulated state-change runtime.

**Only for Suite**

begun:1
   An indication only present once the suite has begun, written first before any of the common fields.

**Only for Task and Alias**

Written before the common fields, in this order:

passwd:
   The job one-time submission password (omitted if empty).
   Not an actual secret, but instead a hash used to identify the job.

rid:
   The remote/process id of the running job.

abort<:...>abort
   A text describing the reason to abort the task or alias.

try:
   The current value of the submission attempt counter.

**Only for Task**

alias_no:
   The numeric suffix that will be used to name the next alias of the task.

**Examples**

.. code-block:: shell

    -- real MIGRATE checkpoint excerpt:
    suite test_two_autoarchive_in_hierarchy # begun:1 state:complete
    family family # state:complete
    task t # try:1 state:complete

    -- constructed, showing more fields at once:
    task run_member # try:2 state:aborted dur:01:30:00 flag:task_aborted,late rt:04:12:33

.. _state-calendar:

calendar
~~~~~~~~

This represents the live calendar snapshot of the suite, and thus is only present attached to a ``suite``.

**Syntax**

.. code-block:: shell

    calendar initTime:datetime suiteTime:datetime duration:hh:mm:ss
             initLocalTime:datetime lastTime:datetime [calendarIncrement:hh:mm:ss] [dayChanged:1]

**Parameters**

   initTime
      The calendar's starting date/time, taken from the suite's ``clock``.

   suiteTime
      The suite's own tracked current date/time.

   duration
      Elapsed suite-calendar time since ``initTime``.

   initLocalTime
      The real wall-clock time when the calendar was initialised.

   lastTime
      The real wall-clock time of the last calendar update.

   calendarIncrement
      Only if changed from the default — the step size used to advance the calendar.

   dayChanged:1
      Only if true — marks that a day boundary was just crossed.

**Example (real, from a MIGRATE checkpoint)**

.. code-block:: shell

    calendar initTime:2009-Oct-12 00:00:00 suiteTime:2009-Oct-12 01:00:00 duration:01:00:00 initLocalTime:2026-Jul-09 09:11:11 lastTime:2026-Jul-09 09:11:11 calendarIncrement:01:00:00

.. _state-attrs:

Attribute specific state
------------------------

Attribute state information storage follows the same approach as state information for nodes, and is appended to the end of the attribute's line, after a ``#`` separator.

.. list-table::
   :header-rows: 1
   :widths: 18 40 20 22

   * - Attribute
     - Extra content
     - Styles
     - Example
   * - :ref:`attr-trigger` / :ref:`attr-complete`
     - ``# (free)`` if free, followed by full AST dump
     - ``STATE`` only
     - ``trigger fetch_observations == complete`` then ``# (free)``
   * - :ref:`attr-repeat`
     - ``# <current-value-or-index>``, if not same as start
     - all three
     - ``repeat integer MEMBER 1 50 1 # 7``
   * - :ref:`attr-edit` (generated variables)
     - ``# NAME 'value'``
     - ``STATE`` only
     - ``# MEMBER_YYYY '<invalid>'``
   * - :ref:`attr-limit`
     - ``# <value> <path-1> <path-2> ...``, if non-zero
     - all three
     - ``limit concurrent_runs 4 # 2 /forecast_pipeline/forecast/run_member/7``
   * - :ref:`attr-inlimit`
     - ``# incremented:N`` if holding tokens

       ``# referenced limit(value) (val)``
     - all three

       ``STATE`` only
     - ``inlimit concurrent_runs # incremented:1``
   * - :ref:`attr-label`
     - ``# "<new-value>"`` if not initial value
     - all three
     - ``label status "" # "40% complete"``
   * - :ref:`attr-meter`
     - ``# <value>`` if not the minimum value
     - all three
     - ``meter progress 0 100 100 # 40``
   * - :ref:`attr-event`
     - ``# set`` / ``# clear`` if not initial value
     - all three
     - ``event 1 data_ready # set``
   * - :ref:`attr-time` / :ref:`attr-today` / :ref:`attr-cron`
     - any subset of

       ``# free``, ``isValid:false``, ``nextTimeSlot/<hh:mm>``,
       ``relativeDuration/<hh:mm:ss>``
     - all three
     - ``time 10:30 # free isValid:false nextTimeSlot/10:30 relativeDuration/00:00:00``
   * - :ref:`attr-date`
     - ``# free`` if currently free
     - all three
     - ``date 1.*.2009 # free``
   * - :ref:`attr-day`
     - ``# free`` and/or

       ``# expired date:<simple-date>``
     - all three
     - ``day wednesday # expired date:2026-Jul-08``
   * - :ref:`attr-late`
     - ``# late`` if currently set
     - all three
     - ``late -a 20:00 # late``
   * - :ref:`attr-verify`
     - ``# <actual-count>`` as observed so far
     - all three
     - ``verify complete:3 # 2``
   * - :ref:`attr-queue`
     - ``# <index> <state-1> <state-2> ...``
     - all three
     - ``queue name 001 002 003 # 1 complete queued queued``

.. note::

   The ``time``/``today``/``cron`` state comment uses ``/`` rather than ``:`` to separate
   ``nextTimeSlot``/``relativeDuration`` from their values.

   This is done deliberately, because ``:`` is already used in time syntax on the same line.

.. _state-flags:

Reference: flag
---------------

The full Flag type enumeration, to be used as comma-separated names appended after a ``flag:`` token.

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Name
     - Meaning
   * - ``force_aborted``
     - Node will not run again — ``try_no`` exceeded ``ECF_TRIES`` and the task was aborted by a
       user action.
   * - ``user_edit``
     - A user-initiated edit was applied to this task.
   * - ``task_aborted``
     - The task aborted.
   * - ``edit_failed``
     - Variable substitution into the job failed.
   * - ``ecfcmd_failed``
     - The job-submission command failed.
   * - ``no_script``
     - The task's script/include file could not be found.
   * - ``killed``
     - Node will not run again — ``try_no`` exceeded ``ECF_TRIES`` and the task was killed by a
       user action.
   * - ``late``
     - A :ref:`attr-late` threshold has been breached.
   * - ``message``
     - There is a pending message / edit-history entry associated with the node.
   * - ``by_rule``
     - The node was forced complete by its own :ref:`attr-complete` expression, not by the job
       actually finishing.
   * - ``queue_limit``
     - Reserved; not currently used.
   * - ``task_waiting``
     - The task is waiting on a trigger expression evaluated via an interactive client command.
   * - ``locked``
     - Reserved (server-level); not currently used.
   * - ``zombie``
     - Set/cleared internally during zombie handling; not surfaced in the GUI.
   * - ``no_reque``
     - Internal — skip resetting time slots on the next re-queue (used by forced complete/run).
   * - ``archived``
     - This container node has been auto-archived.
   * - ``restored``
     - Avoid re-archiving this container until it is next re-queued.
   * - ``threshold``
     - Job submission exceeded a time threshold (slow disk, huge includes, overloaded machine).
   * - ``sigterm``
     - Records that the server received ``SIGTERM`` — mainly used in testing.
   * - ``log_error``
     - Error opening or writing to the server log file.
   * - ``checkpt_error``
     - Error saving the checkpoint file.
   * - ``killcmd_failed``
     - The kill command failed.
   * - ``statuscmd_failed``
     - The status command failed.
   * - ``status``
     - Set on a task; used together with the status command.
   * - ``remote_error``
     - Error connecting to a remote source (:ref:`attr-mirror`/:ref:`attr-aviso`).
   * - ``not_set``
     - Sentinel value; never actually set on a node.

.. _state-alias:

Reference: alias / endalias
---------------------------

An alias is created by cloning a task, to effectively become a task descendant.

An alias is never emitted when generating ``DEFS`` output, but is present in ``STATE``, ``MIGRATE``, and ``NET`` output.

The alias is defined by an ``alias`` token, followed by the same attributes as a task.
A single ``endalias`` closes *all* of a task's aliases together, written once after the last one.

**Syntax**

.. code-block:: shell

    task name
      ...
      alias alias-name
        ... (same Node attributes as a task)
      alias alias-name-2
        ...
    endalias

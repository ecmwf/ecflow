.. _timeline:

Viewing the Timeline
////////////////////

The **Timeline** tab in an Info panel shows, for a given server, how the
state of each node changed over a period of time. When the tab is first
opened the **server log file** is fetched (it is either accessed locally
or transferred via the network), then the file contents are parsed to
collect the state changes of each node. The result is presented as a
chart with one row per node and a horizontal bar showing how the state
of the node changed across time.

Since the Timeline tab is associated with the server (and not with a
particular node) it stays loaded when a different node is selected in
the same server; selecting a node only highlights and scrolls to the
corresponding row in the chart.

Timeline view
=============

The Timeline view is selected by pressing the **Timeline view mode**
button on the top right of the panel.

Each row in the chart represents a node (suite, family or task) and is
labelled with its path. The horizontal axis represents time, and for
each node a sequence of coloured segments is drawn, one for each period
the node spent in a given state. The colours match the state colours
used elsewhere in ecFlowUI (e.g. in the Node tree and the Table view):
**queued**, **submitted**, **active**, **complete**, **aborted**,
**suspended** and **unknown**. The **complete** state is rendered with a
lighter shade so that long-running suites remain easy to read.

The time axis at the top of the chart shows date and time labels and can
be used to zoom into a period (see below).

.. figure:: /_static/ecflow_ui/viewing_the_timeline/viewing_the_timeline__timeline_view_mode.png
   :width: 5.98770in
   :height: 3.01112in
   :align: center

   Timeline view


Duration view
=============

The Duration view is selected by pressing the **Duration view mode**
button on the top right of the panel.

The chart can be switched to a **Duration view**, which replaces the
per-node bar with two columns showing, for each node, the duration spent
in the **submitted** and **active** states. A combo box selects whether
the duration shown is the **first duration in period** (the duration of
the first occurrence of the state within the displayed period) or the
**mean duration** (the average duration across all occurrences).

.. figure:: /_static/ecflow_ui/viewing_the_timeline/viewing_the_timeline__duration_view_mode.png
   :width: 5.99340in
   :height: 3.01112in
   :align: center

   Duration view

Node details
============

Double-clicking a row (or using **Show details** from the context menu)
opens a details dialog for the selected node. The dialog lists the full
history of state changes for the node together with summary statistics
(minimum, mean, median and quartiles) of the time spent in each state,
and a compact view showing one row per calendar day.

.. list-table::
   :header-rows: 1
   :align: center

   * - Overview
     - Daily cycle

   * - .. image:: /_static/ecflow_ui/viewing_the_timeline/viewing_the_timeline__timeline_details_overview.png
          :width: 3.47400in
          :height: 2.73330in
          :align: center

     - .. image:: /_static/ecflow_ui/viewing_the_timeline/viewing_the_timeline__timeline_details_daily_cycle.png
          :width: 3.47400in
          :height: 2.73330in
          :align: center

Interaction with the chart and other controls
=============================================

Context menu
------------

The chart area supports a context menu with the following options:

-  **Show details**: opens the node details dialog for the selected row (see `Node details`_)
-  **Lookup node in tree**: selects the node in the Node tree and scrolls to it
-  **Copy node path**: copies the node path to the clipboard

Zoom
----

Press the dedicated zoom in/out buttons on the toolbar, then click and drag
on the time axis to define a zoom rectangle; when the mouse is released the
chart zooms into the selected period. The selected period can also be
adjusted using the **Extend period to the start**, **Extend period to the
end** and **Show full period** buttons, or by adjusting the date/time fields
with the exact bounds of the period.

Filtering and sorting
---------------------

The toolbar provides a path filter (supporting regular expressions or
wildcards) together with the following options:

-  **Show only children of the current node**: restricts the chart to
   the subtree of the currently selected node

-  **Show tasks only**: hides suites and families from the chart

-  **Show only nodes with a state change in the current period**: hides
   nodes whose state did not change within the displayed period

The rows can be sorted by path, by time (of the most recent state
change) or following the order of the node tree, in ascending or
descending order.

Reloading and log file information
----------------------------------

The **Reload** button re-fetches and re-parses the current log file,
keeping the currently displayed period. A collapsible information label
at the top of the panel shows the log file path, server, host and port,
and, once expanded, additional details such as the file size, the time
taken to fetch it, and whether the file had to be truncated because it
exceeded the configured maximum size.

Timeline configuration options
------------------------------

There are some configuration options available for the Timeline panel at
Tools > Preferences > Server settings > Server load and timeline. These
settings, which are shared with the Server load panel, control the
maximum size of the log file that is read to build the Timeline chart,
and can be applied globally or defined per server.

Inspecting older log files
--------------------------

By default the Timeline panel fetches the current log file. To work
with other (e.g. older or archived) log files switch to the **Archived
log** mode and load a single (or multiple) log file(s) using the file
load button. A preview dialog lists the chosen file(s) before they are
loaded, so that files which cannot be parsed can be excluded.

Handling the server log file
============================

Fetching the log file
---------------------

The Timeline panel always operates on the log file identified by the
**ECF_LOG** variable of the server's root node. The download is done
automatically when the panel is first opened, and can be repeated at
any time.

.. implementation::

    The sequence of steps to fetch the log file considers the ``ECF_LOG``
    path, the server host and port, and the configured transfer user.
    The log file local or remote availability impacts the retrieval steps,
    which may consider only the last N bytes of the log file to avoid
    excessive data transfer.

    Expand for more details.

    .. extension::

    The file is obtained through the following sequence:

    1. The **ECF_LOG** value, the server host, port and the configured
       transfer user (Tools > Preferences > Server settings, see below) are
       read from the server.

    2. A local existence check is performed on the **ECF_LOG** path. If a
       file with that exact path exists on the machine running ecFlowUI
       (i.e. ecFlowUI and the server share a filesystem), it is read
       directly, and no network transfer takes place at all.

    3. Otherwise the file is fetched asynchronously over the network. Only
       the **last N bytes** of the log are requested, where N is the
       **Maximum log file size for Timeline data** preference (100 MB by
       default, configurable globally or per server). The full log is never
       copied.

    4. The transfer is delegated to a bundled shell script,
       ``ecflow_ui_transfer_file.sh``, launched as a child process
       (``/bin/sh -c "..."``). Two modes are possible:

       -  **Direct SSH** (the common case): the script is invoked with the
          remote user and host, e.g. equivalent to
          ``ecflow_ui_transfer_file.sh -m file -s <ECF_LOG> -t <tmp_file> -b last -v <N> -u <user> -h <host>``.

       -  **SOCKS/proxychains**: when a SOCKS proxy is configured, the
          script is instead invoked with a local SOCKS port
          (``-p <port>`` instead of ``-h <host>``), and connects to
          ``localhost`` through the proxy.

       In both cases an SSH **ProxyJump** (``-J``) can additionally be
       supplied when a jump host is configured.

    5. Inside the script, fetching the last N bytes runs a **single SSH
       command** against the remote host (no ``scp`` involved for this
       mode): it opens ::

           ssh -o StrictHostKeyChecking=no -o PasswordAuthentication=no <user>@<host> "<remote command>" > <local_tmp_file>

       where ``<remote command>`` checks the remote file size and runs
       either ``cat <ECF_LOG>`` (if the whole file is not larger than N
       bytes) or ``tail -c <N> <ECF_LOG>``, so that only the tail of the log
       is streamed back. The SSH connection is compressed (``-C``), does
       not prompt for a password, and skips host-key checking. When SOCKS is
       used, the SSH options additionally include
       ``-o ProxyCommand="nc -x 127.0.0.1:<port> %h %p"``. Whole-file
       transfers (used by other panels, but not by the Timeline, which
       always requests only the last N bytes) are instead performed with
       ``scp`` by the same script.

    6. The remote command's standard output is redirected straight into a
       local temporary file by the shell (no intermediate copy on the
       remote host). Once the child process exits, ecFlowUI checks the
       process exit code and the resulting file, and, on success, hands the
       temporary file to the log parser; on failure, an error message
       (built from the process' standard output/error) is shown in the
       panel.

    7. Reloading (see below) repeats the whole sequence; only the log
       file's tail is re-fetched, so reloading a large log is not
       proportionally slower than the original load.


Processing the log file
-----------------------

The log file is parsed to extract the state changes of each node, which
are then used to draw the Timeline chart. The parsing is done in a single
pass over the log file.

.. implementation::

    Once fetched, the log file is scanned line by line and only the lines
    recording a **state change** are considered; every other line (messages,
    errors, or any other log entry) is ignored. A line indicating a state
    change has the form:

    .. code-block:: text

        LOG:[22:45:30 21.4.2018]  complete: /suite/family/task
        LOG:[22:45:30 21.4.2018]  submitted: /suite/family/task job_size:16408

    Expand for more details.

    .. extension::

    For each such line, the following pieces are extracted:

    -  the **timestamp**, taken from between the square brackets and
       interpreted as UTC;
    -  the **state**, taken from the word between the closing bracket and
       the following colon; and
    -  the **node path**, taken as the first whitespace-delimited token
       that follows the state.

    The state name is resolved against the same state registry used
    throughout ecFlowUI, so the events actually recognised are the six
    states a node goes through during its lifecycle: **unknown**,
    **complete**, **queued**, **aborted**, **submitted** and **active**.
    The **suspended** state shown in the chart's colour legend is a
    persisted node flag rather than a logged transition, so it is never
    produced by the log parser itself; it only appears if a node's live
    state is inspected outside the Timeline. Any trailing information on
    the line (such as ``job_size:`` on a submitted task, or the try
    number and abort reason on an aborted task) is not used to draw the
    bars, although the presence of ``job_size:`` is used as a hint that
    the node is a **task** (see below).

    Each recognised event is appended, in file order, to the node's own
    sequence of ``(timestamp, state)`` pairs; a node is looked up by its
    path so that repeated state changes for the same node extend the
    existing sequence instead of creating duplicate rows. If a suite
    filter is active for the server, events whose top-level path segment
    is not in the filter are discarded before being stored.

    Because a node's type (suite, family or task) is not explicit in the
    log, it is inferred heuristically: a path with a single ``/`` is a
    suite; a line carrying a ``job_size:`` marker identifies its node as
    a task; any other node is left undetermined at parse time and is
    later resolved to a family if some task's path is found to contain
    it, or otherwise by matching the node against the live node tree
    once one is available.

    Only the tail of the log file is scanned, exactly as fetched from
    the server (see above); when the file had to be truncated to the
    configured maximum size, the earliest events may be missing from the
    resulting chart, and this is reported in the log file information
    label.

    All durations shown in the Duration view and in the node details
    dialog (first/mean submitted and active durations, and the minimum,
    maximum, median and quartile statistics per state) are derived
    purely from these stored ``(timestamp, state)`` sequences: the
    duration of a given occurrence of a state is simply the time until
    that node's *next* recorded transition, or, for the last recorded
    transition, the time until the end of the period covered by the
    timeline. No separate duration value is read from the log; every
    duration is computed by differencing consecutive event timestamps
    for the node.

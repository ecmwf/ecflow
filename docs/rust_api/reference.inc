.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

Client
------

.. _rust_client:

.. describe:: pub struct Client

   A client for an ecFlow server, wrapping the C++ ``ClientInvoker``.

   Every request is a blocking round trip. On connection failure the invoker
   retries, sleeping up to ten seconds between attempts by default, so async
   callers should run requests on a blocking thread.

   Dropping the client drops the handle of its most recent
   :ref:`Client::ch_register <rust_client_ch_register>`, if it still holds one; a failure of that request
   is ignored.

   .. rubric:: Example

   .. code-block:: rust

      let mut client = ecflow::Client::with_host_port("localhost", 3141)?;
      client.ping()?;
      println!("{}", client.server_version()?);


.. _rust_client_new:

.. describe:: pub fn new() -> Result<Self>

   Create a client configured from the environment
   (``ECF_HOST``, ``ECF_PORT``, ``ECF_SSL``, ...).

.. _rust_client_with_host_port:

.. describe:: pub fn with_host_port(host: &str, port: u16) -> Result<Self>

   Create a client for the given host and port.

Connection configuration
^^^^^^^^^^^^^^^^^^^^^^^^

.. _rust_client_set_host_port:

.. describe:: pub fn set_host_port(&mut self, host: &str, port: u16) -> Result<()>

   Override the host and port from the environment.

.. _rust_client_host:

.. describe:: pub fn host(&self) -> String

   The configured host.

.. _rust_client_port:

.. describe:: pub fn port(&self) -> String

   The configured port.

.. _rust_client_set_user_name:

.. describe:: pub fn set_user_name(&mut self, user: &str)

   Override the user name from ``ECF_USER``.

.. _rust_client_set_password:

.. describe:: pub fn set_password(&mut self, password: &str)

   Set the password for the user name.

.. _rust_client_enable_ssl:

.. describe:: pub fn enable_ssl(&mut self) -> Result<()>

   Use SSL. The certificate is ``server.crt``, or else
   ``<host>.<port>.crt``, in ``$HOME/.ecflowrc/ssl``; when ``ECF_SSL`` is set
   to anything but ``1``, only ``<host>.<port>.crt``. Fails when no
   certificate is found, or when built without the ``ssl`` feature.

.. _rust_client_disable_ssl:

.. describe:: pub fn disable_ssl(&mut self)

   Do not use SSL, whatever ``ECF_SSL`` says.

.. _rust_client_enable_http:

.. describe:: pub fn enable_http(&mut self)

   Use the HTTP transport.

.. _rust_client_enable_https:

.. describe:: pub fn enable_https(&mut self)

   Use the HTTPS transport.

.. _rust_client_set_connect_timeout:

.. describe:: pub fn set_connect_timeout(&mut self, timeout: Duration)

   Time to wait for a server reply before a request fails.

.. _rust_client_set_retry_connection_period:

.. describe:: pub fn set_retry_connection_period(&mut self, period: Duration)

   Time to wait between connection attempts.

.. _rust_client_set_connection_attempts:

.. describe:: pub fn set_connection_attempts(&mut self, attempts: u32)

   Number of connection attempts per host before giving up.

.. _rust_client_wait_for_server_reply:

.. describe:: pub fn wait_for_server_reply(&mut self, timeout: Duration) -> bool

   Whether the server answers a ping within the timeout, trying until
   it does.

.. _rust_client_certificate:

.. describe:: pub fn certificate(&self) -> Result<String>

   The path of the certificate used for SSL. Fails when built without
   the ``ssl`` feature.

.. _rust_client_set_debug:

.. describe:: pub fn set_debug(&mut self, enabled: bool)

   Print each request and its round trip time to standard output.

Server probes
^^^^^^^^^^^^^

.. _rust_client_ping:

.. describe:: pub fn ping(&mut self) -> Result<Duration>

   Check that the server answers, and return the round trip time.

.. _rust_client_server_version:

.. describe:: pub fn server_version(&mut self) -> Result<String>

   The server's version.

.. _rust_client_stats:

.. describe:: pub fn stats(&mut self) -> Result<String>

   The server's statistics, formatted by the server.

.. _rust_client_suites:

.. describe:: pub fn suites(&mut self) -> Result<Vec<String>>

   The names of the suites in the server.

Server control
^^^^^^^^^^^^^^

.. _rust_client_restart_server:

.. describe:: pub fn restart_server(&mut self) -> Result<()>

   Restart the server: it schedules jobs and accepts child commands again.

.. _rust_client_halt_server:

.. describe:: pub fn halt_server(&mut self) -> Result<()>

   Halt the server: no jobs are scheduled and child commands are blocked
   until a restart.

.. _rust_client_shutdown_server:

.. describe:: pub fn shutdown_server(&mut self) -> Result<()>

   Shut the server down: no jobs are scheduled, child commands still go
   through.

.. _rust_client_terminate_server:

.. describe:: pub fn terminate_server(&mut self) -> Result<()>

   Terminate the server process.

.. _rust_client_checkpt:

.. describe:: pub fn checkpt(&mut self) -> Result<()>

   Write the check point file now: the definition with node state,
   passwords and the rest, to ``ECF_HOME/<host>.<port>.ecf.check`` unless
   ``ECF_CHECK`` names another file.

.. _rust_client_configure_checkpt:

.. describe:: pub fn configure_checkpt(&mut self, mode: Option<CheckPtMode>, interval: Option<Duration>, save_time_alarm: Option<Duration>) -> Result<()>

   Change how the server writes its check point file: the mode, the
   interval between periodic writes, and the time a write may take
   before the server raises its late flag. ``None`` leaves a setting as it
   is; with nothing given this is :ref:`Client::checkpt <rust_client_checkpt>`.

.. _rust_client_restore_from_checkpt:

.. describe:: pub fn restore_from_checkpt(&mut self) -> Result<()>

   Load the check point file, ``ECF_HOME/ECF_CHECK`` or else
   ``ECF_HOME/ECF_CHECKOLD``. Fails unless the server is halted and holds
   no suites.

.. _rust_client_stats_reset:

.. describe:: pub fn stats_reset(&mut self) -> Result<()>

   Reset the server's statistics.

.. _rust_client_debug_server_on:

.. describe:: pub fn debug_server_on(&mut self) -> Result<()>

   Turn on debug output in the server.

.. _rust_client_debug_server_off:

.. describe:: pub fn debug_server_off(&mut self) -> Result<()>

   Turn off debug output in the server.

.. _rust_client_reload_wl_file:

.. describe:: pub fn reload_wl_file(&mut self) -> Result<()>

   Reload the white list file (``ECF_LISTS``), which controls who may read
   and who may write.

.. _rust_client_reload_passwd_file:

.. describe:: pub fn reload_passwd_file(&mut self) -> Result<()>

   Reload the password file (``ECF_PASSWD``).

.. _rust_client_reload_custom_passwd_file:

.. describe:: pub fn reload_custom_passwd_file(&mut self) -> Result<()>

   Reload the custom password file (``ECF_CUSTOM_PASSWD``).

Log
^^^

.. _rust_client_log_msg:

.. describe:: pub fn log_msg(&mut self, message: &str) -> Result<()>

   Write a message to the server's log.

.. _rust_client_new_log:

.. describe:: pub fn new_log(&mut self, path: &str) -> Result<()>

   Switch the server to a new log file. With an empty path, to the one
   the server's ``ECF_LOG`` variable names.

.. _rust_client_get_log:

.. describe:: pub fn get_log(&mut self, last_lines: Option<u32>) -> Result<String>

   The last lines of the server's log, 100 unless a count is given.

.. _rust_client_clear_log:

.. describe:: pub fn clear_log(&mut self) -> Result<()>

   Empty the server's log file.

.. _rust_client_flush_log:

.. describe:: pub fn flush_log(&mut self) -> Result<()>

   Flush and close the server's log file; the next command that logs
   reopens it.

.. _rust_client_get_log_path:

.. describe:: pub fn get_log_path(&mut self) -> Result<String>

   The path of the server's log file.

Nodes
^^^^^

.. _rust_client_delete:

.. describe:: pub fn delete<I, S>(&mut self, paths: I, force: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Delete the nodes at the given paths. With ``force``, even when they are
   active or submitted.

.. _rust_client_delete_all:

.. describe:: pub fn delete_all(&mut self, force: bool) -> Result<()>

   Delete every suite. With ``force``, even when nodes are active or
   submitted.

.. _rust_client_suspend:

.. describe:: pub fn suspend<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Suspend the nodes: no jobs are generated below them until resumed.

.. _rust_client_resume:

.. describe:: pub fn resume<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Resume suspended nodes.

.. _rust_client_requeue:

.. describe:: pub fn requeue<I, S>(&mut self, paths: I, option: &str) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Requeue the nodes and their children, back to the state before their
   last run. ``option`` is empty, ``abort`` to requeue only aborted tasks, or
   ``force`` to requeue even active or submitted ones.

.. _rust_client_run:

.. describe:: pub fn run<I, S>(&mut self, paths: I, force: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Run the nodes now, ignoring triggers, limits, suspension and time
   dependencies. With ``force``, even when they are active or submitted.

.. _rust_client_kill:

.. describe:: pub fn kill<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Kill the jobs of the nodes, with ``ECF_KILL_CMD``.

.. _rust_client_status:

.. describe:: pub fn status<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Query the status of the jobs of the nodes, with ``ECF_STATUS_CMD``.

.. _rust_client_check:

.. describe:: pub fn check<I, S>(&mut self, paths: I) -> Result<String> where I: IntoIterator<Item = S>, S: AsRef<str>

   Check the trigger and complete expressions and the limits below the
   nodes, and return the errors and warnings; empty when there are none.

.. _rust_client_archive:

.. describe:: pub fn archive<I, S>(&mut self, paths: I, force: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Save the nodes' children to disk and drop them from the definition;
   they come back when the node is requeued, begun or restored. With
   ``force``, even when they are active or submitted.

.. _rust_client_restore:

.. describe:: pub fn restore<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Load archived children back into the definition.

.. _rust_client_force_state:

.. describe:: pub fn force_state<I, S>(&mut self, paths: I, state: NodeState, recursive: bool, set_repeats_to_last_value: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Force the nodes to a state. With ``recursive``, the children too. With
   ``set_repeats_to_last_value``, repeats move to their last value first,
   so a forced complete does not requeue.

.. _rust_client_force_event:

.. describe:: pub fn force_event<I, S>(&mut self, paths: I, set: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Set or clear the events given as ``/node:event``.

.. _rust_client_free_trigger_dep:

.. describe:: pub fn free_trigger_dep<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Free the trigger dependencies of the nodes.

.. _rust_client_free_date_dep:

.. describe:: pub fn free_date_dep<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Free the date dependencies of the nodes.

.. _rust_client_free_time_dep:

.. describe:: pub fn free_time_dep<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Free the time dependencies of the nodes.

.. _rust_client_free_all_dep:

.. describe:: pub fn free_all_dep<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Free the trigger, date and time dependencies of the nodes.

.. _rust_client_order:

.. describe:: pub fn order(&mut self, path: &str, order: NodeOrder) -> Result<()>

   Move the node among its siblings, or sort them. The order decides
   which job is submitted first when nothing else does.

.. _rust_client_job_generation:

.. describe:: pub fn job_generation(&mut self, path: &str) -> Result<()>

   Generate and submit the jobs below the node whose dependencies are
   free, without waiting for the server's next poll.

.. _rust_client_begin_suite:

.. describe:: pub fn begin_suite(&mut self, suite: &str, force: bool) -> Result<()>

   Begin the suite, so that it is scheduled. With ``force``, even when it
   has active or submitted jobs, which then become zombies.

.. _rust_client_begin_all_suites:

.. describe:: pub fn begin_all_suites(&mut self, force: bool) -> Result<()>

   Begin every suite. With ``force``, even when one has active or
   submitted jobs, which then become zombies.

.. _rust_client_edit_history:

.. describe:: pub fn edit_history(&mut self, path: &str) -> Result<Vec<String>>

   The edit history of the node: the user commands applied to it, one
   line each.

.. _rust_client_get_file:

.. describe:: pub fn get_file(&mut self, path: &str, file_type: &str, max_lines: u32) -> Result<String>

   The last ``max_lines`` lines of a file of the node: its ``script``,
   ``job``, ``jobout`` (the job's output), ``manual``, or the output of its
   ``kill`` or ``stat`` command.

.. _rust_client_plug:

.. describe:: pub fn plug(&mut self, source_path: &str, destination_path: &str) -> Result<()>

   Move the node under another parent, which may be on another server
   when given as ``//host:port/path``.

.. _rust_client_group:

.. describe:: pub fn group(&mut self, commands: &str) -> Result<()>

   Run several commands as one request. ``commands`` holds them separated
   by ``;``, each written as an ``ecflow_client`` option without the
   leading ``--``, such as ``halt=yes; restart``. A command that asks for
   confirmation on the command line (``delete``, ``halt``, ``shutdown``,
   ``terminate``) reads the answer from standard input unless given
   ``yes``, and ``show`` and ``why`` write to standard output.

.. _rust_client_query:

.. describe:: pub fn query(&mut self, query_type: &str, path: &str, attribute: &str, evaluate: bool) -> Result<String>

   Query the node. ``query_type`` is ``state``, ``dstate``
   (the state with ``suspended``), ``repeat``, ``event``, ``meter``, ``label``,
   ``variable``, ``limit``, ``limit_max``, or ``trigger`` to evaluate an
   expression; ``attribute`` names the attribute or holds the expression.
   With ``evaluate``, for ``variable`` only, references in the value are
   resolved.

.. _rust_client_alter:

.. describe:: pub fn alter<I, S>(&mut self, paths: I, alter_type: &str, attr_type: &str, name: &str, value: &str) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Alter an attribute of the nodes. ``alter_type`` is ``add``, ``change``,
   ``delete``, ``set_flag`` or ``clear_flag``; ``attr_type`` the kind of
   attribute, such as ``variable``, ``event``, ``meter``, ``label``, ``limit``,
   ``trigger``, ``repeat`` or ``defstatus``; ``name`` and ``value`` what the kind
   needs, empty otherwise.

.. _rust_client_sort_attributes:

.. describe:: pub fn sort_attributes<I, S>(&mut self, paths: I, attribute: &str, recursive: bool) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Sort the attributes of a kind (``event``, ``meter``, ``label``, ``variable``,
   ``limit`` or ``all``) of the nodes by name. With ``recursive``, below them
   too.

Zombies
^^^^^^^

.. _rust_client_zombie_fob:

.. describe:: pub fn zombie_fob<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Let the child commands of the zombies at these paths succeed, so
   their jobs go on without the server recording anything.

.. _rust_client_zombie_fail:

.. describe:: pub fn zombie_fail<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Make the child commands of the zombies at these paths fail, so their
   jobs abort.

.. _rust_client_zombie_adopt:

.. describe:: pub fn zombie_adopt<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Accept the passwords the zombies at these paths carry: the tasks take
   their jobs on.

.. _rust_client_zombie_block:

.. describe:: pub fn zombie_block<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Make the child commands of the zombies at these paths block until
   their timeout.

.. _rust_client_zombie_remove:

.. describe:: pub fn zombie_remove<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Drop the zombies at these paths from the server's list; they come
   back if their child commands call again.

.. _rust_client_zombie_kill:

.. describe:: pub fn zombie_kill<I, S>(&mut self, paths: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Kill the jobs of the zombies at these paths with ``ECF_KILL_CMD``.

.. _rust_client_zombie_get:

.. describe:: pub fn zombie_get(&mut self) -> Result<Vec<Zombie>>

   The zombies the server knows about.

Scripts
^^^^^^^

.. _rust_client_edit_script_edit:

.. describe:: pub fn edit_script_edit(&mut self, path: &str) -> Result<String>

   The task's script for editing, with the variables it uses listed in
   a leading comment block.

.. _rust_client_edit_script_preprocess:

.. describe:: pub fn edit_script_preprocess<I, S>(&mut self, path: &str, file_contents: I) -> Result<String> where I: IntoIterator<Item = S>, S: AsRef<str>

   Without lines, the task's script with its ``%include``\ s expanded and
   the variables it uses listed at the top, still unsubstituted. With
   lines, those instead, fully pre-processed: includes expanded,
   variables substituted, comment and manual sections removed.

.. _rust_client_edit_script_submit:

.. describe:: pub fn edit_script_submit<V, N, W, I, S>(&mut self, path: &str, used_variables: V, file_contents: I, alias: bool, run: bool) -> Result<()> where V: IntoIterator<Item = (N, W)>, N: AsRef<str>, W: AsRef<str>, I: IntoIterator<Item = S>, S: AsRef<str>

   Submit the task at once with a job made from the given lines and these
   values for the variables it uses. With ``alias``, the task is left alone
   and an alias of it is made from the lines instead, run at once only
   when ``run``.

Client handles
^^^^^^^^^^^^^^

.. _rust_client_ch_register:

.. describe:: pub fn ch_register<I, S>(&mut self, auto_add_new_suites: bool, suites: I) -> Result<i32> where I: IntoIterator<Item = S>, S: AsRef<str>

   Register interest in the suites, so that the server sends only those
   on a sync, and return the handle. With ``auto_add_new_suites``, suites
   added later are included. The server keeps the handle until it is
   dropped; the client drops the most recent one when it goes out of
   scope.

.. _rust_client_ch_handle:

.. describe:: pub fn ch_handle(&self) -> i32

   The handle of the most recent registration, 0 without one.

.. _rust_client_ch_suites:

.. describe:: pub fn ch_suites(&mut self) -> Result<Vec<(i32, Vec<String>)>>

   The registered handles, each with the names of its suites.

.. _rust_client_ch_drop:

.. describe:: pub fn ch_drop(&mut self, handle: i32) -> Result<()>

   Drop the handle.

.. _rust_client_ch_drop_user:

.. describe:: pub fn ch_drop_user(&mut self, user: &str) -> Result<()>

   Drop every handle of the user, the client's own when empty.

.. _rust_client_ch_add:

.. describe:: pub fn ch_add<I, S>(&mut self, handle: i32, suites: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Add suites to the handle.

.. _rust_client_ch_remove:

.. describe:: pub fn ch_remove<I, S>(&mut self, handle: i32, suites: I) -> Result<()> where I: IntoIterator<Item = S>, S: AsRef<str>

   Remove suites from the handle.

.. _rust_client_ch_auto_add:

.. describe:: pub fn ch_auto_add(&mut self, handle: i32, auto_add_new_suites: bool) -> Result<()>

   Whether suites added later join the handle.

Child (task) commands
^^^^^^^^^^^^^^^^^^^^^

.. _rust_client_set_child_path:

.. describe:: pub fn set_child_path(&mut self, path: &str)

   The task path the child commands report for (``ECF_NAME``).

.. _rust_client_set_child_password:

.. describe:: pub fn set_child_password(&mut self, password: &str)

   The job password the child commands carry (``ECF_PASS``).

.. _rust_client_set_child_pid:

.. describe:: pub fn set_child_pid(&mut self, pid: &str)

   The process or remote id the child commands carry (``ECF_RID``).

.. _rust_client_set_child_try_no:

.. describe:: pub fn set_child_try_no(&mut self, try_no: u32)

   The try number the child commands carry (``ECF_TRYNO``).

.. _rust_client_set_child_init_add_vars:

.. describe:: pub fn set_child_init_add_vars<V, N, W>(&mut self, vars: V) where V: IntoIterator<Item = (N, W)>, N: AsRef<str>, W: AsRef<str>

   The variables the child init command adds to the task, as name and
   value pairs.

.. _rust_client_set_child_complete_del_vars:

.. describe:: pub fn set_child_complete_del_vars<I, S>(&mut self, names: I) where I: IntoIterator<Item = S>, S: AsRef<str>

   The names of the variables the child complete command removes from
   the task.

.. _rust_client_set_child_timeout:

.. describe:: pub fn set_child_timeout(&mut self, timeout: Duration)

   How long a child command keeps trying to reach the server (``ECF_TIMEOUT``).

.. _rust_client_set_zombie_child_timeout:

.. describe:: pub fn set_zombie_child_timeout(&mut self, timeout: Duration)

   How long a child command keeps trying when the server reports it as a
   zombie (``ECF_ZOMBIE_TIMEOUT``).

.. _rust_client_child_init:

.. describe:: pub fn child_init(&mut self) -> Result<()>

   Report that the job started.

.. _rust_client_child_abort:

.. describe:: pub fn child_abort(&mut self, reason: &str) -> Result<()>

   Report that the job failed.

.. _rust_client_child_event:

.. describe:: pub fn child_event(&mut self, name: &str, value: bool) -> Result<()>

   Set or clear an event of the task.

.. _rust_client_child_meter:

.. describe:: pub fn child_meter(&mut self, name: &str, value: i32) -> Result<()>

   Set a meter of the task.

.. _rust_client_child_label:

.. describe:: pub fn child_label(&mut self, name: &str, value: &str) -> Result<()>

   Set a label of the task.

.. _rust_client_child_wait:

.. describe:: pub fn child_wait(&mut self, expression: &str) -> Result<()>

   Block until the expression holds on the server.

.. _rust_client_child_queue:

.. describe:: pub fn child_queue(&mut self, queue: &str, action: &str, step: &str, path: &str) -> Result<String>

   Act on a queue of the task or of an ancestor, and return the step the
   server handed out.

.. _rust_client_child_complete:

.. describe:: pub fn child_complete(&mut self) -> Result<()>

   Report that the job finished.

Definitions
^^^^^^^^^^^

.. _rust_client_get_server_defs:

.. describe:: pub fn get_server_defs(&mut self) -> Result<()>

   Fetch the whole definition from the server and hold it in the client;
   :ref:`Client::defs_text <rust_client_defs_text>` writes it.

.. _rust_client_sync_local:

.. describe:: pub fn sync_local(&mut self, sync_suite_clock: bool) -> Result<()>

   Bring the held definition up to date with the server's changes since
   the last fetch or sync, or fetch it whole when the client holds none.
   With ``sync_suite_clock``, the suite clocks are updated too.

.. _rust_client_in_sync:

.. describe:: pub fn in_sync(&self) -> bool

   Whether the held definition is in sync with the server, as of the
   last :ref:`Client::sync_local <rust_client_sync_local>`.

.. _rust_client_changed_node_paths:

.. describe:: pub fn changed_node_paths(&self) -> Vec<String>

   The paths of the nodes the last :ref:`Client::sync_local <rust_client_sync_local>` changed.

.. _rust_client_news_local:

.. describe:: pub fn news_local(&mut self) -> Result<bool>

   Whether the server has changes the held definition lacks. Always
   true until the first :ref:`Client::sync_local <rust_client_sync_local>`.

.. _rust_client_reset:

.. describe:: pub fn reset(&mut self)

   Drop the held definition and the client handle.

.. _rust_client_set_auto_sync:

.. describe:: pub fn set_auto_sync(&mut self, enabled: bool)

   Sync the held definition after every command that changes the
   server's definition.

.. _rust_client_is_auto_sync_enabled:

.. describe:: pub fn is_auto_sync_enabled(&self) -> bool

   Whether the held definition is synced after every command that
   changes the server's definition.

.. _rust_client_defs_text:

.. describe:: pub fn defs_text(&self, style: DefsStyle) -> Result<String>

   The held definition as text, as of the last fetch or sync.

.. _rust_client_get_defs_text:

.. describe:: pub fn get_defs_text(&mut self, style: DefsStyle) -> Result<String>

   Fetch the whole definition from the server and return it as text.

.. _rust_client_load_defs_text:

.. describe:: pub fn load_defs_text(&mut self, defs: &str, force: bool) -> Result<()>

   Load definitions given as text into the server. With ``force``, suites
   of the same name are replaced.

.. _rust_client_load_defs_file:

.. describe:: pub fn load_defs_file(&mut self, file: &str, force: bool) -> Result<()>

   Load a definition file into the server. With ``force``, suites of the
   same name are replaced.

.. _rust_client_replace_text:

.. describe:: pub fn replace_text(&mut self, path: &str, defs: &str, create_parents: bool, force: bool) -> Result<()>

   Replace the node at ``path`` with the node of that path in the
   definitions given as text.

.. _rust_client_replace_file:

.. describe:: pub fn replace_file(&mut self, path: &str, file: &str, create_parents: bool, force: bool) -> Result<()>

   Replace the node at ``path`` with the node of that path in the
   definition file.

DefsStyle
---------

.. _rust_defsstyle:

.. describe:: pub enum DefsStyle

   Print style of a definition returned as text.

.. _rust_defsstyle_defs:

.. describe:: Defs

   The definition alone, without node state.

.. _rust_defsstyle_state:

.. describe:: State

   The definition with node state and trigger expressions.

.. _rust_defsstyle_migrate:

.. describe:: Migrate

   The definition with node state, as a server loads it.

NodeOrder
---------

.. _rust_nodeorder:

.. describe:: pub enum NodeOrder

   Where a node moves among its siblings, or how they are sorted.

.. _rust_nodeorder_top:

.. describe:: Top

   First among its siblings.

.. _rust_nodeorder_bottom:

.. describe:: Bottom

   Last among its siblings.

.. _rust_nodeorder_alpha:

.. describe:: Alpha

   The siblings sorted alphabetically.

.. _rust_nodeorder_order:

.. describe:: Order

   The siblings sorted in reverse alphabetical order.

.. _rust_nodeorder_up:

.. describe:: Up

   One place up.

.. _rust_nodeorder_down:

.. describe:: Down

   One place down.

.. _rust_nodeorder_runtime:

.. describe:: Runtime

   The siblings sorted by how long their last run took, longest first.

CheckPtMode
-----------

.. _rust_checkptmode:

.. describe:: pub enum CheckPtMode

   When the server writes its check point file.

.. _rust_checkptmode_never:

.. describe:: Never

   Never.

.. _rust_checkptmode_ontime:

.. describe:: OnTime

   Periodically, at the check point interval.

.. _rust_checkptmode_always:

.. describe:: Always

   After every state change.

NodeState
---------

.. _rust_nodestate:

.. describe:: pub enum NodeState

   The state of a node.

.. _rust_nodestate_unknown:

.. describe:: Unknown

   Not yet begun.

.. _rust_nodestate_complete:

.. describe:: Complete

   Finished.

.. _rust_nodestate_queued:

.. describe:: Queued

   Waiting for its dependencies.

.. _rust_nodestate_aborted:

.. describe:: Aborted

   Failed.

.. _rust_nodestate_submitted:

.. describe:: Submitted

   Handed to the job submission command.

.. _rust_nodestate_active:

.. describe:: Active

   Running.

version
-------

.. _rust_version:

.. describe:: pub fn version() -> String

   The ecFlow version the client library was built from.

ssl_supported
-------------

.. _rust_ssl_supported:

.. describe:: pub fn ssl_supported() -> bool

   Whether the client library was built with OpenSSL support.

Failure
-------

.. _rust_failure:

.. describe:: pub enum Failure

   The class of failure ecFlow observed while talking to the server.

   Mirrors ``ecf::ConnectionFailure``.

.. _rust_failure_none:

.. describe:: None

   No transport failure was observed: the server answered, refusing the
   request, or the error arose before a request was sent, such as an
   invalid definition or a missing setting.

.. _rust_failure_hostresolution:

.. describe:: HostResolution

   The host name could not be resolved.

.. _rust_failure_connectionrefused:

.. describe:: ConnectionRefused

   No listener accepted the connection.

.. _rust_failure_timeout:

.. describe:: Timeout

   The peer accepted the connection but did not reply in time.

.. _rust_failure_closedwithoutreply:

.. describe:: ClosedWithoutReply

   The peer accepted the connection and then closed it without replying.

.. _rust_failure_handshakefailed:

.. describe:: HandshakeFailed

   The TLS handshake did not complete.

.. _rust_failure_certificaterejected:

.. describe:: CertificateRejected

   The TLS handshake failed while verifying the peer certificate.

.. _rust_failure_undecodablereply:

.. describe:: UndecodableReply

   A reply was received that the transport cannot decode.

.. _rust_failure_rejectedrequest:

.. describe:: RejectedRequest

   The peer answered, refusing the request.

.. _rust_failure_other:

.. describe:: Other

   A failure that none of the other values describes.

Error
-----

.. _rust_error:

.. describe:: pub struct Error

   An error from the ecFlow client.

.. _rust_error_failure:

.. describe:: pub const fn failure(&self) -> Failure

   The class of failure ecFlow observed.

.. _rust_error_message:

.. describe:: pub fn message(&self) -> &str

   The message ecFlow reported.

Result
------

.. _rust_result:

.. describe:: pub type Result<T> = std::result::Result<T, Error>

   Result type alias for ecFlow client operations.

Zombie
------

.. _rust_zombie:

.. describe:: pub struct Zombie

   A zombie: a job whose child commands the server does not match to its
   task, because the task was requeued, the password or process differ,
   or the path is gone.

.. _rust_zombie_path:

.. describe:: path: String

   The path of the task.

.. _rust_zombie_kind:

.. describe:: kind: String

   The kind: ``user``, ``ecf``, ``path``, ``ecf_pid``, ``ecf_passwd`` or ``ecf_pid_passwd``.

.. _rust_zombie_process_or_remote_id:

.. describe:: process_or_remote_id: String

   The process or remote id the job reported.

.. _rust_zombie_password:

.. describe:: password: String

   The job password the job reported.

.. _rust_zombie_host:

.. describe:: host: String

   The host the job runs on.

.. _rust_zombie_try_no:

.. describe:: try_no: i32

   The try number the job reported.

.. _rust_zombie_last_child_cmd:

.. describe:: last_child_cmd: String

   The last child command the job sent.

.. _rust_zombie_calls:

.. describe:: calls: i32

   How many times its child commands have called.

.. _rust_zombie_duration:

.. describe:: duration: i32

   Seconds since the zombie was first seen.

.. _rust_zombie_allowed_age:

.. describe:: allowed_age: i32

   Seconds after which the server drops it.

.. _rust_zombie_user_action:

.. describe:: user_action: String

   What the server does with its child commands: ``fob``, ``fail``, ``adopt``, ``block``, ``remove`` or ``kill``.

.. _rust_zombie_manual_user_action:

.. describe:: manual_user_action: bool

   Whether a user chose that action, rather than the task's zombie attribute.

.. _rust_zombie_user_cmd:

.. describe:: user_cmd: String

   The user command that created it, when one did.

.. _rust_zombie_explanation:

.. describe:: explanation: String

   Why the server thinks it is a zombie.

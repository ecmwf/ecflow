// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! The ecFlow client.

use std::time::Duration;

use ecflow_sys::{Exception, NameValue, Zombie, let_cxx_string};

use crate::error::{Error, Failure, Result};

/// Print style of a definition returned as text.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DefsStyle {
    /// The definition alone, without node state.
    Defs,
    /// The definition with node state and trigger expressions.
    State,
    /// The definition with node state, as a server loads it.
    #[default]
    Migrate,
}

impl From<DefsStyle> for ecflow_sys::DefsStyle {
    fn from(style: DefsStyle) -> Self {
        match style {
            DefsStyle::Defs => Self::Defs,
            DefsStyle::State => Self::State,
            DefsStyle::Migrate => Self::Migrate,
        }
    }
}

/// Where a node moves among its siblings, or how they are sorted.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NodeOrder {
    /// First among its siblings.
    Top,
    /// Last among its siblings.
    Bottom,
    /// The siblings sorted alphabetically.
    Alpha,
    /// The siblings sorted in reverse alphabetical order.
    Order,
    /// One place up.
    Up,
    /// One place down.
    Down,
    /// The siblings sorted by the time of their last state change.
    Runtime,
}

impl From<NodeOrder> for ecflow_sys::NodeOrder {
    fn from(order: NodeOrder) -> Self {
        match order {
            NodeOrder::Top => Self::Top,
            NodeOrder::Bottom => Self::Bottom,
            NodeOrder::Alpha => Self::Alpha,
            NodeOrder::Order => Self::Order,
            NodeOrder::Up => Self::Up,
            NodeOrder::Down => Self::Down,
            NodeOrder::Runtime => Self::Runtime,
        }
    }
}

/// When the server writes its check point file.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CheckPtMode {
    /// Never.
    Never,
    /// Periodically, at the check point interval.
    OnTime,
    /// After every state change.
    Always,
}

impl From<CheckPtMode> for ecflow_sys::CheckPtMode {
    fn from(mode: CheckPtMode) -> Self {
        match mode {
            CheckPtMode::Never => Self::Never,
            CheckPtMode::OnTime => Self::OnTime,
            CheckPtMode::Always => Self::Always,
        }
    }
}

/// The state of a node.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NodeState {
    /// Not yet begun.
    Unknown,
    /// Finished.
    Complete,
    /// Waiting for its dependencies.
    Queued,
    /// Failed.
    Aborted,
    /// Handed to the job submission command.
    Submitted,
    /// Running.
    Active,
}

impl From<NodeState> for ecflow_sys::NodeState {
    fn from(state: NodeState) -> Self {
        match state {
            NodeState::Unknown => Self::Unknown,
            NodeState::Complete => Self::Complete,
            NodeState::Queued => Self::Queued,
            NodeState::Aborted => Self::Aborted,
            NodeState::Submitted => Self::Submitted,
            NodeState::Active => Self::Active,
        }
    }
}

/// A client for an ecFlow server, wrapping the C++ `ClientInvoker`.
///
/// Every request is a blocking round trip. On connection failure the invoker
/// retries, sleeping up to ten seconds between attempts by default, so async
/// callers should run requests on a blocking thread.
///
/// # Example
///
/// ```no_run
/// let mut client = ecflow::Client::with_host_port("localhost", 3141)?;
/// client.ping()?;
/// println!("{}", client.server_version()?);
/// # Ok::<(), ecflow::Error>(())
/// ```
pub struct Client {
    inner: ecflow_sys::UniquePtr<ecflow_sys::Client>,
}

// SAFETY: the C++ `ClientInvoker` owns no thread-affine state; each request
// creates its own I/O context. It is not reentrant, hence no `Sync`.
#[allow(clippy::non_send_fields_in_send_ty)]
unsafe impl Send for Client {}

impl Client {
    /// Create a client configured from the environment
    /// (`ECF_HOST`, `ECF_PORT`, `ECF_SSL`, ...).
    pub fn new() -> Result<Self> {
        let inner = ecflow_sys::Client::create()?;
        Ok(Self { inner })
    }

    /// Create a client for the given host and port.
    pub fn with_host_port(host: &str, port: u16) -> Result<Self> {
        let inner = ecflow_sys::Client::from_host_port(host, &port.to_string())?;
        Ok(Self { inner })
    }

    /// Attach the failure class ecFlow diagnosed for the request that just threw.
    fn diagnose<T>(&self, result: std::result::Result<T, Exception>) -> Result<T> {
        result.map_err(|e| Error::new(Failure::from(self.inner.last_failure()), e.what()))
    }

    // ==================== Connection configuration ====================

    /// Override the host and port from the environment.
    pub fn set_host_port(&mut self, host: &str, port: u16) -> Result<()> {
        let_cxx_string!(host = host);
        let_cxx_string!(port = port.to_string());
        let result = self.inner.pin_mut().set_host_port(&host, &port);
        self.diagnose(result)
    }

    /// The configured host.
    #[must_use]
    pub fn host(&self) -> String {
        self.inner.host().to_string()
    }

    /// The configured port.
    #[must_use]
    pub fn port(&self) -> String {
        self.inner.port().to_string()
    }

    /// Override the user name from `ECF_USER`.
    pub fn set_user_name(&mut self, user: &str) {
        let_cxx_string!(user = user);
        self.inner.pin_mut().set_user_name(&user);
    }

    /// Set the password for the user name.
    pub fn set_password(&mut self, password: &str) {
        let_cxx_string!(password = password);
        self.inner.pin_mut().set_password(&password);
    }

    /// Use SSL, whatever `ECF_SSL` says. Fails when built without the `ssl`
    /// feature.
    pub fn enable_ssl(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().enable_ssl();
        self.diagnose(result)
    }

    /// Do not use SSL, whatever `ECF_SSL` says.
    pub fn disable_ssl(&mut self) {
        self.inner.pin_mut().disable_ssl();
    }

    /// Use the HTTP transport.
    pub fn enable_http(&mut self) {
        self.inner.pin_mut().enable_http();
    }

    /// Use the HTTPS transport.
    pub fn enable_https(&mut self) {
        self.inner.pin_mut().enable_https();
    }

    /// Time to wait for a server reply before a request fails.
    pub fn set_connect_timeout(&mut self, timeout: Duration) {
        self.inner.pin_mut().set_connect_timeout(millis(timeout));
    }

    /// Time to wait between connection attempts.
    pub fn set_retry_connection_period(&mut self, period: Duration) {
        self.inner
            .pin_mut()
            .set_retry_connection_period(millis(period));
    }

    /// Number of connection attempts per host before giving up.
    pub fn set_connection_attempts(&mut self, attempts: u32) {
        self.inner.pin_mut().set_connection_attempts(attempts);
    }

    /// Whether the server answers a ping within the timeout, trying until
    /// it does.
    pub fn wait_for_server_reply(&mut self, timeout: Duration) -> bool {
        let seconds = i32::try_from(timeout.as_secs()).unwrap_or(i32::MAX);
        self.inner.wait_for_server_reply(seconds)
    }

    /// The path of the certificate used for SSL. Fails when built without
    /// the `ssl` feature.
    pub fn get_certificate(&self) -> Result<String> {
        self.diagnose(self.inner.get_certificate())
    }

    /// Print each request and its round trip time to standard output.
    pub fn set_debug(&mut self, enabled: bool) {
        self.inner.pin_mut().debug(enabled);
    }

    // ==================== Server probes ====================

    /// Check that the server answers, and return the round trip time.
    pub fn ping(&mut self) -> Result<Duration> {
        self.diagnose(self.inner.pingServer())?;
        Ok(Duration::from_micros(self.inner.round_trip_time()))
    }

    /// The server's version.
    pub fn server_version(&mut self) -> Result<String> {
        self.diagnose(self.inner.server_version())?;
        Ok(self.inner.get_string().to_string())
    }

    /// The server's statistics, formatted by the server.
    pub fn stats(&mut self) -> Result<String> {
        self.diagnose(self.inner.stats())?;
        Ok(self.inner.get_string().to_string())
    }

    /// The names of the suites in the server.
    pub fn suites(&mut self) -> Result<Vec<String>> {
        self.diagnose(self.inner.suites())?;
        Ok(self.inner.reply_strings())
    }

    // ==================== Server control ====================

    /// Restart the server: it schedules jobs and accepts child commands again.
    pub fn restart_server(&mut self) -> Result<()> {
        self.diagnose(self.inner.restartServer())?;
        Ok(())
    }

    /// Halt the server: no jobs are scheduled and child commands are blocked
    /// until a restart.
    pub fn halt_server(&mut self) -> Result<()> {
        self.diagnose(self.inner.haltServer())?;
        Ok(())
    }

    /// Shut the server down: no jobs are scheduled, child commands still go
    /// through.
    pub fn shutdown_server(&mut self) -> Result<()> {
        self.diagnose(self.inner.shutdownServer())?;
        Ok(())
    }

    /// Terminate the server process.
    pub fn terminate_server(&mut self) -> Result<()> {
        self.diagnose(self.inner.terminateServer())?;
        Ok(())
    }

    /// Write the check point file now: the definition with node state,
    /// passwords and the rest, to `ECF_HOME/<host>.<port>.ecf.check` unless
    /// `ECF_CHECK` names another file.
    pub fn checkpt(&mut self) -> Result<()> {
        self.configure_checkpt(None, None, None)
    }

    /// Change how the server writes its check point file: the mode, the
    /// interval between periodic writes, and the time a write may take
    /// before the server raises its late flag. `None` leaves a setting as it
    /// is; with nothing given this is [`Client::checkpt`].
    pub fn configure_checkpt(
        &mut self,
        mode: Option<CheckPtMode>,
        interval: Option<Duration>,
        save_time_alarm: Option<Duration>,
    ) -> Result<()> {
        let seconds = |duration: Duration| i32::try_from(duration.as_secs()).unwrap_or(i32::MAX);
        self.diagnose(self.inner.checkPtDefs(
            mode.map_or(ecflow_sys::CheckPtMode::Undefined, Into::into),
            interval.map_or(0, seconds),
            save_time_alarm.map_or(0, seconds),
        ))?;
        Ok(())
    }

    /// Load the check point file, `ECF_HOME/ECF_CHECK` or else
    /// `ECF_HOME/ECF_CHECKOLD`. Fails unless the server is halted and holds
    /// no suites.
    pub fn restore_from_checkpt(&mut self) -> Result<()> {
        self.diagnose(self.inner.restoreDefsFromCheckPt())?;
        Ok(())
    }

    /// Reset the server's statistics.
    pub fn stats_reset(&mut self) -> Result<()> {
        self.diagnose(self.inner.stats_reset())?;
        Ok(())
    }

    /// Turn on debug output in the server.
    pub fn debug_server_on(&mut self) -> Result<()> {
        self.diagnose(self.inner.debug_server_on())?;
        Ok(())
    }

    /// Turn off debug output in the server.
    pub fn debug_server_off(&mut self) -> Result<()> {
        self.diagnose(self.inner.debug_server_off())?;
        Ok(())
    }

    /// Reload the white list file (`ECF_LISTS`), which controls who may read
    /// and who may write.
    pub fn reload_wl_file(&mut self) -> Result<()> {
        self.diagnose(self.inner.reloadwsfile())?;
        Ok(())
    }

    /// Reload the password file (`ECF_PASSWD`).
    pub fn reload_passwd_file(&mut self) -> Result<()> {
        self.diagnose(self.inner.reloadpasswdfile())?;
        Ok(())
    }

    /// Reload the custom password file (`ECF_CUSTOM_PASSWD`).
    pub fn reload_custom_passwd_file(&mut self) -> Result<()> {
        self.diagnose(self.inner.reloadcustompasswdfile())?;
        Ok(())
    }

    // ==================== Log ====================

    /// Write a message to the server's log.
    pub fn log_msg(&mut self, message: &str) -> Result<()> {
        let_cxx_string!(message = message);
        self.diagnose(self.inner.logMsg(&message))?;
        Ok(())
    }

    /// Switch the server to a new log file. With an empty path, the current
    /// one is reopened.
    pub fn new_log(&mut self, path: &str) -> Result<()> {
        let_cxx_string!(path = path);
        self.diagnose(self.inner.new_log(&path))?;
        Ok(())
    }

    /// The last lines of the server's log, 100 unless a count is given.
    pub fn get_log(&mut self, last_lines: Option<u32>) -> Result<String> {
        let last_lines = last_lines.map_or(0, |n| i32::try_from(n).unwrap_or(i32::MAX));
        self.diagnose(self.inner.getLog(last_lines))?;
        Ok(self.inner.get_string().to_string())
    }

    /// Empty the server's log file.
    pub fn clear_log(&mut self) -> Result<()> {
        self.diagnose(self.inner.clearLog())?;
        Ok(())
    }

    /// Flush and close the server's log file; the next command that logs
    /// reopens it.
    pub fn flush_log(&mut self) -> Result<()> {
        self.diagnose(self.inner.flushLog())?;
        Ok(())
    }

    /// The path of the server's log file.
    pub fn get_log_path(&mut self) -> Result<String> {
        self.diagnose(self.inner.get_log_path())?;
        Ok(self.inner.get_string().to_string())
    }

    // ==================== Nodes ====================

    /// Delete the nodes at the given paths. With `force`, even when they are
    /// active or submitted.
    pub fn delete<I, S>(&mut self, paths: I, force: bool) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.delete_nodes(&strings(paths), force))
    }

    /// Delete every suite. With `force`, even when nodes are active or
    /// submitted.
    pub fn delete_all(&mut self, force: bool) -> Result<()> {
        self.diagnose(self.inner.delete_all(force))?;
        Ok(())
    }

    /// Suspend the nodes: no jobs are generated below them until resumed.
    pub fn suspend<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.suspend(&strings(paths)))
    }

    /// Resume suspended nodes.
    pub fn resume<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.resume(&strings(paths)))
    }

    /// Requeue the nodes and their children, back to the state before their
    /// last run. `option` is empty, `abort` to requeue only aborted tasks, or
    /// `force` to requeue even active or submitted ones.
    pub fn requeue<I, S>(&mut self, paths: I, option: &str) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(option = option);
        self.diagnose(self.inner.requeue(&strings(paths), &option))
    }

    /// Run the nodes now, ignoring triggers, limits, suspension and time
    /// dependencies. With `force`, even when they are active or submitted.
    pub fn run<I, S>(&mut self, paths: I, force: bool) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.run(&strings(paths), force))
    }

    /// Kill the jobs of the nodes, with `ECF_KILL_CMD`.
    pub fn kill<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.kill(&strings(paths)))
    }

    /// Query the status of the jobs of the nodes, with `ECF_STATUS_CMD`.
    pub fn status<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.status(&strings(paths)))
    }

    /// Check the trigger and complete expressions and the limits below the
    /// nodes, and return the errors and warnings; empty when there are none.
    pub fn check<I, S>(&mut self, paths: I) -> Result<String>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.check(&strings(paths)))?;
        Ok(self.inner.get_string().to_string())
    }

    /// Save the nodes' children to disk and drop them from the definition;
    /// they come back when the node is requeued, begun or restored. With
    /// `force`, even when they are active or submitted.
    pub fn archive<I, S>(&mut self, paths: I, force: bool) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.archive(&strings(paths), force))
    }

    /// Load archived children back into the definition.
    pub fn restore<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.restore(&strings(paths)))
    }

    /// Force the nodes to a state. With `recursive`, the children too. With
    /// `set_repeats_to_last_value`, repeats move to their last value first,
    /// so a forced complete does not requeue.
    pub fn force_state<I, S>(
        &mut self,
        paths: I,
        state: NodeState,
        recursive: bool,
        set_repeats_to_last_value: bool,
    ) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.force(
            &strings(paths),
            state.into(),
            recursive,
            set_repeats_to_last_value,
        ))
    }

    /// Set or clear the events given as `/node:event`.
    pub fn force_event<I, S>(&mut self, paths: I, set: bool) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.force_event(&strings(paths), set))
    }

    /// Free the trigger dependencies of the nodes.
    pub fn free_trigger_dep<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(
            self.inner
                .freeDep(&strings(paths), true, false, false, false),
        )
    }

    /// Free the date dependencies of the nodes.
    pub fn free_date_dep<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(
            self.inner
                .freeDep(&strings(paths), false, false, true, false),
        )
    }

    /// Free the time dependencies of the nodes.
    pub fn free_time_dep<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(
            self.inner
                .freeDep(&strings(paths), false, false, false, true),
        )
    }

    /// Free the trigger, date and time dependencies of the nodes.
    pub fn free_all_dep<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(
            self.inner
                .freeDep(&strings(paths), false, true, false, false),
        )
    }

    /// Move the node among its siblings, or sort them. The order decides
    /// which job is submitted first when nothing else does.
    pub fn order(&mut self, path: &str, order: NodeOrder) -> Result<()> {
        let_cxx_string!(path = path);
        self.diagnose(self.inner.order(&path, order.into()))?;
        Ok(())
    }

    /// Generate and submit the jobs below the node whose dependencies are
    /// free, without waiting for the server's next poll.
    pub fn job_generation(&mut self, path: &str) -> Result<()> {
        let_cxx_string!(path = path);
        self.diagnose(self.inner.job_gen(&path))?;
        Ok(())
    }

    /// Begin the suite, so that it is scheduled. With `force`, even when it
    /// has active or submitted jobs, which then become zombies.
    pub fn begin_suite(&mut self, suite: &str, force: bool) -> Result<()> {
        let_cxx_string!(suite = suite);
        self.diagnose(self.inner.begin(&suite, force))?;
        Ok(())
    }

    /// Begin every suite. With `force`, even when one has active or
    /// submitted jobs, which then become zombies.
    pub fn begin_all_suites(&mut self, force: bool) -> Result<()> {
        self.diagnose(self.inner.begin_all_suites(force))?;
        Ok(())
    }

    /// The edit history of the node: the user commands applied to it, one
    /// line each.
    pub fn edit_history(&mut self, path: &str) -> Result<Vec<String>> {
        let_cxx_string!(path = path);
        self.diagnose(self.inner.edit_history(&path))?;
        Ok(self.inner.reply_strings())
    }

    /// The last `max_lines` lines of a file of the node: its `script`,
    /// `job`, `jobout` (the job's output), `manual`, or the output of its
    /// `kill` or `stat` command.
    pub fn get_file(&mut self, path: &str, file_type: &str, max_lines: u32) -> Result<String> {
        let_cxx_string!(path = path);
        let_cxx_string!(file_type = file_type);
        let_cxx_string!(max_lines = max_lines.to_string());
        self.diagnose(self.inner.file(&path, &file_type, &max_lines))?;
        Ok(self.inner.get_string().to_string())
    }

    /// Move the node under another parent, which may be on another server
    /// when given as `//host:port/path`.
    pub fn plug(&mut self, source_path: &str, destination_path: &str) -> Result<()> {
        let_cxx_string!(source_path = source_path);
        let_cxx_string!(destination_path = destination_path);
        self.diagnose(self.inner.plug(&source_path, &destination_path))?;
        Ok(())
    }

    /// Query the node without blocking. `query_type` is `state`, `dstate`
    /// (the state with `suspended`), `repeat`, `event`, `meter`, `label`,
    /// `variable`, `limit`, `limit_max`, or `trigger` to evaluate an
    /// expression; `attribute` names the attribute or holds the expression.
    /// With `evaluate`, for `variable` only, references in the value are
    /// resolved.
    pub fn query(
        &mut self,
        query_type: &str,
        path: &str,
        attribute: &str,
        evaluate: bool,
    ) -> Result<String> {
        let_cxx_string!(query_type = query_type);
        let_cxx_string!(path = path);
        let_cxx_string!(attribute = attribute);
        let result = self
            .inner
            .pin_mut()
            .query(&query_type, &path, &attribute, evaluate);
        self.diagnose(result)?;
        Ok(self.inner.get_string().to_string())
    }

    /// Alter an attribute of the nodes. `alter_type` is `add`, `change`,
    /// `delete`, `set_flag` or `clear_flag`; `attr_type` the kind of
    /// attribute, such as `variable`, `event`, `meter`, `label`, `limit`,
    /// `trigger`, `repeat` or `defstatus`; `name` and `value` what the kind
    /// needs, empty otherwise.
    pub fn alter<I, S>(
        &mut self,
        paths: I,
        alter_type: &str,
        attr_type: &str,
        name: &str,
        value: &str,
    ) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(alter_type = alter_type);
        let_cxx_string!(attr_type = attr_type);
        let_cxx_string!(name = name);
        let_cxx_string!(value = value);
        self.diagnose(
            self.inner
                .alter(&strings(paths), &alter_type, &attr_type, &name, &value),
        )
    }

    /// Sort the attributes of a kind (`event`, `meter`, `label`, `variable`,
    /// `limit` or `all`) of the nodes by name. With `recursive`, below them
    /// too.
    pub fn sort_attributes<I, S>(
        &mut self,
        paths: I,
        attribute: &str,
        recursive: bool,
    ) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(attribute = attribute);
        self.diagnose(
            self.inner
                .alter_sort(&strings(paths), &attribute, recursive),
        )
    }

    // ==================== Zombies ====================

    /// Let the child commands of the zombies at these paths succeed, so
    /// their jobs go on without the server recording anything.
    pub fn zombie_fob<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieFobCliPaths(&strings(paths)))
    }

    /// Make the child commands of the zombies at these paths fail, so their
    /// jobs abort.
    pub fn zombie_fail<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieFailCliPaths(&strings(paths)))
    }

    /// Accept the passwords the zombies at these paths carry: the tasks take
    /// their jobs on.
    pub fn zombie_adopt<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieAdoptCliPaths(&strings(paths)))
    }

    /// Make the child commands of the zombies at these paths block until
    /// their timeout.
    pub fn zombie_block<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieBlockCliPaths(&strings(paths)))
    }

    /// Drop the zombies at these paths from the server's list; they come
    /// back if their child commands call again.
    pub fn zombie_remove<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieRemoveCliPaths(&strings(paths)))
    }

    /// Kill the jobs of the zombies at these paths with `ECF_KILL_CMD`.
    pub fn zombie_kill<I, S>(&mut self, paths: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.zombieKillCliPaths(&strings(paths)))
    }

    /// The zombies the server knows about.
    pub fn zombie_get(&mut self) -> Result<Vec<Zombie>> {
        self.diagnose(self.inner.zombieGet())?;
        Ok(self.inner.zombies())
    }

    // ==================== Scripts ====================

    /// The task's script for editing, with the variables it uses listed in
    /// a leading comment block.
    pub fn edit_script_edit(&mut self, path: &str) -> Result<String> {
        let_cxx_string!(path = path);
        let result = self.inner.pin_mut().edit_script_edit(&path);
        self.diagnose(result)?;
        Ok(self.inner.get_string().to_string())
    }

    /// Without lines, the task's script with its `%include`s expanded and
    /// the variables it uses listed at the top, still unsubstituted. With
    /// lines, those instead, fully pre-processed: includes expanded,
    /// variables substituted, comment and manual sections removed.
    pub fn edit_script_preprocess<I, S>(&mut self, path: &str, file_contents: I) -> Result<String>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(path = path);
        let result = self
            .inner
            .pin_mut()
            .edit_script_preprocess(&path, &strings(file_contents));
        self.diagnose(result)?;
        Ok(self.inner.get_string().to_string())
    }

    /// Submit the task at once with a job made from the given lines and these
    /// values for the variables it uses. With `alias`, the task is left alone
    /// and an alias of it is made from the lines instead, run at once only
    /// when `run`.
    pub fn edit_script_submit<V, N, W, I, S>(
        &mut self,
        path: &str,
        used_variables: V,
        file_contents: I,
        alias: bool,
        run: bool,
    ) -> Result<()>
    where
        V: IntoIterator<Item = (N, W)>,
        N: AsRef<str>,
        W: AsRef<str>,
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(path = path);
        let used_variables: Vec<NameValue> = used_variables
            .into_iter()
            .map(|(name, value)| NameValue {
                name: name.as_ref().to_owned(),
                value: value.as_ref().to_owned(),
            })
            .collect();
        let result = self.inner.pin_mut().edit_script_submit(
            &path,
            &used_variables,
            &strings(file_contents),
            alias,
            run,
        );
        self.diagnose(result)
    }

    // ==================== Client handles ====================

    /// Register interest in the suites, so that the server sends only those
    /// on a sync, and return the handle. With `auto_add_new_suites`, suites
    /// added later are included.
    pub fn ch_register<I, S>(&mut self, auto_add_new_suites: bool, suites: I) -> Result<i32>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(
            self.inner
                .ch_register(auto_add_new_suites, &strings(suites)),
        )?;
        Ok(self.ch_handle())
    }

    /// The handle of the most recent registration, 0 without one.
    #[must_use]
    pub fn ch_handle(&self) -> i32 {
        self.inner.client_handle()
    }

    /// The registered handles, each with the names of its suites.
    pub fn ch_suites(&mut self) -> Result<Vec<(i32, Vec<String>)>> {
        self.diagnose(self.inner.ch_suites())?;
        Ok(self
            .inner
            .client_handle_suites()
            .into_iter()
            .map(|item| (item.handle, item.suites))
            .collect())
    }

    /// Drop the handle.
    pub fn ch_drop(&mut self, handle: i32) -> Result<()> {
        self.diagnose(self.inner.ch_drop(handle))?;
        Ok(())
    }

    /// Drop every handle of the user, the client's own when empty.
    pub fn ch_drop_user(&mut self, user: &str) -> Result<()> {
        let_cxx_string!(user = user);
        self.diagnose(self.inner.ch_drop_user(&user))?;
        Ok(())
    }

    /// Add suites to the handle.
    pub fn ch_add<I, S>(&mut self, handle: i32, suites: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.ch_add(handle, &strings(suites)))
    }

    /// Remove suites from the handle.
    pub fn ch_remove<I, S>(&mut self, handle: i32, suites: I) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.diagnose(self.inner.ch_remove(handle, &strings(suites)))
    }

    /// Whether suites added later join the handle.
    pub fn ch_auto_add(&mut self, handle: i32, auto_add_new_suites: bool) -> Result<()> {
        self.diagnose(self.inner.ch_auto_add(handle, auto_add_new_suites))?;
        Ok(())
    }

    // ==================== Child (task) commands ====================

    /// The task path the child commands report for (`ECF_NAME`).
    pub fn set_child_path(&mut self, path: &str) {
        let_cxx_string!(path = path);
        self.inner.pin_mut().set_child_path(&path);
    }

    /// The job password the child commands carry (`ECF_PASS`).
    pub fn set_child_password(&mut self, password: &str) {
        let_cxx_string!(password = password);
        self.inner.pin_mut().set_child_password(&password);
    }

    /// The process or remote id the child commands carry (`ECF_RID`).
    pub fn set_child_pid(&mut self, pid: &str) {
        let_cxx_string!(pid = pid);
        self.inner.pin_mut().set_child_pid(&pid);
    }

    /// The try number the child commands carry (`ECF_TRYNO`).
    pub fn set_child_try_no(&mut self, try_no: u32) {
        self.inner.pin_mut().set_child_try_no(try_no);
    }

    /// The variables the child init command adds to the task, as name and
    /// value pairs.
    pub fn set_child_init_add_vars<V, N, W>(&mut self, vars: V)
    where
        V: IntoIterator<Item = (N, W)>,
        N: AsRef<str>,
        W: AsRef<str>,
    {
        let vars: Vec<NameValue> = vars
            .into_iter()
            .map(|(name, value)| NameValue {
                name: name.as_ref().to_owned(),
                value: value.as_ref().to_owned(),
            })
            .collect();
        self.inner.pin_mut().set_child_init_add_vars(&vars);
    }

    /// The names of the variables the child complete command removes from
    /// the task.
    pub fn set_child_complete_del_vars<I, S>(&mut self, names: I)
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self.inner
            .pin_mut()
            .set_child_complete_del_vars(&strings(names));
    }

    /// How long a child command keeps trying to reach the server (`ECF_TIMEOUT`).
    pub fn set_child_timeout(&mut self, timeout: Duration) {
        self.inner.pin_mut().set_child_timeout(secs(timeout));
    }

    /// How long a child command keeps trying when the server reports it as a
    /// zombie (`ECF_ZOMBIE_TIMEOUT`).
    pub fn set_zombie_child_timeout(&mut self, timeout: Duration) {
        self.inner.pin_mut().set_zombie_child_timeout(secs(timeout));
    }

    /// Report that the job started.
    pub fn child_init(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().child_init();
        self.diagnose(result)
    }

    /// Report that the job failed.
    pub fn child_abort(&mut self, reason: &str) -> Result<()> {
        let_cxx_string!(reason = reason);
        let result = self.inner.pin_mut().child_abort(&reason);
        self.diagnose(result)
    }

    /// Set or clear an event of the task.
    pub fn child_event(&mut self, name: &str, value: bool) -> Result<()> {
        let_cxx_string!(name = name);
        let result = self.inner.pin_mut().child_event(&name, value);
        self.diagnose(result)
    }

    /// Set a meter of the task.
    pub fn child_meter(&mut self, name: &str, value: i32) -> Result<()> {
        let_cxx_string!(name = name);
        let result = self.inner.pin_mut().child_meter(&name, value);
        self.diagnose(result)
    }

    /// Set a label of the task.
    pub fn child_label(&mut self, name: &str, value: &str) -> Result<()> {
        let_cxx_string!(name = name);
        let_cxx_string!(value = value);
        let result = self.inner.pin_mut().child_label(&name, &value);
        self.diagnose(result)
    }

    /// Block until the expression holds on the server.
    pub fn child_wait(&mut self, expression: &str) -> Result<()> {
        let_cxx_string!(expression = expression);
        let result = self.inner.pin_mut().child_wait(&expression);
        self.diagnose(result)
    }

    /// Act on a queue of the task or of an ancestor, and return the step the
    /// server handed out.
    pub fn child_queue(
        &mut self,
        queue: &str,
        action: &str,
        step: &str,
        path: &str,
    ) -> Result<String> {
        let_cxx_string!(queue = queue);
        let_cxx_string!(action = action);
        let_cxx_string!(step = step);
        let_cxx_string!(path = path);
        let result = self
            .inner
            .pin_mut()
            .child_queue(&queue, &action, &step, &path);
        self.diagnose(result)
    }

    /// Report that the job finished.
    pub fn child_complete(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().child_complete();
        self.diagnose(result)
    }

    // ==================== Definitions ====================

    /// Fetch the whole definition from the server and hold it in the client;
    /// [`Client::defs_text`] writes it.
    pub fn get_server_defs(&mut self) -> Result<()> {
        self.diagnose(self.inner.getDefs())?;
        Ok(())
    }

    /// Bring the held definition up to date with the server's changes since
    /// the last fetch or sync, or fetch it whole when the client holds none.
    /// With `sync_suite_clock`, the suite clocks are updated too.
    pub fn sync_local(&mut self, sync_suite_clock: bool) -> Result<()> {
        self.diagnose(self.inner.sync_local(sync_suite_clock))?;
        Ok(())
    }

    /// Whether the held definition is in sync with the server, as of the
    /// last [`Client::sync_local`].
    #[must_use]
    pub fn in_sync(&self) -> bool {
        self.inner.in_sync()
    }

    /// The paths of the nodes the last [`Client::sync_local`] changed.
    #[must_use]
    pub fn changed_node_paths(&self) -> Vec<String> {
        self.inner
            .changed_node_paths()
            .iter()
            .map(ToString::to_string)
            .collect()
    }

    /// Whether the server has changes the held definition lacks. Always
    /// true until the first [`Client::sync_local`].
    pub fn news_local(&mut self) -> Result<bool> {
        self.diagnose(self.inner.news_local())?;
        Ok(self.inner.get_news())
    }

    /// Drop the held definition and the client handle.
    pub fn reset(&mut self) {
        self.inner.reset();
    }

    /// Sync the held definition after every command.
    pub fn set_auto_sync(&mut self, enabled: bool) {
        self.inner.pin_mut().set_auto_sync(enabled);
    }

    /// Whether the held definition is synced after every command.
    #[must_use]
    pub fn is_auto_sync_enabled(&self) -> bool {
        self.inner.is_auto_sync_enabled()
    }

    /// The held definition as text, as of the last fetch or sync.
    pub fn defs_text(&self, style: DefsStyle) -> Result<String> {
        self.diagnose(self.inner.defs_text(style.into()))
    }

    /// Fetch the whole definition from the server and return it as text.
    pub fn get_defs_text(&mut self, style: DefsStyle) -> Result<String> {
        self.get_server_defs()?;
        self.defs_text(style)
    }

    /// Load definitions given as text into the server. With `force`, suites
    /// of the same name are replaced.
    pub fn load_defs_text(&mut self, defs: &str, force: bool) -> Result<()> {
        let defs = ecflow_sys::Client::parse_defs(defs)?;
        self.diagnose(self.inner.load(&defs, force))?;
        Ok(())
    }

    /// Load a definition file into the server. With `force`, suites of the
    /// same name are replaced.
    pub fn load_defs_file(&mut self, file: &str, force: bool) -> Result<()> {
        let_cxx_string!(file = file);
        self.diagnose(self.inner.loadDefs(&file, force, false, false, false))?;
        Ok(())
    }

    /// Replace the node at `path` with the node of that path in the
    /// definitions given as text.
    pub fn replace_text(
        &mut self,
        path: &str,
        defs: &str,
        create_parents: bool,
        force: bool,
    ) -> Result<()> {
        let defs = ecflow_sys::Client::parse_defs(defs)?;
        let_cxx_string!(path = path);
        self.diagnose(self.inner.replace_1(&path, &defs, create_parents, force))?;
        Ok(())
    }

    /// Replace the node at `path` with the node of that path in the
    /// definition file.
    pub fn replace_file(
        &mut self,
        path: &str,
        file: &str,
        create_parents: bool,
        force: bool,
    ) -> Result<()> {
        let_cxx_string!(path = path);
        let_cxx_string!(file = file);
        self.diagnose(self.inner.replace(&path, &file, create_parents, force))?;
        Ok(())
    }
}

/// The ecFlow version the client library was built from.
#[must_use]
pub fn version() -> String {
    ecflow_sys::Client::version()
}

/// Whether the client library was built with OpenSSL support.
#[must_use]
pub fn ssl_supported() -> bool {
    ecflow_sys::Client::ssl_supported()
}

/// The owned strings the bridge takes for a list of paths.
fn strings<I, S>(items: I) -> Vec<String>
where
    I: IntoIterator<Item = S>,
    S: AsRef<str>,
{
    items.into_iter().map(|s| s.as_ref().to_owned()).collect()
}

fn millis(duration: Duration) -> u64 {
    u64::try_from(duration.as_millis()).unwrap_or(u64::MAX)
}

fn secs(duration: Duration) -> u32 {
    u32::try_from(duration.as_secs()).unwrap_or(u32::MAX)
}

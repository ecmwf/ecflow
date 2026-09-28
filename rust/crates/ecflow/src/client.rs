// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! The ecFlow client.

use std::time::Duration;

use ecflow_sys::{Exception, let_cxx_string};

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

    // ==================== Nodes ====================

    /// Delete the nodes at the given paths. With `force`, even when they are
    /// active or submitted.
    pub fn delete_nodes<I, S>(&mut self, paths: I, force: bool) -> Result<()>
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

    /// Force the nodes to a state (`complete`, `aborted`, `queued`, `active`,
    /// `submitted` or `unknown`), or, for paths of the form `/node:event`,
    /// `set` or `clear` the event. With `recursive`, the children too. With
    /// `set_repeats_to_last_value`, repeats move to their last value first, so
    /// a forced complete does not requeue.
    pub fn force<I, S>(
        &mut self,
        paths: I,
        state_or_event: &str,
        recursive: bool,
        set_repeats_to_last_value: bool,
    ) -> Result<()>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let_cxx_string!(state_or_event = state_or_event);
        self.diagnose(self.inner.force(
            &strings(paths),
            &state_or_event,
            recursive,
            set_repeats_to_last_value,
        ))
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
    pub fn job_gen(&mut self, path: &str) -> Result<()> {
        let_cxx_string!(path = path);
        self.diagnose(self.inner.job_gen(&path))?;
        Ok(())
    }

    /// Begin the suite, so that it is scheduled. With `force`, even when it
    /// has active or submitted jobs, which then become zombies.
    pub fn begin(&mut self, suite: &str, force: bool) -> Result<()> {
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

    // ==================== Definitions as text ====================

    /// Fetch the server's definitions as text.
    pub fn get_defs_text(&mut self, style: DefsStyle) -> Result<String> {
        self.diagnose(self.inner.defs_text(style.into()))
    }

    /// Load definitions given as text into the server. With `force`, suites
    /// of the same name are replaced.
    pub fn load_defs_text(&mut self, defs: &str, force: bool) -> Result<()> {
        let defs = ecflow_sys::Client::parse_defs(defs)?;
        self.diagnose(self.inner.load(&defs, force))?;
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

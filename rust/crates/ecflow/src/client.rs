// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! The ecFlow client.

use std::time::Duration;

use ecflow_sys::ClientWrapper;

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

/// A client for an ecFlow server, wrapping the C++ `ClientInvoker`.
///
/// Every request is a blocking round trip. On connection failure the invoker
/// retries, sleeping up to ten seconds between attempts by default, so async
/// callers should run requests on a blocking thread.
///
/// Commands without a typed method go through [`Client::invoke`].
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
    inner: ecflow_sys::UniquePtr<ClientWrapper>,
}

// SAFETY: the C++ `ClientInvoker` owns no thread-affine state; each request
// creates its own I/O context. It is not reentrant, hence no `Sync`.
#[allow(clippy::non_send_fields_in_send_ty)]
unsafe impl Send for Client {}

impl Client {
    /// Create a client configured from the environment
    /// (`ECF_HOST`, `ECF_PORT`, `ECF_SSL`, ...).
    pub fn new() -> Result<Self> {
        let inner = ClientWrapper::create()?;
        Ok(Self { inner })
    }

    /// Create a client for the given host and port.
    pub fn with_host_port(host: &str, port: u16) -> Result<Self> {
        let inner = ClientWrapper::from_host_port(host, &port.to_string())?;
        Ok(Self { inner })
    }

    /// Attach the failure class of the request that just threw.
    fn check<T>(&self, result: std::result::Result<T, ecflow_sys::Exception>) -> Result<T> {
        result.map_err(|e| Error::new(Failure::from(self.inner.last_failure()), e.what()))
    }

    // ==================== Connection configuration ====================

    /// Override the host and port from the environment.
    pub fn set_host_port(&mut self, host: &str, port: u16) -> Result<()> {
        let result = self.inner.pin_mut().set_host_port(host, &port.to_string());
        self.check(result)
    }

    /// The configured host.
    #[must_use]
    pub fn host(&self) -> String {
        self.inner.host()
    }

    /// The configured port.
    #[must_use]
    pub fn port(&self) -> String {
        self.inner.port()
    }

    /// Override the user name from `ECF_USER`.
    pub fn set_user_name(&mut self, user: &str) {
        self.inner.pin_mut().set_user_name(user);
    }

    /// Set the password for the user name.
    pub fn set_password(&mut self, password: &str) {
        self.inner.pin_mut().set_password(password);
    }

    /// Use SSL, whatever `ECF_SSL` says.
    pub fn enable_ssl(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().enable_ssl();
        self.check(result)
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

    /// Check that the server answers.
    pub fn ping(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().ping_server();
        self.check(result)
    }

    /// The server's version.
    pub fn server_version(&mut self) -> Result<String> {
        let result = self.inner.pin_mut().server_version();
        self.check(result)
    }

    /// The server's statistics, formatted by the server.
    pub fn stats(&mut self) -> Result<String> {
        let result = self.inner.pin_mut().stats();
        self.check(result)
    }

    // ==================== Any command ====================

    /// Run a command given as `ecflow_client` command line arguments, and
    /// return the string reply, if the command has one.
    ///
    /// ```no_run
    /// # let mut client = ecflow::Client::with_host_port("localhost", 3141)?;
    /// client.invoke(["--suspend=/suite/family"])?;
    /// let log = client.invoke(["--log=get", "20"])?;
    /// # Ok::<(), ecflow::Error>(())
    /// ```
    pub fn invoke<I, S>(&mut self, args: I) -> Result<String>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let args: Vec<String> = args.into_iter().map(|a| a.as_ref().to_owned()).collect();
        let result = self.inner.pin_mut().invoke(&args);
        self.check(result)
    }

    /// The list of strings in the most recent reply, for commands such as
    /// `--suites` that return one.
    #[must_use]
    pub fn reply_strings(&self) -> Vec<String> {
        self.inner.reply_strings()
    }

    // ==================== Child (task) commands ====================

    /// The task path the child commands report for (`ECF_NAME`).
    pub fn set_child_path(&mut self, path: &str) {
        self.inner.pin_mut().set_child_path(path);
    }

    /// The job password the child commands carry (`ECF_PASS`).
    pub fn set_child_password(&mut self, password: &str) {
        self.inner.pin_mut().set_child_password(password);
    }

    /// The process or remote id the child commands carry (`ECF_RID`).
    pub fn set_child_pid(&mut self, pid: &str) {
        self.inner.pin_mut().set_child_pid(pid);
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
        self.check(result)
    }

    /// Report that the job failed.
    pub fn child_abort(&mut self, reason: &str) -> Result<()> {
        let result = self.inner.pin_mut().child_abort(reason);
        self.check(result)
    }

    /// Set or clear an event of the task.
    pub fn child_event(&mut self, name: &str, value: bool) -> Result<()> {
        let result = self.inner.pin_mut().child_event(name, value);
        self.check(result)
    }

    /// Set a meter of the task.
    pub fn child_meter(&mut self, name: &str, value: i32) -> Result<()> {
        let result = self.inner.pin_mut().child_meter(name, value);
        self.check(result)
    }

    /// Set a label of the task.
    pub fn child_label(&mut self, name: &str, value: &str) -> Result<()> {
        let result = self.inner.pin_mut().child_label(name, value);
        self.check(result)
    }

    /// Block until the expression holds on the server.
    pub fn child_wait(&mut self, expression: &str) -> Result<()> {
        let result = self.inner.pin_mut().child_wait(expression);
        self.check(result)
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
        let result = self.inner.pin_mut().child_queue(queue, action, step, path);
        self.check(result)
    }

    /// Report that the job finished.
    pub fn child_complete(&mut self) -> Result<()> {
        let result = self.inner.pin_mut().child_complete();
        self.check(result)
    }

    // ==================== Definitions as text ====================

    /// Fetch the server's definitions as text.
    pub fn get_defs_text(&mut self, style: DefsStyle) -> Result<String> {
        let result = self.inner.pin_mut().get_defs_text(style.into());
        self.check(result)
    }

    /// Load definitions given as text into the server. With `force`, suites
    /// of the same name are replaced.
    pub fn load_defs_text(&mut self, defs: &str, force: bool) -> Result<()> {
        let result = self.inner.pin_mut().load_defs_text(defs, force);
        self.check(result)
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
        let result = self
            .inner
            .pin_mut()
            .replace_text(path, defs, create_parents, force);
        self.check(result)
    }
}

/// The ecFlow version the client library was built from.
#[must_use]
pub fn version() -> String {
    ecflow_sys::version()
}

/// Whether the client library was built with OpenSSL support.
#[must_use]
pub fn ssl_supported() -> bool {
    ecflow_sys::ssl_supported()
}

fn millis(duration: Duration) -> u64 {
    u64::try_from(duration.as_millis()).unwrap_or(u64::MAX)
}

fn secs(duration: Duration) -> u32 {
    u32::try_from(duration.as_secs()).unwrap_or(u32::MAX)
}

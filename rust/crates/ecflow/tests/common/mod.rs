// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Harness for tests against a real server.
//!
//! The server is the executable named by `ECFLOW_SERVER`. Without it, the
//! tests are skipped, unless `ECFLOW_TEST_REQUIRE_SERVER` is set, in which
//! case they fail.
//!
//! Ports are reserved with the scheme of the other test suites: a lock file
//! `<port>.lock` in the lock directory (`ECF_PORT_LOCK_DIR`, else the source
//! directory of ecFlow), followed by a bind probe; the lock is held until the
//! server is stopped.
//!
//! [`PermissionsServer`] runs a server in a temporary directory of its own,
//! from a configuration file `<host>.<port>.ecf.cfg` holding the given server
//! Permissions attribute, if any. Every identity is listed, with its password,
//! in both password files of the server, which therefore authenticate no other
//! user. Clients read their passwords from a client-side password file, named
//! by `ECF_CUSTOM_PASSWD` while the server runs; since the environment belongs
//! to the process, the servers of a test binary run one at a time, and the
//! environment is restored when each stops.

#![allow(dead_code)]

use std::ffi::OsString;
use std::net::TcpListener;
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::{Mutex, MutexGuard};
use std::time::{Duration, Instant};

use ecflow::{Client, DefsStyle};

/// The variables read by servers and clients, cleared while a server runs.
const CLEARED: &[&str] = &[
    "ECF_HOME",
    "ECF_PORT",
    "ECF_LOG",
    "ECF_CHECK",
    "ECF_CHECKOLD",
    "ECF_PASSWD",
    "ECF_CUSTOM_PASSWD",
    "ECF_LISTS",
    "ECF_SSL",
    "ECF_USER",
    "ECF_HOST",
    "ECF_HOST_PROTOCOL",
    "ECF_PASSWORD",
    "ECF_TIMEOUT",
    "ECF_CONNECT_TIMEOUT",
];

/// The range of ports reserved for the live-server tests of the crate.
pub const PORTS: std::ops::Range<u16> = 14000..15000;

/// Serialises the servers of a test binary, which share the environment.
static ONE_AT_A_TIME: Mutex<()> = Mutex::new(());

/// Returns the executable to test, or `None` when the tests are to be
/// skipped; panics when the server is required but not given.
pub fn server_executable() -> Option<OsString> {
    match std::env::var_os("ECFLOW_SERVER") {
        Some(executable) => Some(executable),
        None => {
            assert!(
                std::env::var_os("ECFLOW_TEST_REQUIRE_SERVER").is_none(),
                "ECFLOW_TEST_REQUIRE_SERVER is set, but ECFLOW_SERVER names no ecflow_server executable"
            );
            eprintln!("skipped: set ECFLOW_SERVER to an ecflow_server executable");
            None
        }
    }
}

/// A port reserved through its lock file, released when dropped.
pub struct PortLock {
    port: u16,
    file: PathBuf,
}

impl PortLock {
    /// Reserves the first free port of the range: one whose lock file can be
    /// created, and which can then be bound.
    pub fn reserve(ports: std::ops::Range<u16>) -> Self {
        let directory = lock_directory();
        for port in ports.clone() {
            let file = directory.join(format!("{port}.lock"));
            if std::fs::OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(&file)
                .is_err()
            {
                continue;
            }
            let lock = Self { port, file };
            if TcpListener::bind(("0.0.0.0", port)).is_ok() {
                return lock;
            }
        }
        panic!("no free port in {ports:?}");
    }

    pub fn port(&self) -> u16 {
        self.port
    }
}

impl Drop for PortLock {
    fn drop(&mut self) {
        let _ = std::fs::remove_file(&self.file);
    }
}

/// The lock directory shared with the other test suites.
pub fn lock_directory() -> PathBuf {
    match std::env::var_os("ECF_PORT_LOCK_DIR") {
        Some(directory) => PathBuf::from(directory),
        None => Path::new(env!("CARGO_MANIFEST_DIR")).join("../../.."),
    }
}

/// Clears the variables of [`CLEARED`], and restores their values when
/// dropped.
struct Environment {
    saved: Vec<(&'static str, Option<OsString>)>,
}

impl Environment {
    fn clear() -> Self {
        let saved = CLEARED
            .iter()
            .map(|name| (*name, std::env::var_os(name)))
            .collect();
        for name in CLEARED {
            // SAFETY: the servers of the binary run one at a time, under the lock
            unsafe { std::env::remove_var(name) };
        }
        Self { saved }
    }

    fn set(&self, name: &str, value: impl AsRef<std::ffi::OsStr>) {
        // SAFETY: as above
        unsafe { std::env::set_var(name, value) };
    }
}

impl Drop for Environment {
    fn drop(&mut self) {
        for (name, value) in &self.saved {
            // SAFETY: as above
            unsafe {
                match value {
                    Some(value) => std::env::set_var(name, value),
                    None => std::env::remove_var(name),
                }
            }
        }
    }
}

/// An identity: a user name and its password.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Identity {
    pub user: &'static str,
    pub password: &'static str,
}

/// A server configured for tests of Permissions attributes.
///
/// Fields are dropped in order: the process is stopped first (by `Drop`),
/// then the directory removed, the environment restored, the port released
/// and the lock of the binary released.
pub struct PermissionsServer {
    process: Option<Child>,
    owned: bool,
    administrator: Identity,
    directory: tempfile::TempDir,
    environment: Environment,
    port: PortLock,
    _guard: MutexGuard<'static, ()>,
}

impl PermissionsServer {
    /// Starts a server, or returns `None` when the tests are skipped.
    ///
    /// `permissions` is the JSON form of the server attribute, if any. Panics
    /// when the server does not answer, or another server answers on its
    /// port; the process started is then killed, and the environment and the
    /// port restored.
    pub fn start(
        administrator: Identity,
        users: &[Identity],
        permissions: Option<&str>,
    ) -> Option<Self> {
        let executable = server_executable()?;
        let guard = ONE_AT_A_TIME
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let environment = Environment::clear();
        let port = PortLock::reserve(PORTS);
        let directory = tempfile::tempdir().expect("temporary directory");

        // Every file is written before the process starts
        let number = port.port();
        let home = directory.path().join("home");
        std::fs::create_dir(&home).expect("server home");
        let host = host_name();
        let mut identities = vec![administrator];
        identities.extend_from_slice(users);
        for name in ["ecf.passwd", "ecf.custom_passwd"] {
            write_passwords(
                &home.join(format!("{host}.{number}.{name}")),
                &identities,
                number,
                &host,
            );
        }
        let client_passwords = directory.path().join("client.custom_passwd");
        write_passwords(&client_passwords, &identities, number, &host);
        let permissions = permissions
            .map(|p| format!("\"permissions\": {p}, "))
            .unwrap_or_default();
        std::fs::write(
            directory.path().join(format!("{host}.{number}.ecf.cfg")),
            format!(
                "{{\"version\": 1, \"ecflow\": {{\"server\": {{{permissions}\"variables\": \
                 {{\"ECF_HOME\": \"home\", \"ECF_LOG\": \"server.log\", \"ECF_CHECKMODE\": \"CHECK_NEVER\"}}}}}}}}"
            ),
        )
        .expect("configuration file");

        let process = Command::new(executable)
            .arg(format!("--port={number}"))
            .current_dir(directory.path())
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()
            .expect("start ecflow_server");

        // From here on, Drop stops the process (killing it until it is known to be own)
        let mut server = Self {
            process: Some(process),
            owned: false,
            administrator,
            directory,
            environment,
            port,
            _guard: guard,
        };
        server
            .environment
            .set("ECF_CUSTOM_PASSWD", &client_passwords);
        server.wait_for_own_server();
        server.owned = true;
        Some(server)
    }

    /// A client issuing requests as the given user, with the password the
    /// harness lists for that user; a user not listed is refused.
    pub fn client(&self, user: &str) -> Client {
        let mut client = Client::with_host_port("localhost", self.port()).expect("create client");
        client.set_connect_timeout(Duration::from_secs(5));
        client.set_connection_attempts(1);
        client.set_user_name(user);
        client
    }

    /// A client issuing requests under the administrator's identity.
    pub fn administrator(&self) -> Client {
        self.client(self.administrator.user)
    }

    /// Replaces the identity used to stop the server, for tests of the harness
    /// itself.
    pub fn set_administrator(&mut self, administrator: Identity) {
        self.administrator = administrator;
    }

    /// The port of the server.
    pub fn port(&self) -> u16 {
        self.port.port()
    }

    /// The home of the server, which holds its log.
    pub fn home(&self) -> PathBuf {
        self.directory.path().join("home")
    }

    /// Stops the server: terminated under the administrator's identity when
    /// it is known to be the own one and its process still runs (so that a
    /// server started since on the port is never sent a request), and killed
    /// otherwise, or when it does not exit within 30 s. Returns whether it
    /// terminated on request.
    pub fn stop(&mut self) -> bool {
        let Some(mut process) = self.process.take() else {
            return false;
        };
        let running = matches!(process.try_wait(), Ok(None));
        let requested = running && self.owned && self.administrator().terminate_server().is_ok();
        if requested {
            let deadline = Instant::now() + Duration::from_secs(30);
            while Instant::now() < deadline {
                if let Ok(Some(_)) = process.try_wait() {
                    return true;
                }
                std::thread::sleep(Duration::from_millis(100));
            }
        }
        let _ = process.kill();
        let _ = process.wait();
        false
    }

    fn wait_for_own_server(&mut self) {
        let mut probe = self.administrator();
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            if let Some(process) = self.process.as_mut() {
                if let Ok(Some(status)) = process.try_wait() {
                    panic!("server on port {} exited: {status}", self.port());
                }
            }
            if probe.ping().is_ok() {
                break;
            }
            assert!(
                Instant::now() < deadline,
                "server did not answer on port {} within 30 s",
                self.port()
            );
            std::thread::sleep(Duration::from_millis(200));
        }
        // Only the server started has this home, and knows the administrator
        let text = probe
            .get_defs_text(DefsStyle::Migrate)
            .expect("definitions of the server");
        let home = self.home().canonicalize().expect("home");
        assert!(
            text.lines()
                .any(|line| line.contains("ECF_HOME") && line.contains(home.to_str().unwrap())),
            "another server answers on port {}",
            self.port()
        );
    }
}

impl Drop for PermissionsServer {
    fn drop(&mut self) {
        self.stop();
    }
}

/// The host name, as the server names its files.
fn host_name() -> String {
    let output = Command::new("hostname").output().expect("hostname");
    String::from_utf8(output.stdout)
        .expect("host name")
        .trim()
        .to_string()
}

fn write_passwords(path: &Path, identities: &[Identity], port: u16, host: &str) {
    let mut text = String::from("4.5.0\n");
    for identity in identities {
        for h in [host, "localhost"] {
            text.push_str(&format!(
                "{} {h} {port} {}\n",
                identity.user, identity.password
            ));
        }
    }
    std::fs::write(path, text).expect("password file");
}

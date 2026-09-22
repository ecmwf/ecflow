// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Round trip against a real server.
//!
//! Runs when `ECFLOW_SERVER` names an `ecflow_server` executable; the server
//! is started on a free port with a temporary `ECF_HOME` and stopped at the
//! end.

use std::net::TcpListener;
use std::process::{Child, Command, Stdio};
use std::time::{Duration, Instant};

use ecflow::{Client, DefsStyle, Failure};

struct Server {
    process: Child,
    port: u16,
    _home: tempfile::TempDir,
}

impl Server {
    fn start() -> Option<Self> {
        let executable = std::env::var_os("ECFLOW_SERVER")?;
        let home = tempfile::tempdir().expect("temporary ECF_HOME");
        let port = free_port();
        let process = Command::new(executable)
            .arg(format!("--port={port}"))
            .env("ECF_HOME", home.path())
            .env("ECF_PORT", port.to_string())
            .current_dir(home.path())
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()
            .expect("start ecflow_server");
        Some(Self {
            process,
            port,
            _home: home,
        })
    }

    fn client(&self) -> Client {
        let mut client = Client::with_host_port("localhost", self.port).expect("create client");
        client.set_connect_timeout(Duration::from_secs(5));
        client.set_retry_connection_period(Duration::from_millis(500));
        client.set_connection_attempts(2);

        let deadline = Instant::now() + Duration::from_secs(30);
        while let Err(error) = client.ping() {
            assert!(
                Instant::now() < deadline,
                "server did not answer within 30 s: {error}"
            );
            std::thread::sleep(Duration::from_millis(500));
        }
        client
    }
}

/// A free port in the range the server accepts (1024 to 49151), which
/// excludes the ephemeral ports the system would hand out.
fn free_port() -> u16 {
    (3141..49151)
        .find(|port| TcpListener::bind(("127.0.0.1", *port)).is_ok())
        .expect("a free port")
}

impl Drop for Server {
    fn drop(&mut self) {
        let _ = self.process.kill();
        let _ = self.process.wait();
    }
}

const DEFS: &str = "\
suite rust_test
  task t1
    meter progress 0 100 50
    label info \"\"
    event done
endsuite
";

#[test]
fn round_trip() {
    let Some(server) = Server::start() else {
        eprintln!("skipped: set ECFLOW_SERVER to an ecflow_server executable");
        return;
    };
    let mut client = server.client();

    assert!(
        client
            .server_version()
            .expect("server version")
            .starts_with(env!("CARGO_PKG_VERSION")),
        "server version differs from the library version"
    );
    assert!(!client.stats().expect("stats").is_empty());

    client.load_defs_text(DEFS, true).expect("load defs");
    let defs = client.get_defs_text(DefsStyle::Defs).expect("get defs");
    assert!(defs.contains("suite rust_test"), "{defs}");

    client.invoke(["--suites"]).expect("suites");
    assert_eq!(client.reply_strings(), vec!["rust_test".to_string()]);

    // A fresh server starts halted and answers child commands with "blocked"
    // until it is restarted. The dummy job password is then accepted for any
    // task without zombie checks.
    client.invoke(["--restart"]).expect("restart");
    client.set_child_timeout(Duration::from_secs(10));
    client.set_child_path("/rust_test/t1");
    client.set_child_password("_DJP_");
    client.set_child_pid("1");
    client.set_child_try_no(1);
    client.child_meter("progress", 42).expect("meter");
    client.child_label("info", "hello").expect("label");
    client.child_event("done", true).expect("event");

    let state = client.get_defs_text(DefsStyle::State).expect("get state");
    assert!(state.contains("42"), "{state}");
    assert!(state.contains("hello"), "{state}");

    // The server answered, so no transport failure is recorded.
    let error = client
        .invoke(["--suspend=/no_such_suite"])
        .expect_err("unknown path");
    assert_eq!(error.failure(), Failure::None, "{error}");
    assert!(error.message().contains("Could not find node"), "{error}");
}

#[test]
fn connection_refused_is_diagnosed() {
    let port = TcpListener::bind("127.0.0.1:0")
        .expect("bind a free port")
        .local_addr()
        .expect("local address")
        .port();
    let mut client = Client::with_host_port("localhost", port).expect("create client");
    client.set_connect_timeout(Duration::from_secs(2));
    client.set_retry_connection_period(Duration::from_millis(100));
    client.set_connection_attempts(1);

    let error = client.ping().expect_err("nothing listens");
    assert_eq!(error.failure(), Failure::ConnectionRefused, "{error}");
}

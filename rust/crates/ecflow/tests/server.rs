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

use ecflow::{CheckPtMode, Client, DefsStyle, Failure, NodeOrder, NodeState};

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

    let round_trip = client.ping().expect("ping");
    assert!(round_trip > Duration::ZERO, "{round_trip:?}");
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

    assert_eq!(client.suites().expect("suites"), ["rust_test"]);

    // A fresh server starts halted and answers child commands with "blocked"
    // until it is restarted. The dummy job password is then accepted for any
    // task without zombie checks.
    client.restart_server().expect("restart");
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
        .delete(["/no_such_suite"], false)
        .expect_err("unknown path");
    assert_eq!(error.failure(), Failure::None, "{error}");
    assert!(error.message().contains("Could not find node"), "{error}");

    node_commands(&mut client);
    server_commands(&mut client);
}

/// The node commands, on the suite while it is suspended: begun that way,
/// the server does not submit t1, which has no script.
fn node_commands(client: &mut Client) {
    client.suspend(["/rust_test"]).expect("suspend");
    let state = client.get_defs_text(DefsStyle::State).expect("get state");
    assert!(state.contains("suspended"), "{state}");
    client.begin_suite("rust_test", false).expect("begin");
    assert_eq!(client.check(["/rust_test"]).expect("check"), "");
    client
        .force_state(["/rust_test/t1"], NodeState::Complete, false, false)
        .expect("force state");
    client
        .force_event(["/rust_test/t1:done"], false)
        .expect("force event");
    assert_eq!(
        client
            .query("event", "/rust_test/t1", "done", false)
            .expect("query event"),
        "clear"
    );
    client.requeue(["/rust_test/t1"], "").expect("requeue");
    client
        .order("/rust_test/t1", NodeOrder::Top)
        .expect("order");
    client
        .free_trigger_dep(["/rust_test/t1"])
        .expect("free trigger");
    let history = client.edit_history("/rust_test/t1").expect("history");
    assert!(
        history.iter().any(|line| line.contains("--force")),
        "{history:?}"
    );
    assert_eq!(
        client
            .query("state", "/rust_test/t1", "", false)
            .expect("query state"),
        "queued"
    );
    client
        .alter(["/rust_test"], "add", "variable", "FOO", "bar")
        .expect("alter");
    assert_eq!(
        client
            .query("variable", "/rust_test", "FOO", false)
            .expect("query variable"),
        "bar"
    );
    client
        .sort_attributes(["/rust_test"], "variable", true)
        .expect("sort attributes");
    let error = client
        .get_file("/rust_test/t1", "script", 100)
        .expect_err("no script");
    assert_eq!(error.failure(), Failure::None, "{error}");
    client.resume(["/rust_test"]).expect("resume");
}

/// The log and server commands, then the deletion of everything.
fn server_commands(client: &mut Client) {
    client.log_msg("hello from rust").expect("log msg");
    let log = client.get_log(Some(10)).expect("get log");
    assert!(log.contains("hello from rust"), "{log}");
    assert!(!client.get_log_path().expect("log path").is_empty());
    client.flush_log().expect("flush log");

    let handle = client.ch_register(false, ["rust_test"]).expect("register");
    assert!(handle > 0);
    assert_eq!(client.ch_handle(), handle);
    let handles = client.ch_suites().expect("handles");
    assert!(
        handles
            .iter()
            .any(|(h, suites)| *h == handle && suites == &["rust_test"]),
        "{handles:?}"
    );
    client.ch_auto_add(handle, true).expect("auto add");
    client
        .ch_remove(handle, ["rust_test"])
        .expect("remove suite");
    client.ch_add(handle, ["rust_test"]).expect("add suite");
    client.ch_drop(handle).expect("drop handle");
    client
        .zombie_remove(["/rust_test/t1"])
        .expect("zombie remove");

    client.checkpt().expect("checkpt");
    client
        .configure_checkpt(
            Some(CheckPtMode::Never),
            Some(Duration::from_secs(300)),
            None,
        )
        .expect("configure checkpt");
    client.stats_reset().expect("stats reset");
    client.debug_server_on().expect("debug on");
    client.debug_server_off().expect("debug off");

    client.halt_server().expect("halt");
    client.restart_server().expect("restart");
    client.delete(["/rust_test/t1"], true).expect("delete node");
    client.delete_all(true).expect("delete all");
    assert!(client.suites().expect("suites").is_empty());
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

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! The commands `ecflow_client` confirms on the terminal never read standard
//! input here: a child process with standard input closed sends them, and a
//! prompt would either block it or end it.

use std::net::TcpListener;
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

use ecflow::{Client, Confirmation, Failure};

const CHILD: &str = "ECFLOW_PROMPT_CHILD";

#[test]
fn prompting_commands_never_read_standard_input() {
    if std::env::var_os(CHILD).is_some() {
        return in_child();
    }

    let mut child = Command::new(std::env::current_exe().expect("test executable"))
        .args([
            "--exact",
            "prompting_commands_never_read_standard_input",
            "--nocapture",
        ])
        .env(CHILD, "1")
        .stdin(Stdio::null())
        .spawn()
        .expect("start the child");

    let deadline = Instant::now() + Duration::from_secs(60);
    loop {
        if let Some(status) = child.try_wait().expect("poll the child") {
            assert!(status.success(), "child failed: {status}");
            return;
        }
        if Instant::now() >= deadline {
            child.kill().expect("kill the child");
            child.wait().expect("reap the child");
            panic!("child did not finish: a prompt is waiting for input");
        }
        std::thread::sleep(Duration::from_millis(100));
    }
}

/// Nobody listens on the port, so a command that was sent fails to connect
/// and the error carries the request that was built. A refused command
/// never connects.
fn in_child() {
    let port = TcpListener::bind("127.0.0.1:0")
        .expect("bind a free port")
        .local_addr()
        .expect("local address")
        .port();
    let mut client = Client::with_host_port("localhost", port).expect("create client");
    client.set_connect_timeout(Duration::from_secs(2));
    client.set_retry_connection_period(Duration::from_millis(100));
    client.set_connection_attempts(1);

    let refused = client.invoke(["--delete=/s1"]).expect_err("refused");
    assert_eq!(refused.failure(), Failure::None, "{refused}");
    assert!(refused.message().contains("not confirmed"), "{refused}");

    sent_once_confirmed(&mut client);
    never_sent(&mut client);
}

/// The words, what they ask, and the request that is sent once confirmed.
fn sent_once_confirmed(client: &mut Client) {
    let nodes =
        |paths: &[&str]| Confirmation::DeleteNodes(paths.iter().map(|p| (*p).to_owned()).collect());

    for (words, asks, request) in [
        (
            vec!["--delete=/s1"],
            vec![nodes(&["/s1"])],
            "--delete yes /s1",
        ),
        (
            vec!["--delete=/s1", "yes"],
            vec![nodes(&["/s1"])],
            "--delete yes /s1",
        ),
        (
            vec!["--delete=_all_"],
            vec![Confirmation::DeleteAll],
            "--delete _all_ yes",
        ),
        (
            vec!["--dele=force", "/s1"],
            vec![nodes(&["/s1"])],
            "--delete force yes /s1",
        ),
        (
            vec!["--suspend=/s1", "--halt"],
            vec![Confirmation::Halt],
            "--halt=yes",
        ),
        (vec!["--halt", ""], vec![Confirmation::Halt], "--halt=yes"),
        (
            vec!["--shutdown"],
            vec![Confirmation::Shutdown],
            "--shutdown=yes",
        ),
        (
            vec!["--terminate"],
            vec![Confirmation::Terminate],
            "--terminate=yes",
        ),
        (vec!["--halt=yes"], vec![], "--halt=yes"),
        (vec!["--ping", "--halt"], vec![], "--ping"),
        (
            vec!["--group=halt; delete /s1"],
            vec![Confirmation::Halt, nodes(&["/s1"])],
            "--group=--halt=yes; --delete yes /s1",
        ),
        (
            vec!["--group=delete \"x yes z\" /s1"],
            vec![nodes(&["/s1"])],
            "--group=--delete yes /s1",
        ),
    ] {
        let mut asked = Vec::new();
        let error = client
            .confirming(|what| {
                asked.push(what.clone());
                true
            })
            .invoke(&words)
            .expect_err("no server listens");
        assert_eq!(asked, asks, "{words:?}");
        assert_eq!(
            error.failure(),
            Failure::ConnectionRefused,
            "{words:?}: {error}"
        );
        assert!(
            error.message().contains(&format!("request( {request} ")),
            "{words:?}: {error}"
        );
    }
}

/// Declined or refused, nothing is sent.
fn never_sent(client: &mut Client) {
    // A group stops at the first no.
    let declined = client
        .confirming(|_| false)
        .invoke(["--delete=/s1"])
        .expect("declined");
    assert!(declined.is_none());
    let declined = client
        .confirming(|what| *what == Confirmation::Halt)
        .invoke(["--group=halt; delete /s1"])
        .expect("declined");
    assert!(declined.is_none());

    // Approvals do not outlive the call that gave them.
    let error = client.invoke(["--halt"]).expect_err("refused again");
    assert!(error.message().contains("not confirmed"), "{error}");

    let none: [&str; 0] = [];
    let error = client
        .delete_nodes(none, true)
        .expect_err("no paths means every suite");
    assert_eq!(error.failure(), Failure::None, "{error}");
    let error = client
        .confirming(|_| true)
        .delete_nodes(none, true)
        .expect_err("no paths means every suite");
    assert_eq!(error.failure(), Failure::None, "{error}");
}

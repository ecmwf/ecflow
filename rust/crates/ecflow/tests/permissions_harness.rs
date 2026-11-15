// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Self-tests of the harness for tests of Permissions attributes.

mod common;

use std::sync::Mutex;

use common::{Identity, PermissionsServer};

/// Serialises the tests of this binary, which inspect and change the environment.
static TESTS: Mutex<()> = Mutex::new(());

const ADMIN: Identity = Identity {
    user: "admin",
    password: "admin-secret",
};
const ALICE: Identity = Identity {
    user: "alice",
    password: "alice-secret",
};

#[test]
fn identities_are_authenticated_as_listed() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let Some(mut server) = PermissionsServer::start(
        ADMIN,
        &[ALICE],
        Some(
            r#"[{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}]"#,
        ),
    ) else {
        return;
    };

    assert!(server.client("alice").suites().is_ok());
    assert!(server.client("mallory").suites().is_err());
    // The user running the test, without explicit identity, is not authenticated
    let mut plain =
        ecflow::Client::with_host_port("localhost", server.port()).expect("create client");
    plain.set_connection_attempts(1);
    assert!(plain.suites().is_err());

    let log = std::fs::read_to_string(server.home().join("server.log")).expect("server log");
    assert!(log.contains("  Users = 2"), "{log}");

    let port = server.port();
    assert!(server.stop());
    assert!(
        std::net::TcpListener::bind(("0.0.0.0", port)).is_ok(),
        "port {port} still in use"
    );
}

/// Sets variables for the lifetime of a test, restoring their values after.
struct Variables(Vec<(&'static str, Option<std::ffi::OsString>)>);

impl Variables {
    fn set(values: &[(&'static str, &str)]) -> Self {
        let saved = values
            .iter()
            .map(|(name, _)| (*name, std::env::var_os(name)))
            .collect();
        for (name, value) in values {
            // SAFETY: the tests of this binary run one at a time
            unsafe { std::env::set_var(name, value) };
        }
        Self(saved)
    }
}

impl Drop for Variables {
    fn drop(&mut self) {
        for (name, value) in &self.0 {
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

#[test]
fn start_failure_restores_environment_and_port() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if common::server_executable().is_none() {
        return;
    }
    let locks = tempfile::tempdir().expect("lock directory");
    let original_passwords = std::env::var_os("ECF_CUSTOM_PASSWD");
    let _variables = Variables::set(&[
        ("ECF_PORT", "1234"),
        ("ECF_PORT_LOCK_DIR", locks.path().to_str().unwrap()),
    ]);

    // An invalid server attribute stops the start-up of the server
    let result = std::panic::catch_unwind(|| {
        PermissionsServer::start(ADMIN, &[], Some(r#""not a list""#)).map(|server| server.port())
    });
    assert!(
        result.is_err(),
        "the harness accepted a server that did not start"
    );

    assert_eq!(std::env::var("ECF_PORT").as_deref(), Ok("1234"));
    assert_eq!(std::env::var_os("ECF_CUSTOM_PASSWD"), original_passwords);
    let remaining = std::fs::read_dir(locks.path())
        .expect("lock directory")
        .count();
    assert_eq!(remaining, 0, "a port is still locked");
}

#[test]
fn stop_kills_when_termination_is_refused() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let Some(mut server) = PermissionsServer::start(ADMIN, &[], None) else {
        return;
    };
    let port = server.port();

    // Termination is requested as a user the server does not know
    server.set_administrator(Identity {
        user: "mallory",
        password: "mallory-secret",
    });
    assert!(!server.stop());
    assert!(server.administrator().ping().is_err());
    drop(server);
    assert!(
        std::net::TcpListener::bind(("0.0.0.0", port)).is_ok(),
        "port {port} still in use"
    );
}

#[test]
fn invalid_user_name_is_refused_by_the_client() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let Some(server) = PermissionsServer::start(ADMIN, &[ALICE], None) else {
        return;
    };

    for name in [
        "al ice",
        "-bob",
        "*",
        "a:b",
        "a;b",
        "a,b",
        "a#b",
        "a\u{1}b",
        "a\u{a0}b",
        "a\u{3000}b",
    ] {
        let mut client = server.client(name);
        let error = client.ping().expect_err("the user name was accepted");
        assert_ne!(
            error.failure(),
            ecflow::Failure::PermissionDenied,
            "{error}"
        );
        assert!(
            error.to_string().contains("invalid user name"),
            "{name:?}: {error}"
        );
    }
    server.client("alice").ping().expect("ping");
}

#[test]
fn denial_is_a_permission_denied_failure() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let Some(server) = PermissionsServer::start(ADMIN, &[ALICE], None) else {
        return;
    };

    // A white list allowing alice to read only
    let host = String::from_utf8(
        std::process::Command::new("hostname")
            .output()
            .expect("hostname")
            .stdout,
    )
    .expect("host name")
    .trim()
    .to_string();
    std::fs::write(
        server
            .home()
            .join(format!("{host}.{}.ecf.lists", server.port())),
        "4.5.0\nadmin\n-alice\n",
    )
    .expect("white list");
    let mut admin = server.administrator();
    admin.reload_wl_file().expect("reload the white list");
    admin
        .load_defs_text("suite secret_suite\nendsuite\n", false)
        .expect("load");

    let mut alice = server.client("alice");
    let error = alice
        .suspend(["/secret_suite"])
        .expect_err("the request was not denied");
    assert_eq!(error.failure(), ecflow::Failure::PermissionDenied);
    // The message names the reason, the request and its requester (D5.29)
    let message = error.to_string();
    assert!(
        message.contains(
            "Insufficient permissions (write access required at /secret_suite) [--suspend /secret_suite :alice@"
        ),
        "{message}"
    );

    // Other errors are not denials, on the same client: an argument refused
    // before anything is sent, then a request that succeeds
    let error = alice
        .requeue(["/secret_suite"], "invalid")
        .expect_err("the option is invalid");
    assert_ne!(
        error.failure(),
        ecflow::Failure::PermissionDenied,
        "{error}"
    );
    alice.ping().expect("ping");
    let error = alice
        .suspend(["/secret_suite"])
        .expect_err("the request was not denied");
    assert_eq!(error.failure(), ecflow::Failure::PermissionDenied);

    // nor on another client
    let error = admin
        .suspend(["/no/such/node"])
        .expect_err("the node does not exist");
    assert_ne!(error.failure(), ecflow::Failure::PermissionDenied);

    // A wait whose pings are denied leaves no denial behind
    std::fs::write(
        server
            .home()
            .join(format!("{host}.{}.ecf.lists", server.port())),
        "4.5.0\nadmin\n",
    )
    .expect("white list");
    admin.reload_wl_file().expect("reload the white list");
    assert!(!alice.wait_for_server_reply(std::time::Duration::from_secs(1)));
    let error = alice
        .requeue(["/secret_suite"], "invalid")
        .expect_err("the option is invalid");
    assert_ne!(
        error.failure(),
        ecflow::Failure::PermissionDenied,
        "{error}"
    );
}

/// A7.3: an operator with r at server level and r, but not x, on /sB requeues
/// the aborted task /sB/t; the request fails as a denial and the task stays
/// aborted.
#[test]
fn acceptance_a7_3_denied_operation() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let Some(server) = PermissionsServer::start(
        ADMIN,
        &[ALICE],
        Some(
            r#"[{"user": "admin", "permissions": "rwxlops"}, {"user": "alice", "permissions": "r"}]"#,
        ),
    ) else {
        return;
    };
    let mut admin = server.administrator();
    admin
        .load_defs_text(
            "suite sB\npermissions [{\"user\": \"admin\", \"permissions\": \"rwxlop\"}, {\"user\": \"alice\", \"permissions\": \"r\"}]\n  task t\nendsuite\n",
            false,
        )
        .expect("load");
    admin
        .force_state(["/sB/t"], ecflow::NodeState::Aborted, false, false)
        .expect("abort");

    let error = server
        .client("alice")
        .requeue(["/sB/t"], "")
        .expect_err("the requeue was not denied");
    assert_eq!(
        error.failure(),
        ecflow::Failure::PermissionDenied,
        "{error}"
    );
    assert_eq!(
        admin.query("state", "/sB/t", "", false).expect("state"),
        "aborted"
    );
}

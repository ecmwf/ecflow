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

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! The operations of the command `--permissions` in the Rust client, against a
//! live server: the JSON form of the attribute, each operation, invalid input
//! and denials.

mod common;

use std::sync::Mutex;

use common::{Identity, PermissionsServer};
use ecflow::Failure;

/// Serialises the tests of this binary, which change the environment.
static TESTS: Mutex<()> = Mutex::new(());

const ADMIN: Identity = Identity {
    user: "admin",
    password: "admin-secret",
};
const ALICE: Identity = Identity {
    user: "alice",
    password: "alice-secret",
};
const BOB: Identity = Identity {
    user: "bob",
    password: "bob-secret",
};

const SERVER_ATTRIBUTE: &str = r#"[{"user": "admin", "permissions": "rwxlop"}, {"user": "alice", "permissions": "rwxp"}, {"user": "bob", "permissions": "r"}]"#;

const DEFS: &str =
    "suite s\n  family f\n    task t\n  endfamily\nendsuite\nsuite u\n  task t\nendsuite\n";

fn start() -> Option<PermissionsServer> {
    let server = PermissionsServer::start(ADMIN, &[ALICE, BOB], Some(SERVER_ATTRIBUTE))?;
    server
        .administrator()
        .load_defs_text(DEFS, false)
        .expect("load defs");
    Some(server)
}

#[test]
fn each_operation() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(std::sync::PoisonError::into_inner);
    let Some(server) = start() else {
        return;
    };
    let mut alice = server.client("alice");

    // read returns the JSON form
    assert_eq!(
        alice.read_permissions("/").expect("read /"),
        r#"[{"permissions":"rwxlop","user":"admin"},{"permissions":"rwxp","user":"alice"},{"permissions":"r","user":"bob"}]"#
    );
    let missing = alice
        .read_permissions("/s")
        .expect_err("no attribute at /s");
    assert!(
        missing
            .message()
            .contains("/s has no Permissions attribute"),
        "{missing}"
    );

    // create, from either textual form
    alice
        .create_permissions("/s", "alice:rwxp,bob:r")
        .expect("create /s");
    assert_eq!(
        alice.read_permissions("/s").expect("read /s"),
        r#"[{"permissions":"rwxp","user":"alice"},{"permissions":"r","user":"bob"}]"#
    );
    alice
        .create_permissions("/s/f", r#"[{"user": "alice", "permissions": "rwxp"}]"#)
        .expect("create /s/f");
    assert_eq!(
        alice.read_permissions("/s/f").expect("read /s/f"),
        r#"[{"permissions":"rwxp","user":"alice"}]"#
    );
    let twice = alice
        .create_permissions("/s", "alice:rwxp")
        .expect_err("a second attribute");
    assert!(
        twice
            .message()
            .contains("/s already has a Permissions attribute"),
        "{twice}"
    );
    let unlisted = alice
        .create_permissions("/s/f/t", "alice:rp,carol:r")
        .expect_err("carol is not listed above");
    assert!(unlisted.message().contains("carol"), "{unlisted}");

    // update
    alice
        .update_permissions("/s", "alice:rwxp,bob:rw")
        .expect("update /s");
    assert_eq!(
        alice.read_permissions("/s").expect("read /s"),
        r#"[{"permissions":"rwxp","user":"alice"},{"permissions":"rw","user":"bob"}]"#
    );
    let lockout = alice
        .update_permissions("/s", "alice:rwx,bob:rw")
        .expect_err("nobody would hold p");
    assert!(
        lockout.message().contains("nobody able to change"),
        "{lockout}"
    );

    // effective
    assert_eq!(
        alice.effective_permissions("/s", None).expect("own"),
        "rwxp"
    );
    assert_eq!(
        alice.effective_permissions("/s", Some("bob")).expect("bob"),
        "rw"
    );
    assert_eq!(
        alice
            .effective_permissions("/s", Some("carol"))
            .expect("carol"),
        ""
    );
    for name in ["*", "al ice", "a:b"] {
        let error = alice
            .effective_permissions("/s", Some(name))
            .expect_err("invalid user name");
        assert!(
            error.message().contains("invalid user name"),
            "{name}: {error}"
        );
    }

    // delete
    alice.delete_permissions("/s/f").expect("delete /s/f");
    assert!(alice.read_permissions("/s/f").is_err());

    // The changes reach the definitions the client holds
    alice.sync_local(false).expect("sync");
    let text = alice
        .defs_text(ecflow::DefsStyle::Defs)
        .expect("definitions");
    assert!(
        text.contains(r#"permissions [{"permissions":"rwxp","user":"alice"},{"permissions":"rw","user":"bob"}]"#),
        "{text}"
    );
    assert_eq!(text.matches("permissions [").count(), 1, "{text}");
}

#[test]
fn invalid_input_is_refused() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(std::sync::PoisonError::into_inner);
    let Some(server) = start() else {
        return;
    };
    let mut alice = server.client("alice");

    for (path, value, reason) in [
        ("/s", "alice:", "Invalid Permissions"),
        ("/s", "", "create requires a value"),
        ("s", "alice:rwxp", "the path must be absolute"),
        ("/s", "[x:", "as the JSON form"),
    ] {
        let error = alice
            .create_permissions(path, value)
            .expect_err("refused by the client");
        assert!(error.message().contains(reason), "{error}");
        assert_ne!(error.failure(), Failure::PermissionDenied);
    }
    assert!(alice.read_permissions("/s").is_err());
}

#[test]
fn denials_are_permission_denied() {
    let _serial = TESTS
        .lock()
        .unwrap_or_else(std::sync::PoisonError::into_inner);
    let Some(server) = start() else {
        return;
    };
    let mut alice = server.client("alice");
    alice
        .create_permissions("/s", "alice:rwxp,bob:rwx")
        .expect("create /s");
    alice
        .create_permissions("/u", "alice:rwxp")
        .expect("create /u");

    let mut bob = server.client("bob");
    assert_eq!(bob.effective_permissions("/s", None).expect("own"), "rwx");
    let denied: Vec<ecflow::Result<String>> = vec![
        bob.update_permissions("/s", "alice:rwxp,bob:rwxp")
            .map(|()| String::new()),
        bob.delete_permissions("/s").map(|()| String::new()),
        bob.create_permissions("/s/f", "bob:r")
            .map(|()| String::new()),
        bob.update_permissions("/", "admin:rwxlop")
            .map(|()| String::new()),
        bob.read_permissions("/u"),
        bob.effective_permissions("/u", Some("alice")),
    ];
    for result in denied {
        let error = result.expect_err("denied");
        assert_eq!(error.failure(), Failure::PermissionDenied, "{error}");
    }
    assert_eq!(
        alice.read_permissions("/s").expect("unchanged"),
        r#"[{"permissions":"rwxp","user":"alice"},{"permissions":"rwx","user":"bob"}]"#
    );
}

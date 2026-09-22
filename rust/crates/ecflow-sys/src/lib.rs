// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! FFI bindings to the client side of ECMWF's ecFlow C++ library.
//!
//! This crate builds ecFlow from source and exposes `ClientInvoker` through a
//! CXX bridge. Use the `ecflow` crate for a safe API.
//!
//! The `ignore` list holds the `ClientInvoker` methods without a bridge
//! function; every command is reachable through `invoke`.

use bindman::track_cpp_api;

#[track_cpp_api(
    "ecflow/client/ClientInvoker.hpp",
    class = "ClientInvoker",
    ignore = [
        // Environment and diagnosis
        "environment", "effective_protocol", "connection_diagnosis", "probe_protocol", "to_string",
        "set_throw_on_error", "set_auto_sync", "is_auto_sync_enabled", "round_trip_time", "set_cli", "cli",
        "enable_ssl_if_defined", "get_certificate", "set_hostport", "taskPath", "set_jobs_password", "setEnv",
        "testInterface", "process_or_remote_id", "enable_logging", "disable_logging", "reset", "server_reply",
        "defs", "get_string", "in_sync", "get_news", "client_handle", "errorMsg", "get_cmd_from_args",
        "is_not_retrying", "load_in_memory_defs", "client_env_host_port", "check_child_parameters",
        // Task commands taking their arguments from the environment
        "initTask", "abortTask", "eventTask", "meterTask", "labelTask", "waitTask", "queueTask", "completeTask",
        "set_child_host_file", "set_child_denied", "set_child_no_ecf", "set_child_init_add_vars",
        "set_child_complete_del_vars",
        // User commands
        "sync", "sync_local", "news", "news_local", "changed_node_paths", "wait_for_server_reply",
        "wait_for_server_death",
        "restartServer", "haltServer", "shutdownServer", "terminateServer", "server_load", "debug_server_on",
        "debug_server_off", "stats_reset", "stats_server", "suites", "ch_register", "ch_suites", "ch_drop",
        "ch_drop_user", "ch_add", "ch_remove", "ch_auto_add", "ch1_register", "ch1_drop", "ch1_add", "ch1_remove",
        "ch1_auto_add", "begin", "begin_all_suites", "zombieGet", "zombieFob", "zombieFail", "zombieAdopt",
        "zombieBlock", "zombieRemove", "zombieKill", "zombieFobCli", "zombieFailCli", "zombieAdoptCli",
        "zombieBlockCli", "zombieRemoveCli", "zombieKillCli", "zombieFobCliPaths", "zombieFailCliPaths",
        "zombieAdoptCliPaths", "zombieBlockCliPaths", "zombieRemoveCliPaths", "zombieKillCliPaths", "job_gen",
        "edit_history", "kill", "status", "suspend", "resume", "check", "delete_nodes", "delete_node",
        "delete_all", "archive", "restore", "replace_1", "requeue", "run", "order", "checkPtDefs",
        "restoreDefsFromCheckPt", "force", "freeDep", "file", "plug", "query", "alter", "alter_sort",
        "reloadwsfile", "reloadpasswdfile", "reloadcustompasswdfile", "group", "logMsg", "new_log", "getLog",
        "clearLog", "flushLog", "get_log_path", "forceDependencyEval", "edit_script", "edit_script_edit",
        "edit_script_preprocess", "edit_script_submit",
    ]
)]
#[cxx::bridge(namespace = "ecflow_bridge")]
mod ffi {
    unsafe extern "C++" {
        include!("EcflowBridge.h");

        /// The ecFlow version the library was built from.
        #[must_use]
        fn version() -> String;

        /// Whether the library was built with OpenSSL support.
        #[must_use]
        fn ssl_supported() -> bool;

        // ==================== Client ====================

        /// Wraps a `ClientInvoker`; every request runs in its throw-on-error mode.
        type ClientWrapper;

        /// Create a client configured from the environment (`ECF_HOST`, `ECF_PORT`, ...).
        #[Self = "ClientWrapper"]
        fn create() -> Result<UniquePtr<ClientWrapper>>;

        /// Create a client for the given host and port.
        #[Self = "ClientWrapper"]
        fn from_host_port(host: &str, port: &str) -> Result<UniquePtr<ClientWrapper>>;

        // Connection configuration

        /// Override the host and port from the environment.
        fn set_host_port(self: Pin<&mut ClientWrapper>, host: &str, port: &str) -> Result<()>;
        /// The configured host.
        fn host(self: &ClientWrapper) -> String;
        /// The configured port.
        fn port(self: &ClientWrapper) -> String;
        /// Override the user name from `ECF_USER`.
        fn set_user_name(self: Pin<&mut ClientWrapper>, user: &str);
        /// Set the password for the user name.
        fn set_password(self: Pin<&mut ClientWrapper>, password: &str);
        /// Use SSL, whatever `ECF_SSL` says; fails when built without the `ssl` feature.
        fn enable_ssl(self: Pin<&mut ClientWrapper>) -> Result<()>;
        /// Do not use SSL, whatever `ECF_SSL` says.
        fn disable_ssl(self: Pin<&mut ClientWrapper>);
        /// Use the HTTP transport.
        fn enable_http(self: Pin<&mut ClientWrapper>);
        /// Use the HTTPS transport.
        fn enable_https(self: Pin<&mut ClientWrapper>);
        /// Time to wait for a server reply before a request fails.
        fn set_connect_timeout(self: Pin<&mut ClientWrapper>, milliseconds: u64);
        /// Time to wait between connection attempts.
        fn set_retry_connection_period(self: Pin<&mut ClientWrapper>, milliseconds: u64);
        /// Number of connection attempts per host before giving up.
        fn set_connection_attempts(self: Pin<&mut ClientWrapper>, attempts: u32);
        /// Print each request and its round trip time to standard output.
        fn debug(self: Pin<&mut ClientWrapper>, enabled: bool);

        /// Failure class of the request that last threw, as `ecf::ConnectionFailure`.
        fn last_failure(self: &ClientWrapper) -> i32;

        // Server probes

        /// Check that the server answers.
        fn ping_server(self: Pin<&mut ClientWrapper>) -> Result<()>;
        /// The server's version.
        fn server_version(self: Pin<&mut ClientWrapper>) -> Result<String>;
        /// The server's statistics, formatted by the server.
        fn stats(self: Pin<&mut ClientWrapper>) -> Result<String>;

        /// Run any command given as `ecflow_client` command line arguments.
        fn invoke(self: Pin<&mut ClientWrapper>, args: &[String]) -> Result<String>;

        /// The list of strings in the most recent reply, for commands that return one.
        fn reply_strings(self: &ClientWrapper) -> Vec<String>;

        // Child (task) commands

        /// The task path the child commands report for (`ECF_NAME`).
        fn set_child_path(self: Pin<&mut ClientWrapper>, path: &str);
        /// The job password the child commands carry (`ECF_PASS`).
        fn set_child_password(self: Pin<&mut ClientWrapper>, password: &str);
        /// The process or remote id the child commands carry (`ECF_RID`).
        fn set_child_pid(self: Pin<&mut ClientWrapper>, pid: &str);
        /// The try number the child commands carry (`ECF_TRYNO`).
        fn set_child_try_no(self: Pin<&mut ClientWrapper>, try_no: u32);
        /// How long a child command keeps trying to reach the server (`ECF_TIMEOUT`).
        fn set_child_timeout(self: Pin<&mut ClientWrapper>, seconds: u32);
        /// How long a child command keeps trying when reported as a zombie (`ECF_ZOMBIE_TIMEOUT`).
        fn set_zombie_child_timeout(self: Pin<&mut ClientWrapper>, seconds: u32);
        /// Report that the job started.
        fn child_init(self: Pin<&mut ClientWrapper>) -> Result<()>;
        /// Report that the job failed.
        fn child_abort(self: Pin<&mut ClientWrapper>, reason: &str) -> Result<()>;
        /// Set or clear an event of the task.
        fn child_event(self: Pin<&mut ClientWrapper>, name: &str, value: bool) -> Result<()>;
        /// Set a meter of the task.
        fn child_meter(self: Pin<&mut ClientWrapper>, name: &str, value: i32) -> Result<()>;
        /// Set a label of the task.
        fn child_label(self: Pin<&mut ClientWrapper>, name: &str, value: &str) -> Result<()>;
        /// Block until the expression holds on the server.
        fn child_wait(self: Pin<&mut ClientWrapper>, expression: &str) -> Result<()>;
        /// Act on a queue and return the step the server handed out.
        fn child_queue(
            self: Pin<&mut ClientWrapper>,
            queue: &str,
            action: &str,
            step: &str,
            path: &str,
        ) -> Result<String>;
        /// Report that the job finished.
        fn child_complete(self: Pin<&mut ClientWrapper>) -> Result<()>;

        // Definitions as text

        /// Fetch the server's definitions as text in the given `PrintStyle`.
        fn get_defs_text(self: Pin<&mut ClientWrapper>, style: i32) -> Result<String>;
        /// Load definitions given as text; with `force`, suites of the same name are replaced.
        fn load_defs_text(self: Pin<&mut ClientWrapper>, defs: &str, force: bool) -> Result<()>;
        /// Replace the node at `path` with the node of that path in the definitions given as text.
        fn replace_text(
            self: Pin<&mut ClientWrapper>,
            path: &str,
            defs: &str,
            create_parents: bool,
            force: bool,
        ) -> Result<()>;
    }
}

// Public re-exports for the safe wrapper crate
pub use cxx::{Exception, UniquePtr};
pub use ffi::*;

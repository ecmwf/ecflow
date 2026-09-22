// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! FFI bindings to the client side of ECMWF's ecFlow C++ library.
//!
//! This crate builds ecFlow from source and binds `ClientInvoker` through a
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
        "in_sync", "get_news", "client_handle", "errorMsg", "get_cmd_from_args", "is_not_retrying",
        "load_in_memory_defs", "loadDefs", "client_env_host_port", "check_child_parameters",
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
        "delete_all", "archive", "restore", "requeue", "run", "order", "checkPtDefs",
        "restoreDefsFromCheckPt", "force", "freeDep", "file", "plug", "query", "alter", "alter_sort",
        "reloadwsfile", "reloadpasswdfile", "reloadcustompasswdfile", "group", "logMsg", "new_log", "getLog",
        "clearLog", "flushLog", "get_log_path", "forceDependencyEval", "edit_script", "edit_script_edit",
        "edit_script_preprocess", "edit_script_submit",
    ]
)]
#[cxx::bridge]
mod ffi {
    /// The class of failure observed while communicating with a server.
    ///
    /// Declared as the existing `ecf::ConnectionFailure`, so cxx verifies at
    /// compile time that each discriminant matches the C++ enum.
    #[namespace = "ecf"]
    #[repr(i32)]
    enum ConnectionFailure {
        /// No failure was observed.
        None,
        /// The host name could not be resolved.
        HostResolution,
        /// No listener accepted the connection.
        ConnectionRefused,
        /// The peer accepted the connection but did not reply in time.
        Timeout,
        /// The peer accepted the connection and then closed it without replying.
        ClosedWithoutReply,
        /// The TLS handshake did not complete.
        HandshakeFailed,
        /// The TLS handshake failed while verifying the peer certificate.
        CertificateRejected,
        /// A reply was received that the transport cannot decode.
        UndecodableReply,
        /// The peer answered, refusing the request.
        RejectedRequest,
        /// A failure that none of the other values describes.
        Other,
    }

    /// The print style of definitions written as text.
    ///
    /// Declared as the existing `PrintStyle::Type_t` (aliased in the bridge
    /// header), so cxx verifies at compile time that each discriminant
    /// matches the C++ enum.
    #[namespace = "ecflow_bridge"]
    #[repr(i32)]
    enum DefsStyle {
        /// Nothing is written.
        #[cxx_name = "NOTHING"]
        Nothing,
        /// The definition alone, without node state.
        #[cxx_name = "DEFS"]
        Defs,
        /// The definition with node state and trigger expressions.
        #[cxx_name = "STATE"]
        State,
        /// The definition with node state, as a server loads it.
        #[cxx_name = "MIGRATE"]
        Migrate,
        /// The definition as transferred between client and server, loaded with relaxed checks.
        #[cxx_name = "NET"]
        Net,
    }

    unsafe extern "C++" {
        include!("EcflowBridge.h");

        #[namespace = "ecf"]
        type ConnectionFailure;

        /// A suite definition.
        type Defs;
    }

    #[namespace = "ecflow_bridge"]
    unsafe extern "C++" {
        type DefsStyle;

        /// The ecFlow client: a `ClientInvoker`, every request in its
        /// throw-on-error mode.
        type Client;

        /// Create a client configured from the environment (`ECF_HOST`, `ECF_PORT`, `ECF_SSL`, ...).
        #[Self = "Client"]
        fn create() -> Result<UniquePtr<Client>>;
        /// Create a client for the given host and port.
        #[Self = "Client"]
        fn from_host_port(host: &str, port: &str) -> Result<UniquePtr<Client>>;
        /// Parse definitions given in the ecFlow text format.
        #[Self = "Client"]
        fn parse_defs(text: &str) -> Result<SharedPtr<Defs>>;
        /// The ecFlow version the library was built from.
        #[Self = "Client"]
        #[must_use]
        fn version() -> String;
        /// Whether the library was built with OpenSSL support.
        #[Self = "Client"]
        #[must_use]
        fn ssl_supported() -> bool;

        // Connection configuration

        /// Override the host and port from the environment.
        fn set_host_port(self: Pin<&mut Client>, host: &CxxString, port: &CxxString) -> Result<()>;
        /// The configured host.
        fn host(self: &Client) -> &CxxString;
        /// The configured port.
        fn port(self: &Client) -> &CxxString;
        /// Override the user name from `ECF_USER`.
        fn set_user_name(self: Pin<&mut Client>, user: &CxxString);
        /// Set the password for the user name.
        fn set_password(self: Pin<&mut Client>, password: &CxxString);
        /// Use SSL, whatever `ECF_SSL` says; fails when built without the `ssl` feature.
        fn enable_ssl(self: Pin<&mut Client>) -> Result<()>;
        /// Do not use SSL, whatever `ECF_SSL` says.
        fn disable_ssl(self: Pin<&mut Client>);
        /// Use the HTTP transport.
        fn enable_http(self: Pin<&mut Client>);
        /// Use the HTTPS transport.
        fn enable_https(self: Pin<&mut Client>);
        /// Time to wait for a server reply before a request fails.
        fn set_connect_timeout(self: Pin<&mut Client>, milliseconds: u64);
        /// Time to wait between connection attempts.
        fn set_retry_connection_period(self: Pin<&mut Client>, milliseconds: u64);
        /// Number of connection attempts per host before giving up.
        fn set_connection_attempts(self: Pin<&mut Client>, attempts: u32);
        /// Print each request and its round trip time to standard output.
        fn debug(self: Pin<&mut Client>, flag: bool);
        /// The failure class of the most recent request.
        fn last_failure(self: &Client) -> ConnectionFailure;

        // Server probes

        /// Check that the server answers.
        fn pingServer(self: &Client) -> Result<i32>;
        /// Ask for the server's version; the reply is in `get_string`.
        fn server_version(self: &Client) -> Result<i32>;
        /// Ask for the server's statistics; the reply is in `get_string`.
        fn stats(self: &Client) -> Result<i32>;
        /// The string of the most recent reply, for commands that return one.
        fn get_string(self: &Client) -> &CxxString;

        // Any command

        /// Run any command given as `ecflow_client` command line arguments.
        fn invoke(self: &Client, args: &[String]) -> Result<()>;
        /// The list of strings in the most recent reply, for commands that return one.
        fn reply_strings(self: &Client) -> Vec<String>;

        // Child (task) commands

        /// The task path the child commands report for (`ECF_NAME`).
        fn set_child_path(self: Pin<&mut Client>, path: &CxxString);
        /// The job password the child commands carry (`ECF_PASS`).
        fn set_child_password(self: Pin<&mut Client>, pass: &CxxString);
        /// The process or remote id the child commands carry (`ECF_RID`).
        fn set_child_pid(self: Pin<&mut Client>, pid: &CxxString);
        /// The try number the child commands carry (`ECF_TRYNO`).
        fn set_child_try_no(self: Pin<&mut Client>, try_no: u32);
        /// How long a child command keeps trying to reach the server (`ECF_TIMEOUT`).
        fn set_child_timeout(self: Pin<&mut Client>, seconds: u32);
        /// How long a child command keeps trying when reported as a zombie (`ECF_ZOMBIE_TIMEOUT`).
        fn set_zombie_child_timeout(self: Pin<&mut Client>, seconds: u32);
        /// Report that the job started.
        fn child_init(self: Pin<&mut Client>) -> Result<()>;
        /// Report that the job failed.
        fn child_abort(self: Pin<&mut Client>, reason: &CxxString) -> Result<()>;
        /// Set or clear an event of the task.
        fn child_event(
            self: Pin<&mut Client>,
            event_name_or_number: &CxxString,
            value: bool,
        ) -> Result<()>;
        /// Set a meter of the task.
        fn child_meter(
            self: Pin<&mut Client>,
            meter_name: &CxxString,
            meter_value: i32,
        ) -> Result<()>;
        /// Set a label of the task.
        fn child_label(
            self: Pin<&mut Client>,
            label_name: &CxxString,
            label_value: &CxxString,
        ) -> Result<()>;
        /// Block until the expression holds on the server.
        fn child_wait(self: Pin<&mut Client>, on_expression: &CxxString) -> Result<()>;
        /// Act on a queue and return the step the server handed out.
        fn child_queue(
            self: Pin<&mut Client>,
            queue: &CxxString,
            action: &CxxString,
            step: &CxxString,
            path: &CxxString,
        ) -> Result<String>;
        /// Report that the job finished.
        fn child_complete(self: Pin<&mut Client>) -> Result<()>;

        // Definitions

        /// Fetch the server's definitions; they are then in `defs`.
        fn getDefs(self: &Client) -> Result<i32>;
        /// The definitions of the most recent reply.
        fn defs(self: &Client) -> SharedPtr<Defs>;
        /// Fetch the server's definitions and write them as text in the given style.
        fn defs_text(self: &Client, style: DefsStyle) -> Result<String>;
        /// Load definitions into the server; with `force`, suites of the same name are replaced.
        fn load(self: &Client, defs: &SharedPtr<Defs>, force: bool) -> Result<i32>;
        /// Replace the node at `path` with the node of that path in the given definitions.
        fn replace_1(
            self: &Client,
            path: &CxxString,
            client_defs: &SharedPtr<Defs>,
            create_parents_as_required: bool,
            force: bool,
        ) -> Result<i32>;
    }
}

// Public re-exports for the safe wrapper crate
pub use cxx::{Exception, SharedPtr, UniquePtr, let_cxx_string};
pub use ffi::*;

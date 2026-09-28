// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! FFI bindings to the client side of ECMWF's ecFlow C++ library.
//!
//! This crate builds ecFlow from source and binds `ClientInvoker` through a
//! CXX bridge. Use the `ecflow` crate for a safe API.
//!
//! The `ignore` list holds the `ClientInvoker` methods without a bridge
//! function.

use bindman::track_cpp_api;

#[track_cpp_api(
    "ecflow/client/ClientInvoker.hpp",
    class = "ClientInvoker",
    ignore = [
        // Environment and diagnosis
        "environment", "effective_protocol", "connection_diagnosis", "probe_protocol", "to_string",
        "set_throw_on_error", "set_auto_sync", "is_auto_sync_enabled", "set_cli", "cli", "invoke",
        "enable_ssl_if_defined", "get_certificate", "set_hostport", "taskPath", "set_jobs_password", "setEnv",
        "testInterface", "process_or_remote_id", "enable_logging", "disable_logging", "reset", "server_reply",
        "in_sync", "get_news", "errorMsg", "get_cmd_from_args", "is_not_retrying",
        "load_in_memory_defs", "loadDefs", "client_env_host_port", "check_child_parameters",
        // Task commands taking their arguments from the environment
        "initTask", "abortTask", "eventTask", "meterTask", "labelTask", "waitTask", "queueTask", "completeTask",
        "set_child_host_file", "set_child_denied", "set_child_no_ecf", "set_child_init_add_vars",
        "set_child_complete_del_vars",
        // User commands
        "sync", "sync_local", "news", "news_local", "changed_node_paths", "wait_for_server_reply",
        "wait_for_server_death",
        "server_load", "stats_server", "ch1_register", "ch1_drop", "ch1_add", "ch1_remove", "ch1_auto_add",
        "zombieGet", "zombieFob", "zombieFail", "zombieAdopt", "zombieBlock", "zombieRemove", "zombieKill",
        "zombieFobCli", "zombieFailCli", "zombieAdoptCli", "zombieBlockCli", "zombieRemoveCli", "zombieKillCli",
        "delete_node", "group", "forceDependencyEval", "edit_script", "edit_script_edit",
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

    /// Where a node moves among its siblings, or how they are sorted.
    ///
    /// Declared as the existing `NOrder::Order` (aliased in the bridge
    /// header), so cxx verifies at compile time that each discriminant
    /// matches the C++ enum.
    #[namespace = "ecflow_bridge"]
    #[repr(i32)]
    enum NodeOrder {
        /// First among its siblings.
        #[cxx_name = "TOP"]
        Top,
        /// Last among its siblings.
        #[cxx_name = "BOTTOM"]
        Bottom,
        /// The siblings sorted alphabetically.
        #[cxx_name = "ALPHA"]
        Alpha,
        /// The siblings sorted in reverse alphabetical order.
        #[cxx_name = "ORDER"]
        Order,
        /// One place up.
        #[cxx_name = "UP"]
        Up,
        /// One place down.
        #[cxx_name = "DOWN"]
        Down,
        /// The siblings sorted by the time of their last state change.
        #[cxx_name = "RUNTIME"]
        Runtime,
    }

    /// When the server writes its check point file.
    ///
    /// Declared as the existing `ecf::CheckPt::Mode` (aliased in the bridge
    /// header), so cxx verifies at compile time that each discriminant
    /// matches the C++ enum.
    #[namespace = "ecflow_bridge"]
    #[repr(i32)]
    enum CheckPtMode {
        /// Never.
        #[cxx_name = "NEVER"]
        Never,
        /// Periodically, at the check point interval.
        #[cxx_name = "ON_TIME"]
        OnTime,
        /// After every state change.
        #[cxx_name = "ALWAYS"]
        Always,
        /// Leave the mode as it is.
        #[cxx_name = "UNDEFINED"]
        Undefined,
    }

    /// The state of a node.
    ///
    /// Declared as the existing `NState::State` (aliased in the bridge
    /// header), so cxx verifies at compile time that each discriminant
    /// matches the C++ enum.
    #[namespace = "ecflow_bridge"]
    #[repr(i32)]
    enum NodeState {
        /// Not yet begun.
        #[cxx_name = "UNKNOWN"]
        Unknown,
        /// Finished.
        #[cxx_name = "COMPLETE"]
        Complete,
        /// Waiting for its dependencies.
        #[cxx_name = "QUEUED"]
        Queued,
        /// Failed.
        #[cxx_name = "ABORTED"]
        Aborted,
        /// Handed to the job submission command.
        #[cxx_name = "SUBMITTED"]
        Submitted,
        /// Running.
        #[cxx_name = "ACTIVE"]
        Active,
    }

    /// A registered client handle and the suites it covers.
    #[namespace = "ecflow_bridge"]
    #[derive(Debug, Clone, PartialEq, Eq)]
    struct HandleSuites {
        /// The handle.
        handle: i32,
        /// The names of its suites.
        suites: Vec<String>,
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
        type NodeOrder;
        type CheckPtMode;
        type NodeState;

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
        /// The round trip time of the most recent request, in microseconds.
        fn round_trip_time(self: &Client) -> u64;

        // Server probes

        /// Check that the server answers.
        fn pingServer(self: &Client) -> Result<i32>;
        /// Ask for the server's version; the reply is in `get_string`.
        fn server_version(self: &Client) -> Result<i32>;
        /// Ask for the server's statistics; the reply is in `get_string`.
        fn stats(self: &Client) -> Result<i32>;
        /// Ask for the suite names; the reply is in `reply_strings`.
        fn suites(self: &Client) -> Result<i32>;
        /// The string of the most recent reply, for commands that return one.
        fn get_string(self: &Client) -> &CxxString;
        /// The list of strings in the most recent reply, for commands that return one.
        fn reply_strings(self: &Client) -> Vec<String>;

        // Server control

        /// Restart the server: it schedules jobs and accepts child commands again.
        fn restartServer(self: &Client) -> Result<i32>;
        /// Halt the server: no jobs are scheduled and child commands are blocked.
        fn haltServer(self: &Client) -> Result<i32>;
        /// Shut the server down: no jobs are scheduled, child commands still go through.
        fn shutdownServer(self: &Client) -> Result<i32>;
        /// Terminate the server process.
        fn terminateServer(self: &Client) -> Result<i32>;
        /// Set the check point mode, interval and save time alarm, each when given; with none, write a check point now.
        fn checkPtDefs(
            self: &Client,
            mode: CheckPtMode,
            check_pt_interval: i32,
            check_pt_save_time_alarm: i32,
        ) -> Result<i32>;
        /// Load the check point file; the server must be halted and hold no suites.
        fn restoreDefsFromCheckPt(self: &Client) -> Result<i32>;
        /// Reset the server's statistics.
        fn stats_reset(self: &Client) -> Result<i32>;
        /// Turn on debug output in the server.
        fn debug_server_on(self: &Client) -> Result<i32>;
        /// Turn off debug output in the server.
        fn debug_server_off(self: &Client) -> Result<i32>;
        /// Reload the white list file that controls who may read and write.
        fn reloadwsfile(self: &Client) -> Result<i32>;
        /// Reload the password file.
        fn reloadpasswdfile(self: &Client) -> Result<i32>;
        /// Reload the custom password file.
        fn reloadcustompasswdfile(self: &Client) -> Result<i32>;

        // Nodes

        /// Delete the nodes at the given paths; with `force`, even when active or submitted.
        fn delete_nodes(self: &Client, paths: &[String], force: bool) -> Result<()>;
        /// Delete every suite; with `force`, even when nodes are active or submitted.
        fn delete_all(self: &Client, force: bool) -> Result<i32>;
        /// Suspend the nodes: no jobs are generated below them.
        fn suspend(self: &Client, paths: &[String]) -> Result<()>;
        /// Resume suspended nodes.
        fn resume(self: &Client, paths: &[String]) -> Result<()>;
        /// Requeue the nodes and their children; `option` is empty, `abort` or `force`.
        fn requeue(self: &Client, paths: &[String], option: &CxxString) -> Result<()>;
        /// Run the nodes now, ignoring their dependencies; with `force`, even when active or submitted.
        fn run(self: &Client, paths: &[String], force: bool) -> Result<()>;
        /// Kill the jobs of the nodes.
        fn kill(self: &Client, paths: &[String]) -> Result<()>;
        /// Query the status of the jobs of the nodes.
        fn status(self: &Client, paths: &[String]) -> Result<()>;
        /// Check the expressions and limits below the nodes; the report is in `get_string`.
        fn check(self: &Client, paths: &[String]) -> Result<()>;
        /// Save the nodes' children to disk and drop them from the definition; with `force`, even when active.
        fn archive(self: &Client, paths: &[String], force: bool) -> Result<()>;
        /// Load archived children back into the definition.
        fn restore(self: &Client, paths: &[String]) -> Result<()>;
        /// Force the nodes to a state.
        fn force(
            self: &Client,
            paths: &[String],
            state: NodeState,
            recursive: bool,
            set_repeats_to_last_value: bool,
        ) -> Result<()>;
        /// Set or clear the events given as `path:event`.
        fn force_event(self: &Client, paths: &[String], set: bool) -> Result<()>;
        /// Free the chosen dependencies of the nodes.
        #[allow(clippy::fn_params_excessive_bools)]
        fn freeDep(
            self: &Client,
            paths: &[String],
            trigger: bool,
            all: bool,
            date: bool,
            time: bool,
        ) -> Result<()>;
        /// Move the node among its siblings, or sort them.
        fn order(self: &Client, path: &CxxString, order: NodeOrder) -> Result<i32>;
        /// Generate and submit the jobs below the node whose dependencies are free, without waiting for the server poll.
        fn job_gen(self: &Client, path: &CxxString) -> Result<i32>;
        /// Begin the suite; with `force`, even when it has active or submitted jobs.
        fn begin(self: &Client, suite: &CxxString, force: bool) -> Result<i32>;
        /// Begin every suite; with `force`, even when one has active or submitted jobs.
        fn begin_all_suites(self: &Client, force: bool) -> Result<i32>;
        /// Ask for the node's edit history; the lines are in `reply_strings`.
        fn edit_history(self: &Client, path: &CxxString) -> Result<i32>;
        /// Ask for a file of the node, the last `max_lines` of it; the text is in `get_string`.
        fn file(
            self: &Client,
            path: &CxxString,
            file_type: &CxxString,
            max_lines: &CxxString,
        ) -> Result<i32>;
        /// Move the node to another parent, possibly on another server.
        fn plug(self: &Client, source_path: &CxxString, dest_path: &CxxString) -> Result<i32>;
        /// Query an attribute or evaluate an expression; the answer is in `get_string`.
        fn query(
            self: Pin<&mut Client>,
            query_type: &CxxString,
            path_to_attribute: &CxxString,
            attribute: &CxxString,
            evaluate: bool,
        ) -> Result<i32>;
        /// Add, change or delete an attribute of the nodes, or set or clear a flag.
        fn alter(
            self: &Client,
            paths: &[String],
            alter_type: &CxxString,
            attr_type: &CxxString,
            name: &CxxString,
            value: &CxxString,
        ) -> Result<()>;
        /// Sort the attributes of a kind of the nodes by name.
        fn alter_sort(
            self: &Client,
            paths: &[String],
            attribute: &CxxString,
            recursive: bool,
        ) -> Result<()>;

        // Logs

        /// Write a message to the server's log.
        fn logMsg(self: &Client, msg: &CxxString) -> Result<i32>;
        /// Switch the server to a new log file; with an empty path, reopen the current one.
        fn new_log(self: &Client, new_path: &CxxString) -> Result<i32>;
        /// Ask for the last lines of the server's log, 100 with 0; the text is in `get_string`.
        fn getLog(self: &Client, last_lines: i32) -> Result<i32>;
        /// Empty the server's log file.
        fn clearLog(self: &Client) -> Result<i32>;
        /// Flush and close the server's log file.
        fn flushLog(self: &Client) -> Result<i32>;
        /// Ask for the path of the server's log file; it is in `get_string`.
        fn get_log_path(self: &Client) -> Result<i32>;

        // Zombies

        /// Let the child commands of the zombies at these paths succeed.
        fn zombieFobCliPaths(self: &Client, paths: &[String]) -> Result<()>;
        /// Make the child commands of the zombies at these paths fail.
        fn zombieFailCliPaths(self: &Client, paths: &[String]) -> Result<()>;
        /// Accept the passwords the zombies at these paths carry.
        fn zombieAdoptCliPaths(self: &Client, paths: &[String]) -> Result<()>;
        /// Make the child commands of the zombies at these paths block.
        fn zombieBlockCliPaths(self: &Client, paths: &[String]) -> Result<()>;
        /// Drop the zombies at these paths from the server's list.
        fn zombieRemoveCliPaths(self: &Client, paths: &[String]) -> Result<()>;
        /// Kill the zombie processes at these paths with `ECF_KILL_CMD`.
        fn zombieKillCliPaths(self: &Client, paths: &[String]) -> Result<()>;

        // Client handles

        /// Register interest in the suites; the new handle is in `client_handle`.
        fn ch_register(self: &Client, auto_add_new_suites: bool, suites: &[String]) -> Result<()>;
        /// The handle of the most recent registration, 0 without one.
        fn client_handle(self: &Client) -> i32;
        /// Ask for the registered handles and their suites; they are in `client_handle_suites`.
        fn ch_suites(self: &Client) -> Result<i32>;
        /// The handles and their suites from the most recent `ch_suites` reply.
        fn client_handle_suites(self: &Client) -> Vec<HandleSuites>;
        /// Drop the handle.
        fn ch_drop(self: &Client, client_handle: i32) -> Result<i32>;
        /// Drop every handle of the user, the client's own when empty.
        fn ch_drop_user(self: &Client, user: &CxxString) -> Result<i32>;
        /// Add suites to the handle.
        fn ch_add(self: &Client, client_handle: i32, suites: &[String]) -> Result<()>;
        /// Remove suites from the handle.
        fn ch_remove(self: &Client, client_handle: i32, suites: &[String]) -> Result<()>;
        /// Whether suites added later join the handle.
        fn ch_auto_add(self: &Client, client_handle: i32, auto_add_new_suites: bool)
        -> Result<i32>;

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

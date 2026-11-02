// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <iostream>
#include <string>

///
/// @brief Holds the global data used to determine incremental changes to the data model, and the built-in defaults.
///
/// The global data is composed of two counters:
///
///   - `state_change_no_`:   the number of node state changes on the currently loaded definition;
///   - `modify_change_no_`:  the number of structural changes to the currently loaded definition.
///
/// Each node and attribute stores a local state change number which, whenever the node or attribute changes, is
/// assigned the incremented value of the global `state_change_no_`. Large scale changes, such as adding or deleting
/// nodes, increment the global `modify_change_no_` instead.
///
/// When a client copies the full definition, both numbers are copied too. In subsequent synchronisation requests, the
/// client sends both numbers back, which allows the server to determine what has changed since.
///
/// The class also provides the built-in defaults of the server settings (file names, commands, intervals).
///
class Ecf {
public:
    using counter_t        = uint64_t;
    using atomic_counter_t = std::atomic<counter_t>;

    // Disable default construction
    Ecf() = delete;

    ///
    /// @brief Increments the global state change number, on the server side only, and returns it.
    ///
    /// @return The state change number, incremented when running on the server side.
    ///
    static counter_t incr_state_change_no() {
        if (server_) {
            ++state_change_no_;
        }
        return state_change_no_;
    }

    ///
    /// @brief Returns the global state change number.
    ///
    /// @return The state change number.
    ///
    static counter_t state_change_no() { return state_change_no_; }

    ///
    /// @brief Sets the global state change number.
    ///
    /// @param[in] x The new state change number
    ///
    static void set_state_change_no(counter_t x) { state_change_no_ = x; }

    ///
    /// @brief Increments the global modify change number, on the server side only, and returns it.
    ///
    /// The modify change number tracks structural changes: addition, deletion and re-ordering of nodes.
    ///
    /// @return The modify change number, incremented when running on the server side.
    ///
    static counter_t incr_modify_change_no() {
        if (server_) {
            ++modify_change_no_;
        }
        return modify_change_no_;
    }

    ///
    /// @brief Returns the global modify change number.
    ///
    /// @return The modify change number.
    ///
    static counter_t modify_change_no() { return modify_change_no_; }

    ///
    /// @brief Sets the global modify change number.
    ///
    /// @param[in] x The new modify change number
    ///
    static void set_modify_change_no(counter_t x) { modify_change_no_ = x; }

    ///
    /// @brief Indicates whether the process runs on the server side.
    ///
    /// Only the server side increments the state and modify change numbers. The flag is also used in debugging and
    /// tests, e.g. to tell server and client output apart.
    ///
    /// @return True on the server side.
    ///
    static bool server() { return server_; }

    ///
    /// @brief Sets whether the process runs on the server side.
    ///
    /// Only the server sets the flag; the function is public so that tests can set it too.
    ///
    /// @param[in] f True on the server side
    ///
    static void set_server(bool f) { server_ = f; }

    ///
    /// @brief Indicates whether the equality operators report why two objects differ.
    ///
    /// When enabled, in debug builds, the equality operators of the data model print the first difference found.
    ///
    /// @return True when reporting is enabled.
    ///
    static bool debug_equality() { return debug_equality_; }

    ///
    /// @brief Enables or disables the reporting of differences by the equality operators.
    ///
    /// @param[in] f True to enable reporting
    ///
    static void set_debug_equality(bool f) { debug_equality_ = f; }

    ///
    /// @brief Returns the debug level.
    ///
    /// A debug level above 0 disables some warning messages (ECFLOW-99).
    ///
    /// @return The debug level.
    ///
    static unsigned int debug_level() { return debug_level_; }

    ///
    /// @brief Sets the debug level.
    ///
    /// @param[in] level The debug level; above 0, some warning messages are disabled
    ///
    static void set_debug_level(unsigned int level) { debug_level_ = level; }

    ///
    /// @brief Returns the name of the server executable.
    ///
    /// @return The name, `ecflow_server`.
    ///
    static const char* SERVER_NAME();

    ///
    /// @brief Returns the name of the client executable.
    ///
    /// @return The name, `ecflow_client`.
    ///
    static const char* CLIENT_NAME();

    ///
    /// @brief Returns the default home directory of the server (`ECF_HOME`).
    ///
    /// @return The directory, `.`, the directory where the server is started.
    ///
    static const std::string& HOME();

    ///
    /// @brief Returns the default port of the server (`ECF_PORT`).
    ///
    /// @return The port, 3141.
    ///
    static int PORT();

    ///
    /// @brief Returns the default mode in which the server writes checkpoints (`ECF_CHECKMODE`).
    ///
    /// @return The mode, `CHECK_ON_TIME`.
    ///
    static const std::string& CHECK_MODE();

    ///
    /// @brief Returns the default name of the log file of the server.
    ///
    /// @return The file name, `ecf.log`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& LOG_FILE();

    ///
    /// @brief Returns the name of the configuration file of the server.
    ///
    /// @return The file name, `ecf.cfg`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& CONFIG_FILE();

    ///
    /// @brief Returns the default name of the checkpoint file of the server.
    ///
    /// @return The file name, `ecf.check`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& CHECKPT();

    ///
    /// @brief Returns the default name of the backup checkpoint file of the server.
    ///
    /// @return The file name, `ecf.check.b`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& BACKUP_CHECKPT();

    ///
    /// @brief Returns the default character marking variables and directives in scripts (`ECF_MICRODEF`).
    ///
    /// @return The character, `%`.
    ///
    static const std::string& MICRO();

    ///
    /// @brief Returns the default command submitting a job (`ECF_JOB_CMD`).
    ///
    /// @return The command, `%ECF_JOB% 1> %ECF_JOBOUT% 2>&1`.
    ///
    static const std::string& JOB_CMD();

    ///
    /// @brief Returns the default command killing a job (`ECF_KILL_CMD`).
    ///
    /// @return The command, `kill -15 %ECF_RID%`.
    ///
    static const std::string& KILL_CMD();

    ///
    /// @brief Returns the default command, run by the server, obtaining the status of a job (`ECF_STATUS_CMD`).
    ///
    /// @return The command, `ps --pid %ECF_RID% -f > %ECF_JOB%.stat 2>&1`.
    ///
    static const std::string& STATUS_CMD();

    ///
    /// @brief Returns the default command, run by the client, obtaining the status of a job (`ECF_CHECK_CMD`).
    ///
    /// @return The command, `ps --pid %ECF_RID% -f`.
    ///
    static const std::string& CHECK_CMD();

    ///
    /// @brief Returns the default command, run by the client, opening a URL in a browser (`ECF_URL_CMD`).
    ///
    /// @return The command, `${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%`.
    ///
    static const std::string& URL_CMD();

    ///
    /// @brief Returns the default base of the URLs opened by the client (`ECF_URL_BASE`).
    ///
    /// @return The base, `https://confluence.ecmwf.int`.
    ///
    static const std::string& URL_BASE();

    ///
    /// @brief Returns the default URL opened by the client, relative to the base (`ECF_URL`).
    ///
    /// @return The URL, `display/ECFLOW/ecflow+home`.
    ///
    static const std::string& URL();

    ///
    /// @brief Returns the default name of the password file of the server.
    ///
    /// @return The file name, `ecf.passwd`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& PASSWD_FILE();

    ///
    /// @brief Returns the default name of the custom password file of the server.
    ///
    /// @return The file name, `ecf.custom_passwd`, before the `<host>.<port>` prefix is added.
    ///
    static const std::string& CUSTOM_PASSWD_FILE();

    ///
    /// @brief Returns the default interval at which the server checks dependencies and submits jobs.
    ///
    /// @return The interval in seconds (`ECF_INTERVAL`), 60.
    ///
    static int SUBMIT_INTERVAL();

    ///
    /// @brief Returns the default threshold above which the generation of a task job is reported.
    ///
    /// @return The threshold in milliseconds (`ECF_TASK_THRESHOLD`), 4000.
    ///
    static int TASK_THRESHOLD();

    ///
    /// @brief Returns the default age beyond which the edit history of nodes is pruned when a checkpoint is loaded.
    ///
    /// @return The age in days (`ECF_PRUNE_NODE_LOG`), 30.
    ///
    static int PRUNE_NODE_LOG();

private:
    static bool server_;
    static bool debug_equality_;
    static unsigned int debug_level_;
    static atomic_counter_t state_change_no_;
    static atomic_counter_t modify_change_no_;
};

///
/// @brief Restores the global state and modify change numbers on destruction.
///
/// Captures both numbers on construction, so that the changes made within the scope of the object leave the numbers
/// as they were.
///
class EcfPreserveChangeNo {
public:
    ///
    /// @brief Captures the current state and modify change numbers.
    ///
    EcfPreserveChangeNo();

    ///
    /// @brief Restores the state and modify change numbers captured on construction.
    ///
    ~EcfPreserveChangeNo();

private:
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

///
/// @brief Enables the reporting of differences by the equality operators, for the scope of the object.
///
/// On destruction, reporting is disabled, and server variables are taken into account again by comparisons.
///
class DebugEquality {
public:
    ///
    /// @brief Enables the reporting of differences by the equality operators.
    ///
    DebugEquality() { Ecf::set_debug_equality(true); }

    // Disable copy (and move) semantics
    DebugEquality(const DebugEquality&)                  = delete;
    const DebugEquality& operator=(const DebugEquality&) = delete;
    DebugEquality(DebugEquality&&)                       = delete;
    DebugEquality& operator=(DebugEquality&&)            = delete;

    ///
    /// @brief Disables the reporting of differences, and takes server variables into account again.
    ///
    ~DebugEquality() {
        Ecf::set_debug_equality(false);
        set_ignore_server_variables(false);
    }

    ///
    /// @brief Indicates whether comparisons of server states ignore the server variables.
    ///
    /// @return True when the server variables are ignored.
    ///
    static bool ignore_server_variables() { return ignore_server_variables_; }

    ///
    /// @brief Sets whether comparisons of server states ignore the server variables.
    ///
    /// @param[in] flg True to ignore the server variables
    ///
    static void set_ignore_server_variables(bool flg) { ignore_server_variables_ = flg; }

private:
    static bool ignore_server_variables_;
};

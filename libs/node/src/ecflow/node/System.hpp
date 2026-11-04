// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

///
/// \brief Works with class Signal
///
/// When a child process terminates it does not disappear entirely.
/// Instead, it becomes a ‘zombie process’ which is no longer capable of executing,
/// but which still has a PID and an entry in the process table.
/// This is indicated by the state code Z in ps or top.
///
/// The presence of a moderate number of zombie processes is not particularly harmful,
/// but they add unnecessary clutter that can be confusing to the administrator.
/// In extreme cases they could exhaust the number of available process table slots.
///
/// For these reasons, well-behaved programs should ensure that zombie processes are removed in a timely manner.
/// The process of eliminating zombie processes is known as ‘reaping’.
/// The simplest method is to call wait, but this will block the parent process if the child has not yet terminated.
/// Alternatives are to use waitpid to poll or SIGCHLD to reap asynchronously.
/// The Class uses waitpid.
///

#include <string>
#include <utility>
#include <vector>

#include <sys/types.h> // for pid_t

#include "ecflow/node/NodeFwd.hpp"

namespace ecf {

///
/// @brief Identifies how close_descriptors_from() closed the descriptors.
///
enum class DescriptorClosing {
    CloseRange, ///< A single close_range system call closed them
    Loop        ///< Every descriptor number below the limit on open files was closed in turn
};

///
/// @brief Closes every descriptor numbered @p first or above.
///
/// @details Meant for the child of fork(), before exec, where only async-signal-safe calls are allowed; it makes no
/// other. On Linux, where the close_range system call is known at build time, a single call closes the descriptors,
/// whatever their number. Where it is not, and when the call fails at run time (for example, with ENOSYS on a kernel
/// older than 5.9), every descriptor number from @p first up to sysconf(_SC_OPEN_MAX) is closed in turn, whose cost
/// grows with that limit. Either way, no descriptor numbered @p first or above remains open.
///
/// @param first The lowest descriptor number to close.
/// @param allow_close_range When false, every descriptor number is closed in turn, even where close_range is
/// available; used to test that path.
/// @return The method that closed the descriptors.
///
DescriptorClosing close_descriptors_from(int first, bool allow_close_range = true);

/// Job submission in ECF is a two phase step.
/// phase 1: Spawn of ECF_JOB_CMD
/// phase 2: Invocation of ECF_JOB_CMD, this creates the *real* job which communicates with the server
/// For TEST     :ECF_JOB_CMD = "%ECF_JOB% 1> %ECF_JOBOUT% 2>&1
///     this collapses phase 1 and 2, to a single step
/// For Operation:ECF_JOB_CMD = ecf_submit %USER% %REMOTE_HOST% %ECFJOB% %ECFJOBOUT%'
///     This uses ecf_submit,This spawns the process to the different load levellers depending on OS, etc.
///   and hence involves at least 2 process.
/// This class handles phase 1, we capture the death of the child process
/// and hence this class will not in operations handle the death of the real job
/// created by ecf_submit.

class System {
private:
    System();

public:
    // Disable copy (and move) semantics
    System(const System&)            = delete;
    System& operator=(const System&) = delete;
    System(System&&)                 = delete;
    System& operator=(System&&)      = delete;

    ~System();

    static System* instance();

    /// Destroy the singleton. used in test only, to avoid valgrind issues
    static void destroy();

    ///
    /// @brief Holds the configuration of System, set by the server from its settings.
    ///
    /// Every member has a default that leaves the log as without the configuration, so that a user of libs/node other
    /// than the server is unaffected. A finer setting is added as a further member, with its own default, without
    /// changing set_configuration().
    ///
    struct Configuration
    {
        /// Whether the spawn, context and termination records of the spawned commands are written to the log, as
        /// `EXT` records; set by `/ecflow/server/log/commands/enabled` in `<host>.<port>.ecf.cfg`.
        bool log_commands = false;
    };

    ///
    /// @brief Sets the configuration, which applies to the commands spawned from then on.
    ///
    /// @param[in] configuration The configuration; discarded with the instance by destroy().
    ///
    void set_configuration(const Configuration& configuration) { configuration_ = configuration; }

    ///
    /// @brief Returns the configuration.
    ///
    /// @return The configuration; the defaults of Configuration until set_configuration() is called.
    ///
    const Configuration& configuration() const { return configuration_; }

    /// Let the server set this. Typically only set once, however in test  can be many times
    /// Note:: In test the Defs file in the server can be cleared, i.e. for each new test
    ///        Hence we maintain a weak_ptr to the Defs.
    void setDefs(const defs_ptr& defs) { defs_ = defs; }

    // return true, if command can be spawned, else false.
    // For jobs, We cannot store reference to Task*, as future functionality like
    // auto-migrate, etc, means we may end up pointing to garbage.
    // so instead we will store absNodePath. For other commands this can be empty
    enum CmdType { ECF_JOB_CMD, ECF_KILL_CMD, ECF_STATUS_CMD };

    ///
    /// @brief The name and the value of each variable substituted to render a command, in the order of first use.
    ///
    using Context = std::vector<std::pair<std::string, std::string>>;

    ///
    /// @brief Spawns a command, without waiting for its termination.
    ///
    /// The command is logged as @p cmdToLog, never as @p cmdToSpawn, so that the log holds no credential the server
    /// can recognise: the caller renders @p cmdToLog with the value of every credential variable masked (see
    /// Node::SubstitutionRecord), and System masks it further with ecf::mask_credentials(), for the credentials
    /// written literally. A secret under any other name, in any other form, is logged as it is. @p cmdToLog is
    /// kept with the process, for the records written at its termination.
    ///
    /// When Configuration::log_commands is set, the spawn is followed by an `EXT` spawn record holding @p cmdToLog,
    /// with its line breaks written as `\n`, and an `EXT` context record holding @p context as a JSON object, e.g.
    /// `{"QUEUE":"nf","OPTS":"-q nf --x=1"}`, each value masked by ecf::mask_credentials(); each record is one line.
    /// A name given more than once keeps its position and its last value.
    ///
    /// @param[in] cmd_type The kind of command
    /// @param[in] cmdToSpawn The command, as rendered, run by `/bin/sh -c`
    /// @param[in] cmdToLog The command as rendered for the log, with the value of every credential variable masked
    /// @param[in] context The variables substituted to render the command, with the value of every credential
    /// variable masked
    /// @param[in] absPath The path of the node of the command
    /// @param[out] errorMsg Receives the reason of a failure to fork
    /// @return True when the command was spawned.
    ///
    bool spawn(CmdType cmd_type,
               const std::string& cmdToSpawn,
               const std::string& cmdToLog,
               const Context& context,
               const std::string& absPath,
               std::string& errorMsg);

    // Handle children that have stopped,aborted or terminated, etc.
    // The signal handler is kept as light as possible, since it is re-entrant.
    // So Signal handles stores the termination state which handled later
    // by processTerminatedChildren. Typically, when we un-block SIGCHILD
    void processTerminatedChildren();

    /// returns the number of active process.
    /// for debug only
    int process() const;

private:
    /// Install signals that can catch signal from child process termination
    static void catchChildProcessTermination();

    // When a process terminates abnormally. This function is used to find the
    // associated task, and set it to the abort state.
    // Relies on the stored Defs ptr. which was set in the server
    void died(const std::string& absNodePath, CmdType, const std::string& reason);

    /// Does the real work of spawning children; returns 0 and the pid of the child on success
    int sys(CmdType,
            const std::string& cmdToSpawn,
            const std::string& cmdToLog,
            const std::string& absPath,
            pid_t& child_pid,
            std::string& errorMsg);

    static std::string cmd_type(CmdType);

private:
    weak_defs_ptr defs_; // weak_ptr is an observer of a shared_ptr
    Configuration configuration_;
    static System* instance_;
};

} // namespace ecf

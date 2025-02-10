/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_System_HPP
#define ecflow_node_System_HPP

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

#include "ecflow/node/NodeFwd.hpp"

namespace ecf {

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
public:
    // Disable copy (and move) semantics
    System(const System&)                  = delete;
    const System& operator=(const System&) = delete;

    static System* instance();

    /// Destroy the singleton. used in test only, to avoid valgrind issues
    static void destroy();

    /// Let the server set this. Typically only set once, however in test  can be many times
    /// Note:: In test the Defs file in the server can be cleared, i.e. for each new test
    ///        Hence we maintain a weak_ptr to the Defs.
    void setDefs(const defs_ptr& defs) { defs_ = defs; }

    // return true, if command can be spawned, else false.
    // For jobs, We can't store reference to Task*, as future functionality like
    // auto-migrate, etc, means we may end up pointing to garbage.
    // so instead we will store absNodePath. For other commands this can be empty
    enum CmdType { ECF_JOB_CMD, ECF_KILL_CMD, ECF_STATUS_CMD };
    bool spawn(CmdType, const std::string& cmdToSpawn, const std::string& absPath, std::string& errorMsg);

    // Handle children that have stopped,aborted or terminated, etc.
    // The signal handler is kept as light as possible, since it is re-entrant.
    // So Signal handles stores the termination state which handled later
    // by processTerminatedChildren. Typically, when we un-block SIGCHILD
    void processTerminatedChildren();

    /// returns the number of active process.
    /// for debug only
    int process() const;

private:
    ~System();
    System();

    /// Install signals that can catch signal from child process termination
    static void catchChildProcessTermination();

    // When a process terminates abnormally. This function is used to find the
    // associated task, and set it to the abort state.
    // Relies on the stored Defs ptr. which was set in the server
    void died(const std::string& absNodePath, CmdType, const std::string& reason);

    /// Does the real work of spawning children
    int sys(CmdType, const std::string& cmdToSpawn, const std::string& absPath, std::string& errorMsg);

    static std::string cmd_type(CmdType);

private:
    weak_defs_ptr defs_; // weak_ptr is an observer of a shared_ptr
    static System* instance_;
};

} // namespace ecf

#endif /* ecflow_node_System_HPP */

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cerrno>
#include <csignal>

#include <sys/wait.h> // for waitpid

#ifndef O_WRONLY
    #include <fcntl.h>
#endif

#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/System.hpp"

// #define DEBUG_FORK 1
// #define DEBUG_CATCH_CHILD 1
// #define DEBUG_TERMINATED_CHILD 1
// #define DEBUG_CHILD_ABORT 1

#ifdef DEBUG_FORK
    #include <iostream>
#endif

using namespace std;

namespace ecf {

// ===========================================================================
// Process
// ===========================================================================
struct Process
{
public:
    Process(const std::string& absPath, const std::string& cmdToSpawn, System::CmdType cmd_type, pid_t pid)
        : absNodePath_(absPath),
          cmd_(cmdToSpawn),
          cmd_type_(cmd_type),
          have_status_(0),
          pid_(pid),
          status_(0) {}

    std::string absNodePath_;  // Path to Task(ECF_JOB_CMD), empty for ECF_KILL_CMD & ECF_STATUS_CMD
    std::string cmd_;          // the command that was spawned
    System::CmdType cmd_type_; // Type of command
    sig_atomic_t have_status_; // Nonzero if this process has stopped or terminated.  */
    pid_t pid_;                // The process ID of this child.
    int status_;               // The status of this child; 0 if running,
                               // otherwise a status value from waitpid
};
std::vector<Process> processVec_;

/* Nonzero means some child's status has changed
   so look at process_list for the details.  */
volatile int process_status_change_ = 0;

// ===========================================================================
// System
// ===========================================================================
System* System::instance_ = nullptr;

System* System::instance() {
    if (instance_ == nullptr) {

        // Block SIGCHLD so that we control, when child process termination is handled
        ecf::Signal::block_sigchild();

        // install signal handler, that will catch Child process termination
        // The install function can be called asynchronously, and hence deserves
        // special consideration. Currently we temporarily unblock pending
        // SIGCHLD signals at the end of Job generation, and then block them again.
        // During the brief moment between unblock/block we expect
        // the handler to be called.
        catchChildProcessTermination();

        instance_ = new System();
    }
    return instance_;
}

void System::destroy() {
    delete instance_;
    instance_ = nullptr;
}

System::System()  = default;
System::~System() = default;

bool System::spawn(System::CmdType cmd_type,
                   const std::string& cmdToSpawn,
                   const std::string& absPath,
                   std::string& errorMsg) {
#ifdef DEBUG_FORK
    LogToCout logToCoutAsWell;
    LOG(Log::DBG, "  System::spawn path(" << absPath << ") cmd(" << cmdToSpawn << ") cmd_type(" << cmd_type << ")\n");
#endif

    std::string msg;
    if (sys(cmd_type, cmdToSpawn, absPath, msg)) {
        std::stringstream ss;
        ss << "Child process creation failed( " << msg << ") for command " << cmdToSpawn;
        if (!absPath.empty())
            ss << " at path(" << absPath << ")";
        errorMsg = ss.str();
#ifdef DEBUG_FORK
        LOG(Log::DBG, "  System::spawn returning false\n");
#endif
        return false;
    }
    return true;
}

int System::sys(System::CmdType cmd_type,
                const std::string& cmdToSpawn,
                const std::string& absPath,
                std::string& errorMsg) {
#ifdef DEBUG_FORK
    LogToCout logToCoutAsWell;
    LOG(Log::DBG, "  System::sys path(" << absPath << ")  cmd(" << cmdToSpawn << ")  cmd_type(" << cmd_type << ")\n");
#endif
    /**************************************************************************
     ?  Execute the command (cmdToSpawn) and return, DO NOT WAIT for the termination
     |  of the children.
     |  The stdin, stdout and stderr are closed (or redirected to /dev/null)
     =  PID in case of success or 0 in case of errors.
     ************************************o*************************************/
    pid_t child_pid;
    if ((child_pid = fork()) == 0) { /* The child */

        int f;
        close(2);
        if ((f = open("/dev/null", O_WRONLY)) != 2)
            close(f);

        close(1);
        if ((f = open("/dev/null", O_WRONLY)) != 1)
            close(f);

        close(0);
        if ((f = open("/dev/null", O_RDONLY)) != 0)
            close(f);

        // ==============================================================================
        // Ideally we should close all open file descriptors in the child process
        //    On Linux: sysconf(_SC_OPEN_MAX); returns 1024
        //    This means making 1024 - 3 system calls
        // This is especially import for socket descriptors, since if the server goes down
        // The children/zombies will prevent the server restart on the same port.
        // i.e the classic Address in use
        // Its not clear how big a performance issue this, an alternative would be, to only close
        // open socket file descriptors. But this will require a singleton of some sort
        // ===============================================================================
        int fd_limit = sysconf(_SC_OPEN_MAX);
        for (int i = 3; i < fd_limit; i++)
            close(i);

        execl("/bin/sh", "sh", "-c", cmdToSpawn.c_str(), (char*)nullptr);
        /*
         *  Maybe the file protection failed (no executable bit set)
         *  or the shell couldn't be found. Look at man execve(2).
         */
        _exit(127);
    }

    if (child_pid == -1) {
        std::stringstream ss;
        ss << "fork() error(" << strerror(errno) << ")";
        errorMsg = ss.str();
        return 1;
    }

    // Store the process pid, so that we can wait for it. ho ho.
    processVec_.emplace_back(absPath, cmdToSpawn, cmd_type, child_pid);

#ifdef DEBUG_FORK
    // LogToCout logToCoutAsWell;
    LOG(Log::DBG, "   submit: Path(" << absPath << ") child_pid(" << child_pid << ") cmd(" << cmdToSpawn << ")");
#endif
    return 0;
}

static void catch_child(int sig)
/**************************************************************************
?  Catch the death of the child process
|  This function can be called asynchronously hence it could interfere
   with the rest of program. Hence this function does not allocate or
   free memory. We simply store the status, for later processing
************************************o*************************************/
{
#ifdef DEBUG_CATCH_CHILD
    std::cout << "   catch_child (process death) sig = " << sig << endl;
#endif

    int saved_errno = errno; // save error number since waitpid can change this
    int status;
    pid_t child_pid;

    // waitpid returns:
    //  - on success, returns the process ID of the child whose state has changed
    //  - if WNOHANG was specified and one or more child(ren) specified by pid exist,
    //    but have *NOT* yet changed state, then *0* is returned
    //    *** hence we MUST check for 0, other wise we will end up in an infinite loop **
    //  - returns -1 on error
    while ((child_pid = waitpid(-1, &status, WNOHANG)) != -1 && child_pid != 0) {

        auto theEnd = processVec_.end();
        for (auto i = processVec_.begin(); i != theEnd; ++i) {
            if ((*i).pid_ == child_pid) {

#ifdef DEBUG_CATCH_CHILD
                std::cout << "   catch_child Found pid " << child_pid << endl;
#endif
                // Indicate that the status field
                // has data to look at.  We do this only after storing it.
                (*i).have_status_ = 1;

                // store the status
                (*i).status_ = status;

                // The program should check this flag from time to time
                // to see if there is any news in processVec_.
                process_status_change_++;
                break;
            }
        }
    }

    // restore error number
    errno = saved_errno;
}

void System::processTerminatedChildren() {
#ifdef DEBUG_TERMINATED_CHILD
    std::cout << "System::processTerminatedChildren() process_status_change_ = " << process_status_change_
              << "   processVec_.size() = " << processVec_.size() << endl;
    LogToCout logToCoutAsWell;
#endif
    if (process_status_change_ == 0) {
        return;
    }

    // Must be the first thing we do.
    process_status_change_ = 0;

    std::vector<Process>::iterator i;
    for (i = processVec_.begin(); i != processVec_.end(); ++i) {

        if ((*i).have_status_) {

#ifdef DEBUG_TERMINATED_CHILD
            std::cout << "System::processTerminatedChildren(): " << System::cmd_type((*i).cmd_type_) << " path("
                      << (*i).absNodePath_ << ") pid(" << (*i).pid_ << ") has status(stopped or terminated)" << endl;
#endif
            // exit status is one of mutually exclusive [ WIFEXITED | WIFSIGNALED | WIFSTOPPED | WIFCONTINUED ]
            if (WIFEXITED((*i).status_)) {

                // *Normal* termination via exit
                if (WEXITSTATUS((*i).status_)) {
                    // exit is non zero.
                    std::stringstream ss;
                    ss << System::cmd_type((*i).cmd_type_) << " PID(" << (*i).pid_ << ") path(" << (*i).absNodePath_
                       << ") exited with status " << WEXITSTATUS((*i).status_) << " [ " << (*i).cmd_ << " ]";
                    died((*i).absNodePath_, (*i).cmd_type_, ss.str());
                }
                else {
                    // exit(0) child terminated normally
#ifdef DEBUG_TERMINATED_CHILD
                    LOG(Log::DBG,
                        System::cmd_type((*i).cmd_type_)
                            << " PID(" << (*i).pid_ << ") exited normally [ " << (*i).cmd_ << " ]");
#endif
                }

                // remove the process since it has terminated
                processVec_.erase(i--);
            }
            else if (WIFSIGNALED((*i).status_)) {

                // *abnormal* child process terminated by a signal
                std::stringstream ss;
                ss << System::cmd_type((*i).cmd_type_) << " PID(" << (*i).pid_ << ") path(" << (*i).absNodePath_
                   << ") died of signal " << WTERMSIG((*i).status_) << " [ " << (*i).cmd_ << " ]";
                died((*i).absNodePath_, (*i).cmd_type_, ss.str());

                // remove the process since it has terminated
                processVec_.erase(i--);
            }
            else if (WIFSTOPPED((*i).status_)) {

                LOG(Log::WAR,
                    System::cmd_type((*i).cmd_type_) << " PID(" << (*i).pid_ << ") STOPPED? [ " << (*i).absNodePath_
                                                     << " ] [ " << (*i).cmd_ << " ]");
            }
            else {

                // Can only be WIFCONTINUED. (XSI extension to POSIX)
                LOG(Log::WAR,
                    System::cmd_type((*i).cmd_type_) << " PID(" << (*i).pid_ << ") CONTINUED? [ " << (*i).absNodePath_
                                                     << " ] [ " << (*i).cmd_ << " ]");
            }
        }
        else {
#ifdef DEBUG_TERMINATED_CHILD
            LOG(Log::DBG,
                System::cmd_type((*i).cmd_type_)
                    << " stray PID(" << (*i).pid_ << ") (ignored) [ " << (*i).cmd_ << " ]");
#endif
        }
    }

#ifdef DEBUG_TERMINATED_CHILD
    std::cout << "System::processTerminatedChildren() process size = " << processVec_.size() << endl;
#endif
}

int System::process() const {
    return static_cast<int>(processVec_.size());
}

// ============================================================================
// See: Advanced programming in the UNIX environment: Page 328
// Note: with sigaction the handle stays installed, until changed
//       this is different to the signal(..) which on some system needs
//       to be reinstalled at start/end of SignalFunction.
// ============================================================================
typedef void SignalFunction(int);
SignalFunction* signal_(int signo, SignalFunction* func) {
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else {
        // We intentionally try to set the SA_RESTART flag for all signals other than SIGALRM
        // so that any system call interrupted by these other signals in automatically restarted
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }

    if (sigaction(signo, &act, &oact) < 0)
        return (SIG_ERR);

    // Return the old handler
    return oact.sa_handler;
}

void System::catchChildProcessTermination() {
    // Call our local version (which uses sigaction) rather than the out date signal(..)
    signal_(SIGCHLD, catch_child);
}

void System::died(const std::string& absNodePath, CmdType cmd_type, const std::string& reason)
/**************************************************************************
 ?  Process the death of the process. This is most unwanted and implies
 |  that the shell died abnormally.
 ************************************o*************************************/
{
#ifdef DEBUG_CHILD_ABORT
    std::cout << "System::died path = '" << absNodePath << "'" << endl;
#endif

    /// always write to log, before returning
    ecf::log(Log::ERR, reason);

    /// If the Path is empty, then this could be something *OTHER THAN* job submission
    /// Typically testing, don't assert
    if (absNodePath.empty()) {
        return;
    }

    defs_ptr defs = defs_.lock();
    if (!defs.get()) {
        LOG_ASSERT(defs.get(), "System::died, defs not defined ???");
        return;
    }

    node_ptr node = defs->findAbsNode(absNodePath);
    if (!node.get()) {
#ifdef DEBUG_CHILD_ABORT
        std::cout << "System::died " << absNodePath << " could not be found in defs \n";
#endif
        return;
    }

    Submittable* submittable = node->isSubmittable();
    if (!submittable) {
#ifdef DEBUG_CHILD_ABORT
        std::cout << "System::died " << absNodePath << " path is NOT a Task or Alias \n";
#endif
        return;
    }

    // This function can get called at any time.
    // AND out of context of any command, hence we must handle case where Suite handles are used.
    // Otherwise the view will not know about aborted states.
    // ECFLOW-104 aborted state for a task following an error at submission
    SuiteChanged1 changed(submittable->suite());

    // Only failure of the JOB command should cause SUBMITTABLE to abort.
    // i.e Badly formed status and kill commands should not abort a running job
    switch (cmd_type) {
        case System::ECF_JOB_CMD: {
            submittable->flag().set(ecf::Flag::JOBCMD_FAILED);

            if (submittable->state() == NState::COMPLETE || submittable->state() == NState::ACTIVE) {
                // ABNORMAL flow
                // Occasionally TRIMURTI can time out and fail, even when the job is active or complete.(i.e for super
                // fast jobs) queued->submitted->active->complete  See: ECFLOW-1589 In this case we should *AVOID*
                // setting node to aborted.
                std::string str = "System::died: ECF_JOB_CMD *failed*, but state is ";
                str += NState::toString(submittable->state());
                str += "(";
                str += absNodePath;
                str += ") setting zombie flag";
                ecf::log(Log::ERR, str);
                submittable->flag().set(ecf::Flag::ZOMBIE);
#ifdef DEBUG_CHILD_ABORT
                std::cout << str << "\n";
#endif
                return;
            }

#ifdef DEBUG_CHILD_ABORT
            std::cout << "System::died aborting task " << absNodePath << "\n";
#endif
            // NORMAL flow   :  queued->submitted->abort  : for failing ECF_JOB_CMD
            //
            // Set state aborted since the job terminated abnormally, and provide a reason
            submittable->aborted(reason);
            break;
        }
        case System::ECF_KILL_CMD:
            submittable->flag().set(ecf::Flag::KILLCMD_FAILED);
            break;
        case System::ECF_STATUS_CMD:
            submittable->flag().set(ecf::Flag::STATUSCMD_FAILED);
            break;
    };
}

std::string System::cmd_type(System::CmdType cmd_type) {
    switch (cmd_type) {
        case System::ECF_JOB_CMD:
            return "ECF_JOB_CMD";
        case System::ECF_KILL_CMD:
            return "ECF_KILL_CMD";
        case System::ECF_STATUS_CMD:
            return "ECF_STATUS_CMD";
        default:
            assert(false);
    }
    return "ECF_JOB_CMD";
}

} // namespace ecf

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/CtsWaitCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

// #define DEBUG_ZOMBIE 1

bool TaskCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<TaskCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (path_to_submittable_ != the_rhs->path_to_node())
        return false;
    if (jobs_password_ != the_rhs->jobs_password())
        return false;
    if (process_or_remote_id_ != the_rhs->process_or_remote_id())
        return false;
    if (try_no_ != the_rhs->try_no())
        return false;
    return ClientToServerCmd::equals(rhs);
}

// **********************************************************************************
// IMPORTANT: In the current SMS/ECF only the init child command, passes the
// process_or_remote_id_, for *ALL* other child commands this is empty.
// The Automated tests get round this via setting ECF_RID in the header/tail job includes
// However since this may not be in .sms/.ecf files, we cannot rely on it
// Hence we need to be able to handle *EMPTY* process_or_remote_id_ for child commands
//
//  Task State | Child                      | Log | Explanation
//  -------------------------------------------------------------------------------------------------------------
//  SUBMITTED  | child!=INIT                | ERR | Script has child command in back ground, Bug in script, out of order
//  ACTIVE     | INIT & pid & passwd match  | WAR | two init commands, overloaded server, or 2*init in script, *FOB*.
//  Forgive ACTIVE     | INIT & pid & passwd !match | ERR | two init commands, Task started again by user. COMPLETE   |
//  COMPLETE                   | WAR | two complete, zombie, overloaded server. *FOB*, Allow job to complete. Forgive.
//  COMPLETE   | child != COMPLETE          | ERR | zombie
//  ABORTED    | ABORTED                    | WAR | two aborted, zombie, overloaded server. *FOB*, allow process to
//  exit. Forgive ABORTED    | child != ABORTED           | WAR | zombie
//  -------------------------------------------------------------------------------------------------------------------
//
//  zombie type    |   PID   | password | explanation
//  ---------------------------------------------------------------------------------------------------------------
//  ECF_PID        |    X    | matches  | PID miss-match, but password matches. Job scheduled twice. Check submitter
//  ECF_PID_PASSWD |    X    |     X    | Both PID and password miss-match. Re-queue & submit of active job
//  ECF_PASSWD     | matches |     X    | Password miss-match, PID matches, system has re-cycled PID or hacked job file?
//  ECF            | matches | matches  | overloaded server,Two init commands or task complete or aborted but receives
//  another child cmd USER           |    ?    |     ?    | User initiated zombie whilst task was active or submitted,
//  command is recorded in zombie PATH           |   n/a   |    n/a   | Task not found. Nodes replaced whilst jobs were
//  running
//  ----------------------------------------------------------------------------------------------------------------
bool TaskCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& theReply) const {
#ifdef DEBUG_ZOMBIE
    std::cout << "   TaskCmd::authenticate " << Child::to_string(child_type());
    std::cout << " " << path_to_submittable_ << " " << process_or_remote_id_ << " " << jobs_password_ << " " << try_no_;
    const Zombie& zombie = as->zombie_ctrl().find(path_to_submittable_, process_or_remote_id_, jobs_password_);
    if (!zombie.empty())
        std::cout << "  " << zombie;
    else {
        const Zombie& zombiep = as->zombie_ctrl().find_by_path_only(path_to_submittable_);
        if (!zombiep.empty())
            std::cout << "  find_by_path_only: " << zombiep;
    }
#endif
    /// ***************************************************************************
    /// Task based cmd have their own authentication mechanism, hence we
    /// Don't need to call the base class authenticate
    /// **************************************************************************

    if (!as->allowTaskCommunication()) {
        // This is not an Error, hence we don't throw exception
        theReply = PreAllocatedReply::block_client_server_halted_cmd();
        return false;
    }

    submittable_ = get_submittable(as);
    if (!submittable_) {
#ifdef DEBUG_ZOMBIE
        std::cout << ": PATH Zombie\n";
#endif
        // Create path zombie, if not already created:
        std::string action_taken;
        static_cast<void>(as->zombie_ctrl().handle_path_zombie(as, this, action_taken, theReply));

        // distinguish output by using *path*
        std::stringstream ss;
        ss << " zombie(*path*) : chd:" << ecf::Child::to_string(child_type()) << " : " << path_to_submittable_ << " : "
           << process_or_remote_id_ << " : " << jobs_password_ << " : action(" << action_taken << ")";
        log(Log::ERR, ss.str());
        return false;
    }

    // If the CMD *WAS* created with Submittable::DUMMY_JOBS_PASSWORD then we were testing
    // This will be copied from client to server, hence we want to avoid same check in the
    // server. when handleRequest is called()
    // DO NOT place #ifdef DEBUG otherwise test will start failing for the release build
    if (jobs_password_ == Submittable::DUMMY_JOBS_PASSWORD()) {
        return true;
    }

    SuiteChanged1 changed(submittable_->suite());

    /// Check if User wants to explicitly bypass password checking
    /// This can be done via AlterCmd by adding a variable on the task,  ECF_PASS with value
    /// Submittable::FREE_JOBS_PASSWORD Note: this *does not* look for the variable up the node tree, only on the task.
    std::string ecf_pass_value;
    if (submittable_->findVariableValue(ecf::environment::ECF_PASS, ecf_pass_value)) {

        if (ecf_pass_value == Submittable::FREE_JOBS_PASSWORD()) {
            submittable_->get_flag().clear(ecf::Flag::ZOMBIE);
            return true;
        }
    }

    /// Handle corner case ,where we have two jobs with different process id, but same password
    /// Can happen if jobs is started externally,
    /// or via test(i.e submit 1,submit 2(force)) before job1 active its password is overridden
    bool submittable_allready_aborted  = false;
    bool submittable_allready_active   = false;
    bool submittable_allready_complete = false;
    password_missmatch_                = false;
    pid_missmatch_                     = false;

    /// *** In complete state, the password is empty.
    if (submittable_->jobsPassword() != jobs_password_) {
#ifdef DEBUG_ZOMBIE
        std::cout << ": submittable pass(" << submittable_->jobsPassword() << ") != jobs_password_(" << jobs_password_
                  << ")";
#endif
        password_missmatch_ = true;
    }

    /// When state is in SUBMITTED, its process/remote_id is EMPTY. It will be set by the INIT child command.
    /// Hence we can *NOT* mark it as pid_missmatch.
    ///
    /// *** See Note above: Not all child commands pass a process_id. ***
    /// *** Hence this test for zombies is ONLY valid if process sets the process_or_remote_id_ ****
    /// *** User should really set ECF_RID in the scripts
    /// *** In submitted state, the pid is empty. hence clear pid_missmatch_ below
    /// *** In complete state, the pid is empty.  hence clear pid_missmatch_ below
    if (!submittable_->process_or_remote_id().empty() && !process_or_remote_id_.empty() &&
        submittable_->process_or_remote_id() != process_or_remote_id_) {
#ifdef DEBUG_ZOMBIE
        std::cout << ":task pid(" << submittable_->process_or_remote_id() << ") != process pid("
                  << process_or_remote_id_ << ")";
#endif
        pid_missmatch_ = true;
    }

    switch (submittable_->state()) {
        case NState::SUBMITTED: {
            // The pid on the task will be empty
            if (child_type() != Child::INIT) {
                std::stringstream ss;
                ss << path_to_submittable_
                   << " When a node is submitted, expected next child command to be INIT but received "
                   << Child::to_string(child_type());
                log(Log::ERR, ss.str());
            }
            break;
        }

        case NState::ACTIVE: {
            if (child_type() == Child::INIT) {
#ifdef DEBUG_ZOMBIE
                std::cout << ":(child_type() == Child::INIT) && submittable_->state() == NState::ACTIVE)";
#endif
                // *IF* password and pid matches be more forgiving. How can this case arise:
                // i.e server is heavily overloaded, client calls init, which times out because server is busy
                // Client then sends init again. In this case rather than treating it as a zombie, we will let it
                // through providing the password and pid matches.
                if (!password_missmatch_ && !pid_missmatch_) {
                    string ret = " [ overloaded || --init*2 ](pid and passwd match) : chd:";
                    ret += ecf::Child::to_string(child_type());
                    ret += " : ";
                    ret += path_to_submittable_;
                    ret += " : already active : action(fob)";
                    log(Log::WAR, ret);
                    theReply = PreAllocatedReply::ok_cmd();
                    return false;
                }
                submittable_allready_active = true;
            }
            break;
        }

        case NState::COMPLETE: {
#ifdef DEBUG_ZOMBIE
            std::cout << ": submittable_->state() == NState::COMPLETE)";
#endif
            if (child_type() == Child::COMPLETE) {
                // Note: when a node completes, we clear tasks password and pid, to save memory on checkpt & network
                // bandwidth (We could choose not to clear, This would allow us to disambiguate between 2/ and 3/
                // below). HOWEVER:
                //
                // How can this situation arise:
                //   1/ Two calls to --complete  (rare)
                //   2/ Overloaded server. Client send --complete to server, but it is overload and does not respond,
                //   the client then
                //      times out. Server handles the request. When client tries again we get here. (possible)
                //   3/ Zombie, two separate process. (possible, typically done by user action)
                //
                // For all three it should be safe to just fob:
                //   1/ Two calls to --complete # Be forgiving
                //   2/ Overloaded server       # The correct course of action
                //   3/ zombie                  # The zombie has completed anyway, don't bother blocking it

                submittable_->get_flag().clear(ecf::Flag::ZOMBIE);
                as->zombie_ctrl().remove_by_path(path_to_submittable_);

                string ret = " [ overloaded || zombie || --complete*2 ] : chd:";
                ret += ecf::Child::to_string(child_type());
                ret += " : ";
                ret += path_to_submittable_;
                ret += " : already complete : action(fob)";
                log(Log::WAR, ret);
                theReply = PreAllocatedReply::ok_cmd();
                return false;
            }

            // If Task state is complete, and we receive **any** child command then it is a zombie
            submittable_allready_complete = true;
            password_missmatch_           = false;
            pid_missmatch_                = false;
            break;
        }

        case NState::ABORTED: {
#ifdef DEBUG_ZOMBIE
            std::cout << ": submittable_->state() == NState::ABORTED)";
#endif

            if (child_type() == Child::ABORT) {

                if (!password_missmatch_ && !pid_missmatch_) {

                    as->zombie_ctrl().remove(submittable_);

                    string ret = " [ overloaded || --abort*2 ] (pid and passwd match) : chd:";
                    ret += ecf::Child::to_string(child_type());
                    ret += " : ";
                    ret += path_to_submittable_;
                    ret += " : already aborted : action(fob)";
                    log(Log::WAR, ret);
                    theReply = PreAllocatedReply::ok_cmd();
                    return false;
                }
            }

            // If Task state is aborted, and we receive **any** child command then it is a zombie
            submittable_allready_aborted = true;
            password_missmatch_          = false;
            pid_missmatch_               = false;
            break;
        }
        case NState::QUEUED:
            break; // WTF
        case NState::UNKNOWN:
            break; // WTF
    }

#ifdef DEBUG_ZOMBIE
    std::cout << "\n";
#endif

    if (password_missmatch_ || pid_missmatch_ || submittable_allready_active || submittable_allready_complete ||
        submittable_allready_aborted) {
        /// If the task has adopted we return true, and carry on as normal
        std::string action_taken;
        if (!as->zombie_ctrl().handle_zombie(submittable_, this, action_taken, theReply)) {

            // LOG failure: Include type of zombie.
            // ** NOTE **: the zombie may have been removed by user actions. i.e if fob and child cmd is abort |
            // complete, etc
            std::stringstream ss;
            ss << " zombie";
            const Zombie& theZombie =
                as->zombie_ctrl().find(path_to_submittable_, process_or_remote_id_, jobs_password_);
            if (!theZombie.empty())
                ss << "(" << theZombie.type_str() << ")";

            ss << " : chd:" << ecf::Child::to_string(child_type());
            ss << " : " << path_to_submittable_ << "(" << NState::toString(submittable_->state()) << ")";
            ss << " : " << process_or_remote_id_ << " : " << jobs_password_;
            if (submittable_allready_active)
                ss << " : already active";
            if (submittable_allready_complete)
                ss << " : already complete";
            if (submittable_allready_aborted)
                ss << " : already aborted";
            if (password_missmatch_)
                ss << " : passwd != [ task:" << submittable_->jobsPassword() << " child:" << jobs_password_ << " ]";
            if (pid_missmatch_)
                ss << " : pid != [ task:" << submittable_->process_or_remote_id() << " child:" << process_or_remote_id_
                   << " ]";
            ss << " : action(" << action_taken << ")";
            log(Log::ERR, ss.str());
            return false;
        }
    }
    return true;
}

Submittable* TaskCmd::get_submittable(AbstractServer* as) const {
    node_ptr node = as->defs()->findAbsNode(path_to_submittable_);
    if (!node.get()) {
        return nullptr;
    }

    return node->isSubmittable();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CtsWaitCmd::CtsWaitCmd(const std::string& pathToTask,
                       const std::string& jobsPassword,
                       const std::string& process_or_remote_id,
                       int try_no,
                       const std::string& expression)
    : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
      expression_(expression) {
    // Parse expression to make sure its valid
    static_cast<void>(Expression::parse(expression, "CtsWaitCmd:")); // will throw for errors
}

void CtsWaitCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "wait ";
    os += expression_;
    os += " ";
    os += path_to_node();
}

bool CtsWaitCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsWaitCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (expression_ != the_rhs->expression())
        return false;
    return TaskCmd::equals(rhs);
}

STC_Cmd_ptr CtsWaitCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_wait_++;

    SuiteChanged1 changed(submittable_->suite());

    // Parse the expression, should not fail since client should have already check expression parses
    // The complete expression have been parsed and we have created the abstract syntax tree
    // We now need CHECK the AST for path nodes, event and meter. repeats,etc.
    // *** This will also set the Node pointers ***
    // If the expression references paths that don't exist throw an error
    // This can be captured in the ecf script, which should then abort the task
    // Otherwise we will end up blocking indefinitely
    std::unique_ptr<AstTop> ast =
        submittable_->parse_and_check_expressions(expression_, true, "CtsWaitCmd:"); // will throw for errors

    // Evaluate the expression
    if (ast->evaluate()) {

        submittable_->get_flag().clear(ecf::Flag::WAIT);

        // expression evaluates, return OK
        return PreAllocatedReply::ok_cmd();
    }

    submittable_->get_flag().set(ecf::Flag::WAIT);

    // Block/wait while expression is false
    return PreAllocatedReply::block_client_on_home_server_cmd();
}

const char* CtsWaitCmd::arg() {
    return TaskApi::waitArg();
}
const char* CtsWaitCmd::desc() {
    return "Evaluates an expression, and block while the expression is false.\n"
           "For use in the '.ecf' file *only*, hence the context is supplied via environment variables\n"
           "  arg1 = string(expression)\n\n"
           "Usage:\n"
           "  ecflow_client --wait=\"/suite/taskx == complete\"";
}

void CtsWaitCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(CtsWaitCmd::arg(), po::value<string>(), CtsWaitCmd::desc());
}
void CtsWaitCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    std::string expression = vm[arg()].as<std::string>();

    if (clientEnv->debug())
        cout << "  CtsWaitCmd::create " << CtsWaitCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ") expression(" << expression << ")\n";

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("CtsWaitCmd: " + errorMsg);
    }

    cmd = std::make_shared<CtsWaitCmd>(clientEnv->task_path(),
                                       clientEnv->jobs_password(),
                                       clientEnv->process_or_remote_id(),
                                       clientEnv->task_try_no(),
                                       expression);
}

std::ostream& operator<<(std::ostream& os, const CtsWaitCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CtsWaitCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CtsWaitCmd)

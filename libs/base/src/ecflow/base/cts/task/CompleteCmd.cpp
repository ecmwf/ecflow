/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/CompleteCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

void CompleteCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "complete ";
    os += path_to_node();
    if (!var_to_del_.empty()) {
        os += " --remove";
        for (const auto& var_to_del : var_to_del_) {
            os += " ";
            os += var_to_del;
        }
    }
}

bool CompleteCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CompleteCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (var_to_del_ != the_rhs->variables_to_delete())
        return false;
    return TaskCmd::equals(rhs);
}

STC_Cmd_ptr CompleteCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_complete_++;

    {
        /// If there is an associated zombie, remove from the list. Must match,
        /// Do this before task->complete(), since that clears password & process id
        /// remove(..) uses password/ process id to match the right zombie
        as->zombie_ctrl().remove(submittable_);

        // update suite change numbers before job submission, submittable_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());
        submittable_->complete(); // will set task->set_state(NState::COMPLETE);

        for (const auto& var_to_delete : var_to_del_) {
            submittable_->delete_variable_no_error(var_to_delete);
        }
    }

    // Do job submission in case any triggers dependent on NState::COMPLETE
    as->increment_job_generation_count();
    return PreAllocatedReply::ok_cmd();
}

const char* CompleteCmd::arg() {
    return TaskApi::completeArg();
}
const char* CompleteCmd::desc() {
    return "Mark task as complete. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n\n"
           "If this child command is a zombie, then the default action will be to *block*.\n"
           "The default can be overridden by using zombie attributes.\n"
           "Otherwise the blocking period is defined by ECF_TIMEOUT.\n"
           "The init command allows variables to be added, and complete command\n"
           "allows for them to be removed.\n"
           "  arg1(--remove)(optional) = a list of variables to removed from this task\n\n"
           "Usage:\n"
           "  ecflow_client --complete\n"
           "  ecflow_client --complete --remove name1 name2 # delete variables name1 and name2 on the task";
}

void CompleteCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(CompleteCmd::arg(), CompleteCmd::desc())(
        "remove", po::value<vector<string>>()->multitoken(), "remove variables i.e name name2");
}
void CompleteCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    if (clientEnv->debug())
        cout << "  CompleteCmd::create " << CompleteCmd::arg() << " task_path(" << clientEnv->task_path()
             << ") password(" << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id()
             << ") try_no(" << clientEnv->task_try_no() << ")\n";

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("CompleteCmd: " + errorMsg);
    }

    std::vector<std::string> variable_vec;
    if (vm.count("remove"))
        variable_vec = vm["remove"].as<vector<string>>();

    cmd = std::make_shared<CompleteCmd>(clientEnv->task_path(),
                                        clientEnv->jobs_password(),
                                        clientEnv->process_or_remote_id(),
                                        clientEnv->task_try_no(),
                                        variable_vec);
}

std::ostream& operator<<(std::ostream& os, const CompleteCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CompleteCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CompleteCmd)

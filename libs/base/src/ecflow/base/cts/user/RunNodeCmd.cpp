/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/RunNodeCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

bool RunNodeCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<RunNodeCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (paths_ != the_rhs->paths())
        return false;
    if (force_ != the_rhs->force())
        return false;
    return UserCmd::equals(rhs);
}

ecf::authentication_t RunNodeCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t RunNodeCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void RunNodeCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::run(paths_, force_)));
}
void RunNodeCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::run(paths_, force_));
}
void RunNodeCmd::print(std::string& os, const std::string& path) const {
    user_cmd(os, CtsApi::to_string(CtsApi::run(std::vector<std::string>(1, path), force_)));
}

STC_Cmd_ptr RunNodeCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().run_node_++;
    assert(isWrite()); // isWrite used in handleRequest() to control check pointing

    Defs* defs = as->defs().get();
    std::stringstream ss;
    size_t vec_size = paths_.size();
    for (size_t i = 0; i < vec_size; i++) {
        node_ptr node = find_node_for_edit_no_throw(defs, paths_[i]);
        if (!node.get()) {
            ss << "RunNodeCmd: Could not find node at path " << paths_[i] << "\n";
            LOG(Log::ERR, "RunNodeCmd: Could not find node at path " << paths_[i]);
            continue;
        }

        if (!node->suite()->begun()) {
            std::stringstream mss;
            mss << "RunNodeCmd failed: For " << paths_[i] << ". The suite " << node->suite()->name()
                << " must be 'begun' first\n";
            throw std::runtime_error(mss.str());
        }

        SuiteChangedPtr changed(node.get());

        // Please note: that if any tasks under theNode are in
        // active or submitted states, then we will have created zombies jobs
        // The GUI: that calls this command should call a separate request
        // the returns the active/submitted tasks first. This can then be
        // presented to the user, who can elect to kill them if required.
        bool createJobs = true;
        if (test_)
            createJobs = false;

        /// This will *NOT* timeout, unlike server Job generation
        JobsParam jobsParam(as->poll_interval(),
                            createJobs); // default here is to spawn jobs , spawn jobs = true
                                         // At the task level, if create jobs is false, we will not spawn jobs
#ifdef DEBUG_JOB_SUBMISSION
        jobsParam.logDebugMessage(" from RunNodeCmd::doHandleRequest");
#endif

        if (force_)
            as->zombie_ctrl().add_user_zombies(node.get(), CtsApi::runArg());

        // Avoid re-running the task again on the same time slot
        node->miss_next_time_slot();

        if (!node->run(jobsParam, force_)) {
            LOG(Log::ERR, "RunNodeCmd: Failed for " << paths_[i] << " : " << jobsParam.getErrorMsg());
        }
    }

    std::string error_msg = ss.str();
    if (!error_msg.empty()) {
        throw std::runtime_error(error_msg);
    }

    return PreAllocatedReply::ok_cmd();
}

// bool RunNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
//     return do_authenticate(as, cmd, paths_);
// }

const char* RunNodeCmd::arg() {
    return CtsApi::runArg();
}
const char* RunNodeCmd::desc() {
    return "Ignore triggers, limits, time or date dependencies, just run the Task.\n"
           "When a job completes, it may be automatically re-queued if it has a cron\n"
           "or multiple time dependencies. If we have multiple time based attributes,\n"
           "then each run, will expire the time.\n"
           "When we run before the time, we want to avoid re-running the task then\n"
           "a flag is set, so that it is not automatically re-queued.\n"
           "A repeat attribute is incremented when all the child nodes are complete\n"
           "in this case the child nodes are automatically re-queued.\n"
           "Hence this command can be aid, in allowing a Repeat attribute to be incremented\n"
           "  arg1 = (optional)force\n"
           "         Forcibly run, even if there are nodes that are active or submitted\n"
           "         This can result in zombie creation\n"
           "  arg2 = node path(s). The paths must begin with a leading '/' character.\n"
           "         If the path is /suite/family will recursively run all tasks\n"
           "         When providing multiple paths avoid running the same task twice\n"
           "Example:\n"
           "  --run=/suite/t1                    # run task t1\n"
           "Effect:\n"
           "     task t1; time 12:00             # will complete if run manually\n"
           "     task t2; time 10:00 13:00 01:00 # will run 4 times before completing\n"
           " When we have a time range(i.e as shown with task t2), then next time slot\n"
           " is incremented for each run, until it expires, and the task completes.\n"
           " Use the Why command, to show next run time (i.e. next time slot)";
}

void RunNodeCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(RunNodeCmd::arg(), po::value<vector<string>>()->multitoken(), RunNodeCmd::desc());
}
void RunNodeCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    vector<string> args = vm[RunNodeCmd::arg()].as<vector<string>>();

    if (ace->debug())
        dumpVecArgs(RunNodeCmd::arg(), args);

    std::vector<std::string> options, paths;
    split_args_to_options_and_paths(args, options, paths); // relative order is still preserved
    if (paths.empty()) {
        std::stringstream ss;
        ss << "RunNodeCmd: No paths specified. Paths must begin with a leading '/' character\n"
           << RunNodeCmd::desc() << "\n";
        throw std::runtime_error(ss.str());
    }

    bool force = false;
    if (!options.empty()) {
        if (options.size() != 1) {
            std::stringstream ss;
            ss << "RunNodeCmd: Invalid arguments. Expected a single optional 'force'\n" << RunNodeCmd::desc() << "\n";
            throw std::runtime_error(ss.str());
        }
        if (options[0].find("force") != std::string::npos)
            force = true;
        else {
            std::stringstream ss;
            ss << "RunNodeCmd: Expected force <path(s)>\n" << RunNodeCmd::desc() << "\n";
            throw std::runtime_error(ss.str());
        }
    }
    cmd = std::make_shared<RunNodeCmd>(paths, force);
}

std::ostream& operator<<(std::ostream& os, const RunNodeCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(RunNodeCmd)
CEREAL_REGISTER_DYNAMIC_INIT(RunNodeCmd)

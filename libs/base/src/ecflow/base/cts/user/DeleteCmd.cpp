/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/DeleteCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Task.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

DeleteCmd::DeleteCmd(const std::string& absNodePath, bool force) : group_cmd_(nullptr), force_(force) {
    if (!absNodePath.empty())
        paths_.push_back(absNodePath);
}

void DeleteCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::delete_node(paths_, force_)));
}

void DeleteCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::delete_node(paths_, force_));
}

void DeleteCmd::print(std::string& os, const std::string& path) const {
    std::vector<std::string> paths(1, path);
    user_cmd(os, CtsApi::to_string(CtsApi::delete_node(paths, force_)));
}

bool DeleteCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<DeleteCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (paths_ != the_rhs->paths())
        return false;
    if (force_ != the_rhs->force())
        return false;
    return UserCmd::equals(rhs);
}

ecf::authentication_t DeleteCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t DeleteCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

const char* DeleteCmd::theArg() const {
    return CtsApi::delete_node_arg();
}

STC_Cmd_ptr DeleteCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().node_delete_++;

    if (paths_.empty()) {
        if (!force_)
            check_for_active_or_submitted_tasks(as, nullptr);
        else
            as->zombie_ctrl().add_user_zombies(as->defs(), CtsApi::delete_node_arg());
        as->clear_defs();

        // If this command is part of a group command, let the following sync command, know about the new handle
        if (group_cmd_)
            group_cmd_->set_client_handle(0);

        // This will reset client defs and set client handle to zero on the client side.
        return PreAllocatedReply::delete_all_cmd();
    }
    else {
        std::stringstream ss;
        Defs* defs = as->defs().get();

        for (const auto& path : paths_) {

            node_ptr theNodeToDelete = defs->findAbsNode(path);
            if (!theNodeToDelete.get()) {
                ss << "DeleteCmd:Delete: Could not find node at path '" << path << "'\n";
                continue;
            }
            // since node is to be deleted, we need to record the paths.
            add_node_path_for_edit_history(path);

            if (!force_)
                check_for_active_or_submitted_tasks(as, theNodeToDelete.get());
            else
                as->zombie_ctrl().add_user_zombies(theNodeToDelete.get(), CtsApi::delete_node_arg());

            if (!defs->deleteChild(theNodeToDelete.get())) {
                std::string errorMsg = "Delete: Cannot delete node " + theNodeToDelete->debugNodePath();
                throw std::runtime_error(errorMsg);
            }
        }

        std::string error_msg = ss.str();
        if (!error_msg.empty()) {
            throw std::runtime_error(error_msg);
        }
    }

    return PreAllocatedReply::ok_cmd();
}

// bool DeleteCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
//     return do_authenticate(as, cmd, paths_);
// }

void DeleteCmd::check_for_active_or_submitted_tasks(AbstractServer* as, Node* theNodeToDelete) {
    vector<Task*> taskVec;
    if (theNodeToDelete) {
        theNodeToDelete->getAllTasks(taskVec);
    }
    else {
        as->defs()->getAllTasks(taskVec);
    }

    vector<Task*> activeVec, submittedVec;
    for (Task* t : taskVec) {
        if (t->state() == NState::ACTIVE)
            activeVec.push_back(t);
        if (t->state() == NState::SUBMITTED)
            submittedVec.push_back(t);
    }
    if (!activeVec.empty() || !submittedVec.empty()) {
        std::stringstream ss;
        if (theNodeToDelete)
            ss << "Cannot delete node " << theNodeToDelete->debugNodePath() << "\n";
        else
            ss << "Cannot delete all nodes.\n";
        if (!activeVec.empty()) {
            ss << " There are " << activeVec.size() << " active tasks. First : " << activeVec.front()->absNodePath()
               << "\n";
        }
        if (!submittedVec.empty()) {
            ss << " There are " << submittedVec.size()
               << " submitted tasks. First : " << submittedVec.front()->absNodePath() << "\n";
        }
        ss << "Please use the 'force' option to bypass this check, at the expense of creating zombies\n";
        throw std::runtime_error(ss.str());
    }
}

static const char* delete_node_desc() {
    return "Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.\n"
           "  arg1 = [ force | yes ](optional)  # Use this parameter to bypass checks, i.e. for active or submitted "
           "tasks\n"
           "  arg2 = yes(optional)              # Use 'yes' to bypass the confirmation prompt\n"
           "  arg3 = node paths | _all_         # _all_ means delete all suites\n"
           "                                    # node paths must start with a leading '/'\n"
           "Usage:\n"
           "  --delete=_all_                    # Delete all suites in server. Use with care.\n"
           "  --delete=/suite/f1/t1             # Delete node at /suite/f1/t1. This will prompt\n"
           "  --delete=force /suite/f1/t1       # Delete node at /suite/f1/t1 even if active or submitted\n"
           "  --delete=force yes /s1 /s2        # Delete suites s1,s2 even if active or submitted, bypassing prompt";
}

void DeleteCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(CtsApi::delete_node_arg(), po::value<vector<string>>()->multitoken(), delete_node_desc());
}

void DeleteCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    vector<string> args = vm[theArg()].as<vector<string>>();
    if (ac->debug())
        dumpVecArgs(theArg(), args);

    std::vector<std::string> options, paths;
    split_args_to_options_and_paths(args, options, paths); // relative order is still preserved

    bool force      = false;
    bool all        = false;
    bool do_prompt  = true;
    size_t vec_size = options.size();
    for (size_t i = 0; i < vec_size; i++) {
        if (args[i] == "_all_")
            all = true;
        if (args[i] == "force")
            force = true;
        if (args[i] == "yes")
            do_prompt = false;
    }

    if (!all && paths.empty()) {
        std::stringstream ss;
        ss << "Delete: No paths specified. Paths must begin with a leading '/' character\n";
        throw std::runtime_error(ss.str());
    }

    if (do_prompt) {
        std::string confirm;
        if (paths.empty())
            confirm = "Are you sure you want to delete all the suites ? ";
        else {
            confirm          = "Are you sure want to delete nodes at paths:\n";
            size_t path_size = paths.size();
            for (size_t i = 0; i < path_size; i++) {
                confirm += "  " + paths[i];
                if (i == path_size - 1)
                    confirm += " ? ";
                else
                    confirm += "\n";
            }
        }
        prompt_for_confirmation(confirm);
    }

    cmd = std::make_shared<DeleteCmd>(paths, force);
}

std::ostream& operator<<(std::ostream& os, const DeleteCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(DeleteCmd)
CEREAL_REGISTER_DYNAMIC_INIT(DeleteCmd)

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/ZombieCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Task.hpp"

using namespace ecf;

void ZombieCmd::print(std::string& os) const {
    switch (user_action_) {
        case ZombieCtrlAction::FOB:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieFob(paths_, process_id_, password_)));
            break;
        case ZombieCtrlAction::FAIL:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieFail(paths_, process_id_, password_)));
            break;
        case ZombieCtrlAction::ADOPT:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieAdopt(paths_, process_id_, password_)));
            break;
        case ZombieCtrlAction::REMOVE:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieRemove(paths_, process_id_, password_)));
            break;
        case ZombieCtrlAction::BLOCK:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieBlock(paths_, process_id_, password_)));
            break;
        case ZombieCtrlAction::KILL:
            user_cmd(os, CtsApi::to_string(CtsApi::zombieKill(paths_, process_id_, password_)));
            break;
        default:
            break;
    }
}

void ZombieCmd::print_only(std::string& os) const {
    switch (user_action_) {
        case ZombieCtrlAction::FOB:
            os += CtsApi::to_string(CtsApi::zombieFob(paths_, process_id_, password_));
            break;
        case ZombieCtrlAction::FAIL:
            os += CtsApi::to_string(CtsApi::zombieFail(paths_, process_id_, password_));
            break;
        case ZombieCtrlAction::ADOPT:
            os += CtsApi::to_string(CtsApi::zombieAdopt(paths_, process_id_, password_));
            break;
        case ZombieCtrlAction::REMOVE:
            os += CtsApi::to_string(CtsApi::zombieRemove(paths_, process_id_, password_));
            break;
        case ZombieCtrlAction::BLOCK:
            os += CtsApi::to_string(CtsApi::zombieBlock(paths_, process_id_, password_));
            break;
        case ZombieCtrlAction::KILL:
            os += CtsApi::to_string(CtsApi::zombieKill(paths_, process_id_, password_));
            break;
        default:
            break;
    }
}

bool ZombieCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<ZombieCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (paths_ != the_rhs->paths()) {
        return false;
    }
    if (process_id_ != the_rhs->process_or_remote_id()) {
        return false;
    }
    if (password_ != the_rhs->password()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t ZombieCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t ZombieCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

STC_Cmd_ptr ZombieCmd::doHandleRequest(AbstractServer* as) const {
    switch (user_action_) {
        case ZombieCtrlAction::FOB:
            as->update_stats().zombie_fob_++;
            break;
        case ZombieCtrlAction::FAIL:
            as->update_stats().zombie_fail_++;
            break;
        case ZombieCtrlAction::ADOPT:
            as->update_stats().zombie_adopt_++;
            break;
        case ZombieCtrlAction::REMOVE:
            as->update_stats().zombie_remove_++;
            break;
        case ZombieCtrlAction::BLOCK:
            as->update_stats().zombie_block_++;
            break;
        case ZombieCtrlAction::KILL:
            as->update_stats().zombie_kill_++;
            break;
    }

    // To uniquely identify a zombie we need path to task and remote_id, This information
    // is available from the zombie class via get command. However we do not want to expose the password.
    // Hence the Command level interface  will make do with just the path to the task.
    // The first zombie whose corresponding task where password does *NOT* match is acted upon
    if (process_id_.empty() && password_.empty()) {
        for (const auto& path : paths_) {
            node_ptr node = as->defs()->findAbsNode(path);
            Task* task    = nullptr;
            if (node.get()) {
                task = node->isTask();
            }
            switch (user_action_) {
                case ZombieCtrlAction::FOB:
                    as->zombie_ctrl().fobCli(path, task);
                    break;
                case ZombieCtrlAction::FAIL:
                    as->zombie_ctrl().failCli(path, task);
                    break;
                case ZombieCtrlAction::ADOPT:
                    as->zombie_ctrl().adoptCli(path, task);
                    break;
                case ZombieCtrlAction::REMOVE:
                    as->zombie_ctrl().removeCli(path, task);
                    break;
                case ZombieCtrlAction::BLOCK:
                    as->zombie_ctrl().blockCli(path, task);
                    break;
                case ZombieCtrlAction::KILL:
                    as->zombie_ctrl().killCli(path, task);
                    break;
            }
        }
    }
    else {
        // expect a single path, process_id and password only apply for a single task
        if (paths_.size() == 1) {
            switch (user_action_) {
                case ZombieCtrlAction::FOB:
                    as->zombie_ctrl().fob(paths_[0], process_id_, password_);
                    break;
                case ZombieCtrlAction::FAIL:
                    as->zombie_ctrl().fail(paths_[0], process_id_, password_);
                    break;
                case ZombieCtrlAction::ADOPT:
                    as->zombie_ctrl().adopt(paths_[0], process_id_, password_);
                    break;
                case ZombieCtrlAction::REMOVE:
                    as->zombie_ctrl().remove(paths_[0], process_id_, password_);
                    break;
                case ZombieCtrlAction::BLOCK:
                    as->zombie_ctrl().block(paths_[0], process_id_, password_);
                    break;
                case ZombieCtrlAction::KILL:
                    as->zombie_ctrl().kill(paths_[0], process_id_, password_);
                    break;
            }
        }
        else {
            return PreAllocatedReply::error_cmd("ZombieCmd: process_or_remote_id and password cannot be used when "
                                                "multiple paths are specified. Please specify a single path");
        }
    }

    return PreAllocatedReply::ok_cmd();
}

const char* ZombieCmd::theArg() const {

    switch (user_action_) {
        case ZombieCtrlAction::FOB:
            return CtsApi::zombieFobArg();
        case ZombieCtrlAction::FAIL:
            return CtsApi::zombieFailArg();
        case ZombieCtrlAction::ADOPT:
            return CtsApi::zombieAdoptArg();
        case ZombieCtrlAction::REMOVE:
            return CtsApi::zombieRemoveArg();
        case ZombieCtrlAction::BLOCK:
            return CtsApi::zombieBlockArg();
        case ZombieCtrlAction::KILL:
            return CtsApi::zombieKillArg();
        default:
            break;
    }
    assert(false);
    return nullptr;
}

void ZombieCmd::addOption(boost::program_options::options_description& desc) const {
    switch (user_action_) {

        case ZombieCtrlAction::FOB: {
            desc.add_options()(CtsApi::zombieFobArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        case ZombieCtrlAction::FAIL: {
            desc.add_options()(CtsApi::zombieFailArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        case ZombieCtrlAction::ADOPT: {
            desc.add_options()(CtsApi::zombieAdoptArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        case ZombieCtrlAction::REMOVE: {
            desc.add_options()(CtsApi::zombieRemoveArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        case ZombieCtrlAction::BLOCK: {
            desc.add_options()(CtsApi::zombieBlockArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        case ZombieCtrlAction::KILL: {
            desc.add_options()(CtsApi::zombieKillArg(),
                               boost::program_options::value<std::vector<std::string>>()->multitoken());
            break;
        }
        default:
            assert(false);
            break;
    }
}

void ZombieCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    auto args = vm[theArg()].as<std::vector<std::string>>();
    if (ace->debug()) {
        dumpVecArgs(theArg(), args);
    }

    // For Command Line Interface only the task_path is provided. Just have to make do.
    // arg = process_or_remote_id   ( empty for CLI and when multiple paths specified)
    // arg = password               ( empty for CLI and when multiple paths specified)
    // arg = paths
    std::string process_or_remote_id;
    std::string password;

    std::vector<std::string> options, paths;
    split_args_to_options_and_paths(args, options, paths); // relative order is still preserved
    if (paths.empty()) {
        throw std::runtime_error(
            MESSAGE("ZombieCmd: No paths specified. At least one path expected. Paths must begin with a leading '/' "
                    "character\n"));
    }
    if (paths.size() > 1 && !options.empty()) {
        throw std::runtime_error(MESSAGE(
            "ZombieCmd: process_or_remote_id and password cannot be used when multiple paths are specified. Please "
            "specify a single path\n"));
    }
    if (options.size() >= 3) {
        throw std::runtime_error(MESSAGE(
            "ZombieCmd: to many options expected only process_or_remote_id and password and a list of paths.\n"));
    }
    if (options.size() == 1) {
        process_or_remote_id = options[0];
    }
    if (options.size() == 2) {
        password = options[1];
    }

    if (ace->get_cli()) {
        // We are using command line, expect only paths
        if (!process_or_remote_id.empty()) {
            throw std::runtime_error(
                "ZombieCmd:: With the command line interface, we expected only paths i.e /path/to/task");
        }
        if (!password.empty()) {
            throw std::runtime_error(
                "ZombieCmd:: With the command line interface, we expected only paths i.e /path/to/task");
        }
    }

    cmd = std::make_shared<ZombieCmd>(user_action_, paths, process_or_remote_id, password);
}

std::ostream& operator<<(std::ostream& os, const ZombieCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(ZombieCmd)
CEREAL_REGISTER_DYNAMIC_INIT(ZombieCmd)

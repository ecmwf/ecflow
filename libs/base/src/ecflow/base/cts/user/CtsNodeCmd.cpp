/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/CtsNodeCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/node/Jobs.hpp"
#ifdef DEBUG
    #include "ecflow/core/Ecf.hpp"
#endif

using namespace ecf;

bool CtsNodeCmd::why_cmd(std::string& nodePath) const {
    if (api_ == CtsNodeCmd::WHY) {
        nodePath = absNodePath_;
        return true;
    }
    return false;
}

void CtsNodeCmd::print(std::string& os) const {
    switch (api_) {
        case CtsNodeCmd::GET: {
            user_cmd(os, CtsApi::get(absNodePath_));
#ifdef DEBUG
            os += Ecf::server()
                      ? MESSAGE(" [server(" << Ecf::state_change_no() << " " << Ecf::modify_change_no() << ")]")
                      : "";
#endif
            break;
        }
        case CtsNodeCmd::GET_STATE:
            user_cmd(os, CtsApi::get_state(absNodePath_));
            break;
        case CtsNodeCmd::MIGRATE:
            user_cmd(os, CtsApi::migrate(absNodePath_));
            break;
        case CtsNodeCmd::JOB_GEN:
            user_cmd(os, CtsApi::job_gen(absNodePath_));
            break;
        case CtsNodeCmd::WHY:
            user_cmd(os, CtsApi::why(absNodePath_));
            break;
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            user_cmd(os, CtsApi::checkJobGenOnly(absNodePath_));
            break;
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::print: Unrecognised command");
    }
}
void CtsNodeCmd::print_only(std::string& os) const {
    switch (api_) {
        case CtsNodeCmd::GET:
            os += CtsApi::get(absNodePath_);
            break;
        case CtsNodeCmd::GET_STATE:
            os += CtsApi::get_state(absNodePath_);
            break;
        case CtsNodeCmd::MIGRATE:
            os += CtsApi::migrate(absNodePath_);
            break;
        case CtsNodeCmd::JOB_GEN:
            os += CtsApi::job_gen(absNodePath_);
            break;
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            os += CtsApi::checkJobGenOnly(absNodePath_);
            break;
        case CtsNodeCmd::WHY:
            os += CtsApi::why(absNodePath_);
            break;
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::print_only : Unrecognised command");
    }
}

ecf::authentication_t CtsNodeCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t CtsNodeCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

bool CtsNodeCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsNodeCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (api_ != the_rhs->api()) {
        return false;
    }
    if (absNodePath_ != the_rhs->pathToNode()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

bool CtsNodeCmd::isWrite() const {
    switch (api_) {
        case CtsNodeCmd::GET:
            return false; // read only
        case CtsNodeCmd::GET_STATE:
            return false; // read only
        case CtsNodeCmd::MIGRATE:
            return false; // read only
        case CtsNodeCmd::JOB_GEN:
            return true; // requires write privilege
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            return false; // read only
        case CtsNodeCmd::WHY:
            return false; // read only
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::isWrite: Unrecognised command");
    }
    assert(false);
    return false;
}

const char* CtsNodeCmd::theArg() const {
    switch (api_) {
        case CtsNodeCmd::GET:
            return CtsApi::getArg();
        case CtsNodeCmd::GET_STATE:
            return CtsApi::get_state_arg();
        case CtsNodeCmd::MIGRATE:
            return CtsApi::migrate_arg();
        case CtsNodeCmd::JOB_GEN:
            return CtsApi::job_genArg();
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            return CtsApi::checkJobGenOnlyArg();
        case CtsNodeCmd::WHY:
            return CtsApi::whyArg();
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::theArg: Unrecognised command");
    }
    assert(false);
    return nullptr;
}

PrintStyle::Type_t CtsNodeCmd::show_style() const {
    if (api_ == CtsNodeCmd::GET) {
        return PrintStyle::DEFS;
    }
    else if (api_ == CtsNodeCmd::GET_STATE) {
        return PrintStyle::STATE;
    }
    else if (api_ == CtsNodeCmd::MIGRATE) {
        return PrintStyle::MIGRATE;
    }
    return ClientToServerCmd::show_style();
}

ClientToServerCmd::time_duration_t CtsNodeCmd::timeout() const {
    if (api_ == CtsNodeCmd::GET) {
        return timeout_for_load_sync_and_get();
    }
    if (api_ == CtsNodeCmd::MIGRATE) {
        return std::chrono::seconds{120};
    }
    return ClientToServerCmd::timeout();
}

STC_Cmd_ptr CtsNodeCmd::doHandleRequest(AbstractServer* as) const {
    Defs* defs = as->defs().get();

    switch (api_) {

        case CtsNodeCmd::GET:
        case CtsNodeCmd::MIGRATE:
        case CtsNodeCmd::GET_STATE: {
            // The client will configure the output display
            as->update_stats().get_defs_++;
            if (absNodePath_.empty()) {
                // with migrate we need to get edit history.
                return PreAllocatedReply::defs_cmd(as, (api_ == MIGRATE)); // if true, save edit history
            }
            // however a request for a particular node that is not there is treated as an error
            node_ptr theNodeToReturn = find_node(defs, absNodePath_);
            return PreAllocatedReply::node_cmd(as, theNodeToReturn);
        }

        case CtsNodeCmd::CHECK_JOB_GEN_ONLY: {
            as->update_stats().node_check_job_gen_only_++;
            job_creation_ctrl_ptr jobCtrl = std::make_shared<JobCreationCtrl>();
            jobCtrl->set_node_path(absNodePath_);
            defs->check_job_creation(jobCtrl);
            if (!jobCtrl->get_error_msg().empty()) {
                throw std::runtime_error(jobCtrl->get_error_msg());
            }
            break;
        }

        case CtsNodeCmd::JOB_GEN: {
            as->update_stats().node_job_gen_++;

            if (as->state() == SState::RUNNING) {

                if (absNodePath_.empty()) {
                    // If no path specified do a full job generation over all suites
                    return doJobSubmission(as);
                }

                // Generate jobs for the given node, downwards
                node_ptr theNode = find_node_for_edit(defs, absNodePath_);
                Jobs jobs(theNode.get());
                jobs.generate();
            }
            break;
        }

        case CtsNodeCmd::WHY: {
            /// Why is actually invoked on client side.
            /// Added as a command because:
            ///    o documentation
            ///    o allows use with group command, without any other changes
            break;
        }

        case CtsNodeCmd::NO_CMD:
        default:
            throw std::runtime_error("CtsNodeCmd::doHandleRequest: Unrecognised command");
    }

    return PreAllocatedReply::ok_cmd();
}

// bool CtsNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
//     return do_authenticate(as, cmd, absNodePath_);
// }

void CtsNodeCmd::addOption(boost::program_options::options_description& desc) const {
    switch (api_) {
        case CtsNodeCmd::GET: {
            desc.add_options()(CtsApi::getArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::GET_STATE: {
            desc.add_options()(CtsApi::get_state_arg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::MIGRATE: {
            desc.add_options()(CtsApi::migrate_arg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::JOB_GEN: {
            desc.add_options()(CtsApi::job_genArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY: {
            desc.add_options()(CtsApi::checkJobGenOnlyArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::WHY: {
            desc.add_options()(CtsApi::whyArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsNodeCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
}

void CtsNodeCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    assert(api_ != CtsNodeCmd::NO_CMD);

    if (ac->debug()) {
        std::cout << "  CtsNodeCmd::create = '" << theArg() << "'.\n";
    }

    std::string absNodePath = vm[theArg()].as<std::string>();

    cmd = std::make_shared<CtsNodeCmd>(api_, absNodePath);
}

std::ostream& operator<<(std::ostream& os, const CtsNodeCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CtsNodeCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CtsNodeCmd)

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/CtsCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/Gnuplot.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"

using namespace ecf;

// *IMPORTANT*: STATS_RESET was introduced in release 4.0.5

void CtsCmd::print(std::string& os) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            user_cmd(os, CtsApi::zombieGet());
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            user_cmd(os, CtsApi::restoreDefsFromCheckPt());
            break;
        case CtsCmd::RESTART_SERVER:
            user_cmd(os, CtsApi::restartServer());
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            user_cmd(os, CtsApi::shutdownServer());
            break;
        case CtsCmd::HALT_SERVER:
            user_cmd(os, CtsApi::haltServer());
            break;
        case CtsCmd::TERMINATE_SERVER:
            user_cmd(os, CtsApi::terminateServer());
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            user_cmd(os, CtsApi::reloadwsfile());
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            user_cmd(os, CtsApi::reloadpasswdfile());
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            user_cmd(os, CtsApi::reloadcustompasswdfile());
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            user_cmd(os, CtsApi::forceDependencyEval());
            break;
        case CtsCmd::PING:
            user_cmd(os, CtsApi::pingServer());
            break;
        case CtsCmd::STATS:
            user_cmd(os, CtsApi::stats());
            break;
        case CtsCmd::STATS_SERVER:
            user_cmd(os, CtsApi::stats_server());
            break;
        case CtsCmd::STATS_RESET:
            user_cmd(os, CtsApi::stats_reset());
            break;
        case CtsCmd::SUITES:
            user_cmd(os, CtsApi::suites());
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            user_cmd(os, CtsApi::debug_server_on());
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            user_cmd(os, CtsApi::debug_server_off());
            break;
        case CtsCmd::SERVER_LOAD:
            user_cmd(os, CtsApi::server_load(""));
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            os += "CtsCmdCtsCmd::NO_CMD  !!!!";
            break;
        default:
            assert(false);
            os += "CtsCmd did not match api_ !!!!";
            break;
    }
}

void CtsCmd::print_only(std::string& os) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            os += CtsApi::zombieGet();
            break;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            os += CtsApi::restoreDefsFromCheckPt();
            break;
        case CtsCmd::RESTART_SERVER:
            os += CtsApi::restartServer();
            break;
        case CtsCmd::SHUTDOWN_SERVER:
            os += CtsApi::shutdownServer();
            break;
        case CtsCmd::HALT_SERVER:
            os += CtsApi::haltServer();
            break;
        case CtsCmd::TERMINATE_SERVER:
            os += CtsApi::terminateServer();
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            os += CtsApi::reloadwsfile();
            break;
        case CtsCmd::RELOAD_PASSWD_FILE:
            os += CtsApi::reloadpasswdfile();
            break;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            os += CtsApi::reloadcustompasswdfile();
            break;
        case CtsCmd::FORCE_DEP_EVAL:
            os += CtsApi::forceDependencyEval();
            break;
        case CtsCmd::PING:
            os += CtsApi::pingServer();
            break;
        case CtsCmd::STATS:
            os += CtsApi::stats();
            break;
        case CtsCmd::STATS_SERVER:
            os += CtsApi::stats_server();
            break;
        case CtsCmd::STATS_RESET:
            os += CtsApi::stats_reset();
            break;
        case CtsCmd::SUITES:
            os += CtsApi::suites();
            break;
        case CtsCmd::DEBUG_SERVER_ON:
            os += CtsApi::debug_server_on();
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            os += CtsApi::debug_server_off();
            break;
        case CtsCmd::SERVER_LOAD:
            os += CtsApi::server_load("");
            break;
        case CtsCmd::NO_CMD:
            assert(false);
            os += "CtsCmdCtsCmd::NO_CMD  !!!!";
            break;
        default:
            assert(false);
            os += "CtsCmd did not match api_ !!!!";
            break;
    }
}

bool CtsCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (api_ != the_rhs->api()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t CtsCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t CtsCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

bool CtsCmd::isWrite() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return false; // read only
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return true; // requires write privilege
        case CtsCmd::RESTART_SERVER:
            return true; // requires write privilege
        case CtsCmd::SHUTDOWN_SERVER:
            return true; // requires write privilege
        case CtsCmd::HALT_SERVER:
            return true; // requires write privilege
        case CtsCmd::TERMINATE_SERVER:
            return true; // requires write privilege
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return true; // requires write privilege
        case CtsCmd::RELOAD_PASSWD_FILE:
            return true; // requires write privilege
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return true; // requires write privilege
        case CtsCmd::FORCE_DEP_EVAL:
            return true; // requires write privilege
        case CtsCmd::PING:
            return false; // read only
        case CtsCmd::STATS:
            return false; // read only
        case CtsCmd::STATS_SERVER:
            return false; // read only
        case CtsCmd::STATS_RESET:
            return true; // requires write privilege
        case CtsCmd::SUITES:
            return false; // read only
        case CtsCmd::DEBUG_SERVER_ON:
            return false; // read only
        case CtsCmd::DEBUG_SERVER_OFF:
            return false; // read only
        case CtsCmd::SERVER_LOAD:
            return false; // read only
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return false;
}

bool CtsCmd::cmd_updates_defs() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return false;
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return true;
        case CtsCmd::RESTART_SERVER:
            return true;
        case CtsCmd::SHUTDOWN_SERVER:
            return true;
        case CtsCmd::HALT_SERVER:
            return true;
        case CtsCmd::TERMINATE_SERVER:
            return true;
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return false;
        case CtsCmd::RELOAD_PASSWD_FILE:
            return false;
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return false;
        case CtsCmd::FORCE_DEP_EVAL:
            return true;
        case CtsCmd::PING:
            return false;
        case CtsCmd::STATS:
            return false;
        case CtsCmd::STATS_SERVER:
            return false;
        case CtsCmd::STATS_RESET:
            return false;
        case CtsCmd::SUITES:
            return false;
        case CtsCmd::DEBUG_SERVER_ON:
            return false;
        case CtsCmd::DEBUG_SERVER_OFF:
            return false;
        case CtsCmd::SERVER_LOAD:
            return false;
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return false;
}

ClientToServerCmd::time_duration_t CtsCmd::timeout() const {
    if (api_ == CtsCmd::PING) {
        return std::chrono::seconds{10};
    }
    return ClientToServerCmd::timeout();
}

const char* CtsCmd::theArg() const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES:
            return CtsApi::zombieGetArg();
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            return CtsApi::restoreDefsFromCheckPtArg();
        case CtsCmd::RESTART_SERVER:
            return CtsApi::restartServerArg();
        case CtsCmd::SHUTDOWN_SERVER:
            return CtsApi::shutdownServerArg();
        case CtsCmd::HALT_SERVER:
            return CtsApi::haltServerArg();
        case CtsCmd::TERMINATE_SERVER:
            return CtsApi::terminateServerArg();
        case CtsCmd::RELOAD_WHITE_LIST_FILE:
            return CtsApi::reloadwsfileArg();
        case CtsCmd::RELOAD_PASSWD_FILE:
            return CtsApi::reloadpasswdfile_arg();
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            return CtsApi::reloadcustompasswdfile_arg();
        case CtsCmd::FORCE_DEP_EVAL:
            return CtsApi::forceDependencyEvalArg();
        case CtsCmd::PING:
            return CtsApi::pingServerArg();
        case CtsCmd::STATS:
            return CtsApi::statsArg();
        case CtsCmd::STATS_SERVER:
            return CtsApi::stats_server_arg();
        case CtsCmd::STATS_RESET:
            return CtsApi::stats_reset_arg();
        case CtsCmd::SUITES:
            return CtsApi::suitesArg();
        case CtsCmd::DEBUG_SERVER_ON:
            return CtsApi::debug_server_on_arg();
        case CtsCmd::DEBUG_SERVER_OFF:
            return CtsApi::debug_server_off_arg();
        case CtsCmd::SERVER_LOAD:
            return CtsApi::server_load_arg();
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
    assert(false);
    return nullptr;
}

STC_Cmd_ptr CtsCmd::doHandleRequest(AbstractServer* as) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES: {
            as->update_stats().zombie_get_++;
            return PreAllocatedReply::zombie_get_cmd(as);
        }

        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: {
            as->update_stats().restore_defs_from_checkpt_++;
            as->restore_defs_from_checkpt(); // this can throw, i.e. if server not halted, or defs has suites, etc
            break;
        }

        case CtsCmd::RESTART_SERVER: {
            as->update_stats().restart_server_++;
            as->restart();
            return doJobSubmission(as);
        }
        case CtsCmd::SHUTDOWN_SERVER:
            as->update_stats().shutdown_server_++;
            as->shutdown();
            break;
        case CtsCmd::HALT_SERVER:
            as->update_stats().halt_server_++;
            as->halted();
            break;
        case CtsCmd::TERMINATE_SERVER:
            as->checkPtDefs();
            break;
        case CtsCmd::RELOAD_WHITE_LIST_FILE: {
            as->update_stats().reload_white_list_file_++;
            std::string errorMsg;
            if (!as->reloadWhiteListFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::RELOAD_PASSWD_FILE: {
            as->update_stats().reload_passwd_file_++;
            std::string errorMsg;
            if (!as->reloadPasswdFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE: {
            as->update_stats().reload_passwd_file_++;
            std::string errorMsg;
            if (!as->reloadCustomPasswdFile(errorMsg)) {
                throw std::runtime_error(errorMsg);
            }
            break;
        }
        case CtsCmd::FORCE_DEP_EVAL: {
            // The Default JobsParam does *not* allow Job creation, & hence => does not submit jobs
            // The default does *not* allow job spawning
            Jobs jobs(as->defs());
            JobsParam jobsParam; // create jobs =  false, spawn_jobs = false
            if (!jobs.generate(jobsParam)) {
                throw std::runtime_error(jobsParam.getErrorMsg());
            }
            break;
        }
        case CtsCmd::PING:
            as->update_stats().ping_++;
            break;
        case CtsCmd::STATS: {
            as->update_stats().stats_++;
            as->stats().update_for_serialisation();
            as->stats().no_of_suites_ = as->defs()->suites().size();
            std::ostringstream ss;
            as->stats().show(ss); // ECFLOW-880, allow stats to be changed in server, by only returning string
            return PreAllocatedReply::string_cmd(ss.str());
        }
        case CtsCmd::STATS_SERVER: { // Only to be used in test, as subject to change, returns Stats struct
            as->update_stats().stats_++;
            return PreAllocatedReply::stats_cmd(as);
        }
        case CtsCmd::STATS_RESET:
            as->update_stats().reset();
            break; // we could have done as->update_stats().stats_++, to honor reset, we dont
        case CtsCmd::SUITES:
            as->update_stats().suites_++;
            return PreAllocatedReply::suites_cmd(as);
        case CtsCmd::DEBUG_SERVER_ON:
            as->update_stats().debug_server_on_++;
            as->debug_server_on();
            break;
        case CtsCmd::DEBUG_SERVER_OFF:
            as->update_stats().debug_server_off_++;
            as->debug_server_off();
            break;
        case CtsCmd::SERVER_LOAD: {
            as->update_stats().server_load_cmd_++;
            return PreAllocatedReply::server_load_cmd(Log::instance()->path());
        }
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }

    return PreAllocatedReply::ok_cmd();
}

void CtsCmd::addOption(boost::program_options::options_description& desc) const {
    switch (api_) {
        case CtsCmd::GET_ZOMBIES: {
            desc.add_options()(CtsApi::zombieGetArg(), "");
            break;
        }
        case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: {
            desc.add_options()(CtsApi::restoreDefsFromCheckPtArg(), "");
            break;
        }
        case CtsCmd::RESTART_SERVER: {
            desc.add_options()(CtsApi::restartServerArg(), "");
            break;
        }
        case CtsCmd::SHUTDOWN_SERVER: {
            desc.add_options()(CtsApi::shutdownServerArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsCmd::HALT_SERVER: {
            desc.add_options()(CtsApi::haltServerArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsCmd::TERMINATE_SERVER: {
            desc.add_options()(CtsApi::terminateServerArg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsCmd::RELOAD_WHITE_LIST_FILE: {
            desc.add_options()(CtsApi::reloadwsfileArg(), "");
            break;
        }
        case CtsCmd::RELOAD_PASSWD_FILE: {
            desc.add_options()(CtsApi::reloadpasswdfile_arg(), "");
            break;
        }
        case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE: {
            desc.add_options()(CtsApi::reloadcustompasswdfile_arg(), "");
            break;
        }

        case CtsCmd::FORCE_DEP_EVAL: {
            desc.add_options()(CtsApi::forceDependencyEvalArg(), "");
            break;
        }
        case CtsCmd::PING: {
            desc.add_options()(CtsApi::pingServerArg(), "");
            break;
        }
        case CtsCmd::STATS: {
            desc.add_options()(CtsApi::statsArg(), "");
            break;
        }
        case CtsCmd::STATS_SERVER: {
            desc.add_options()(CtsApi::stats_server_arg(), "");
            break;
        }
        case CtsCmd::STATS_RESET: {
            desc.add_options()(CtsApi::stats_reset_arg(), "");
            break;
        }
        case CtsCmd::SUITES: {
            desc.add_options()(CtsApi::suitesArg(), "");
            break;
        }
        case CtsCmd::DEBUG_SERVER_ON: {
            desc.add_options()(CtsApi::debug_server_on_arg(), "");
            break;
        }
        case CtsCmd::DEBUG_SERVER_OFF: {
            desc.add_options()(CtsApi::debug_server_off_arg(), "");
            break;
        }
        case CtsCmd::SERVER_LOAD: {
            desc.add_options()(CtsApi::server_load_arg(),
                               boost::program_options::value<std::string>()->implicit_value(std::string{}));
            break;
        }
        case CtsCmd::NO_CMD:
            assert(false);
            break;
        default:
            assert(false);
            break;
    }
}

bool CtsCmd::handleRequestIsTestable() const {
    if (api_ == CtsCmd::TERMINATE_SERVER) {
        return false;
    }
    if (api_ == CtsCmd::RESTORE_DEFS_FROM_CHECKPT) {
        return false;
    }
    return true;
}

void CtsCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    if (ac->debug()) {
        std::cout << "  CtsCmd::create api = '" << api_ << "'.\n";
    }

    assert(api_ != CtsCmd::NO_CMD);

    if (api_ == CtsCmd::HALT_SERVER || api_ == CtsCmd::SHUTDOWN_SERVER || api_ == CtsCmd::TERMINATE_SERVER) {

        std::string do_prompt = vm[theArg()].as<std::string>();
        if (do_prompt.empty()) {
            if (api_ == CtsCmd::HALT_SERVER) {
                prompt_for_confirmation("Are you sure you want to halt the server ? ");
            }
            else if (api_ == CtsCmd::SHUTDOWN_SERVER) {
                prompt_for_confirmation("Are you sure you want to shut down the server ? ");
            }
            else {
                prompt_for_confirmation("Are you sure you want to terminate the server ? ");
            }
        }
        else if (do_prompt != "yes") {
            throw std::runtime_error(
                "Halt, shutdown and terminate expected 'yes' as the only argument to bypass the confirmation prompt");
        }
    }
    else if (api_ == CtsCmd::SERVER_LOAD) {

        std::string log_file = vm[theArg()].as<std::string>();
        if (ac->debug()) {
            std::cout << "  CtsCmd::create CtsCmd::SERVER_LOAD " << log_file << "\n";
        }

        if (!log_file.empty()) {

            // testing client interface
            if (ac->under_test()) {
                return;
            }

            // No need to call server. Parse the log file to create gnu_plot file.
            Gnuplot gnuplot(log_file, ac->host(), ac->port());
            gnuplot.show_server_load();

            return; // Do not create command, since with log file, it is client specific only
        }
    }
    cmd = std::make_shared<CtsCmd>(api_);
}

std::ostream& operator<<(std::ostream& os, const CtsCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CtsCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CtsCmd)

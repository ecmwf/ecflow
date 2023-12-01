/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include "AbstractClientEnv.hpp"
#include "AbstractServer.hpp"
#include "ClientToServerCmd.hpp"
#include "CtsApi.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/node/Jobs.hpp"
#ifdef DEBUG
    #include "ecflow/core/Ecf.hpp"
#endif

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

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
            std::stringstream ss;
            if (Ecf::server())
                ss << " [server(" << Ecf::state_change_no() << " " << Ecf::modify_change_no() << ")]";
            os += ss.str();
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
            break;
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
            break;
    }
}

bool CtsNodeCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsNodeCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (api_ != the_rhs->api())
        return false;
    if (absNodePath_ != the_rhs->absNodePath())
        return false;
    return UserCmd::equals(rhs);
}

bool CtsNodeCmd::isWrite() const {
    switch (api_) {
        case CtsNodeCmd::GET:
            return false;
            break; // read only
        case CtsNodeCmd::GET_STATE:
            return false;
            break; // read only
        case CtsNodeCmd::MIGRATE:
            return false;
            break; // read only
        case CtsNodeCmd::JOB_GEN:
            return true;
            break; // requires write privilege
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            return false;
            break; // read only
        case CtsNodeCmd::WHY:
            return false;
            break; // read only
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::isWrite: Unrecognised command");
            break;
    }
    assert(false);
    return false;
}

const char* CtsNodeCmd::theArg() const {
    switch (api_) {
        case CtsNodeCmd::GET:
            return CtsApi::getArg();
            break;
        case CtsNodeCmd::GET_STATE:
            return CtsApi::get_state_arg();
            break;
        case CtsNodeCmd::MIGRATE:
            return CtsApi::migrate_arg();
            break;
        case CtsNodeCmd::JOB_GEN:
            return CtsApi::job_genArg();
            break;
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
            return CtsApi::checkJobGenOnlyArg();
            break;
        case CtsNodeCmd::WHY:
            return CtsApi::whyArg();
            break;
        case CtsNodeCmd::NO_CMD:
            break;
        default:
            throw std::runtime_error("CtsNodeCmd::theArg: Unrecognised command");
            break;
    }
    assert(false);
    return nullptr;
}

PrintStyle::Type_t CtsNodeCmd::show_style() const {
    if (api_ == CtsNodeCmd::GET)
        return PrintStyle::DEFS;
    else if (api_ == CtsNodeCmd::GET_STATE)
        return PrintStyle::STATE;
    else if (api_ == CtsNodeCmd::MIGRATE)
        return PrintStyle::MIGRATE;
    return ClientToServerCmd::show_style();
}

int CtsNodeCmd::timeout() const {
    if (api_ == CtsNodeCmd::GET) {
        return time_out_for_load_sync_and_get();
    }
    if (api_ == CtsNodeCmd::MIGRATE) {
        return 120;
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
            // however request for a particular node, thats not there, treated as an error
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
            break;
    }

    return PreAllocatedReply::ok_cmd();
}

bool CtsNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
    return do_authenticate(as, cmd, absNodePath_);
}

static const char* job_gen_only_desc() {
    return "Test hierarchical Job generation only, for chosen Node.\n"
           "The jobs are generated independent of the dependencies\n"
           "This will generate the jobs *only*, i.e. no job submission. Used for checking job generation only\n"
           "  arg = node path | arg = NULL\n"
           "     If no node path specified generates for all Tasks in the definition. For Test only";
}

static const char* job_gen_desc() { // dependency_dependent_job_submission
    return "Job submission for chosen Node *based* on dependencies.\n"
           "The server traverses the node tree every 60 seconds, and if the dependencies are free\n"
           "does job generation and submission. Sometimes the user may free time/date dependencies\n"
           "to avoid waiting for the server poll, this commands allows early job generation\n"
           "  arg = node path | arg = NULL\n"
           "     If no node path specified generates for full definition.";
}

static const char* why_desc() {
    return "Show the reason why a node is not running.\n"
           "Can only be used with the group command. The group command must include a \n"
           "'get' command(i.e returns the server defs)\n"
           "The why command take a optional string argument representing a node path\n"
           "Will return reason why the node is holding and for all its children.\n"
           "If no arguments supplied will report on all nodes\n"
           "  arg = node path | arg = NULL\n"
           "Usage:\n"
           "  --group=\"get; why\"               # returns why for all holding nodes\n"
           "  --group=\"get; why=/suite/family\" # returns why for a specific node";
}

static const char* get_desc() {
    return "Get the suite definition or node tree in form that is re-parse able\n"
           "Get all suite node tree's from the server and write to standard out.\n"
           "The output is parse-able, and can be used to re-load the definition\n"
           "  arg = NULL | arg = node path\n"
           "Usage:\n"
           "  --get     # gets the definition from the server,and writes to standard out\n"
           "  --get=/s1 # gets the suite from the server,and writes to standard out";
}

static const char* get_state_desc() {
    return "Get state data. For the whole suite definition or individual nodes.\n"
           "This will include event, meter, node state, trigger and time state.\n"
           "The output is written to standard out.\n"
           "  arg = NULL | arg = node path\n"
           "Usage:\n"
           "  --get_state     # gets the definition from the server,and writes to standard out\n"
           "  --get_state=/s1 # gets the suite from the server,and writes to standard out";
}

const char* migrate_desc() {
    return "Used to print state of the definition returned from the server to standard output.\n"
           "The node state is shown in the comments.\n"
           "This is the format used in the check point file, but with indentation.\n"
           "Since this is essentially the defs format with state in the comments,"
           "it allows the definition to be migrated to future version of ecflow.\n"
           "The output includes edit history but excludes externs.\n"
           "When the definition is reloaded *NO* checking is done.\n"
           "\n"
           "The following shows a summary of the features associated with each choice:\n"
           "                       --get  --get_state  --migrate\n"
           "Auto generate externs    Yes    Yes          No\n"
           "Checking on reload       Yes    Yes          No\n"
           "Edit History             No     No           Yes\n"
           "Show trigger AST         No     Yes          No\n\n"
           "Usage:\n"
           "    --migrate         # show all suites\n"
           "    --migrate=/s1     # show state for suite s1\n";
}

void CtsNodeCmd::addOption(boost::program_options::options_description& desc) const {
    switch (api_) {
        case CtsNodeCmd::GET: {
            desc.add_options()(CtsApi::getArg(), po::value<string>()->implicit_value(string()), get_desc());
            break;
        }
        case CtsNodeCmd::GET_STATE: {
            desc.add_options()(
                CtsApi::get_state_arg(), po::value<string>()->implicit_value(string()), get_state_desc());
            break;
        }
        case CtsNodeCmd::MIGRATE: {
            desc.add_options()(CtsApi::migrate_arg(), po::value<string>()->implicit_value(string()), migrate_desc());
            break;
        }
        case CtsNodeCmd::JOB_GEN: {
            desc.add_options()(CtsApi::job_genArg(), po::value<string>()->implicit_value(string()), job_gen_desc());
            break;
        }
        case CtsNodeCmd::CHECK_JOB_GEN_ONLY: {
            desc.add_options()(
                CtsApi::checkJobGenOnlyArg(), po::value<string>()->implicit_value(string()), job_gen_only_desc());
            break;
        }
        case CtsNodeCmd::WHY: {
            desc.add_options()(CtsApi::whyArg(), po::value<string>()->implicit_value(string()), why_desc());
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

    if (ac->debug())
        cout << "  CtsNodeCmd::create = '" << theArg() << "'.\n";

    std::string absNodePath = vm[theArg()].as<std::string>();

    cmd                     = std::make_shared<CtsNodeCmd>(api_, absNodePath);
}

std::ostream& operator<<(std::ostream& os, const CtsNodeCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

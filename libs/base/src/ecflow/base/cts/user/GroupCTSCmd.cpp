/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/GroupCTSCmd.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/ClientOptionsParser.hpp"
#include "ecflow/base/cts/CtsCmdRegistry.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

// #define DEBUG_GROUP_CMD 1

//======================================================================================

GroupCTSCmd::GroupCTSCmd(const std::string& cmdSeries, AbstractClientEnv* clientEnv) {
    std::vector<std::string> individualCmdVec;
    Str::split(cmdSeries, individualCmdVec, ";");
    if (individualCmdVec.empty())
        throw std::runtime_error("GroupCTSCmd::GroupCTSCmd: Please provide a list of ';' separated commands\n");
    if (clientEnv->debug()) {
        for (const auto& i : individualCmdVec) {
            cout << "  CHILD COMMAND = " << i << "\n";
        }
    }

    // Create a list of allowable commands for a group. i.e excludes help, group
    po::options_description desc("Allowed group options");
    CtsCmdRegistry cmdRegistry(false /* don't add group option */);
    cmdRegistry.addCmdOptions(desc);

    std::string subCmd;
    for (auto aCmd : individualCmdVec) {
        // massage the commands so that, we add -- at the start of each command.
        // This is required by the boost program options.
        ecf::algorithm::trim(aCmd);

        subCmd.clear();
        if (aCmd.find("--") == std::string::npos)
            subCmd = "--";
        subCmd += aCmd;

        // handle case like: alter add variable FRED "fre d ddy" /suite
        // If we have quote marks, then treat as one string,
        // by replacing spaces with /b, then replacing back after the split
        // This can only handle one level of quotes  hence can't cope with "fred \"joe fred\"
        bool start_quote     = false;
        bool replaced_spaces = false;
        for (char& i : subCmd) {
            if (start_quote) {
                if (i == '"' || i == '\'')
                    start_quote = false;
                else if (i == ' ') {
                    i               = '\b'; // "fre d ddy"  => "fre\bd\bddy"
                    replaced_spaces = true;
                }
            }
            else {
                if (i == '"' || i == '\'')
                    start_quote = true;
            }
        }

        // Each sub command can have, many args
        std::vector<std::string> subCmdArgs;
        Str::split(subCmd, subCmdArgs);

        if (replaced_spaces) {
            for (auto& str : subCmdArgs) {
                for (char& j : str) {
                    if (j == '\b')
                        j = ' '; // "fre\bd\bddy"  => "fre d ddy"
                }
            }
        }

        // The first will be the command, then the args. However from boost 1.59
        // we must use --cmd=value, instead of --cmd value
        if (!subCmdArgs.empty() && subCmdArgs.size() > 1 && subCmdArgs[0].find("=") == std::string::npos) {
            subCmdArgs[0] += "=";
            subCmdArgs[0] += subCmdArgs[1];
            subCmdArgs.erase(subCmdArgs.begin() + 1); // remove, since we have added to first
        }

        /// Hack because we *can't* create program option with vector of strings, which can be empty
        /// Hence if command is just show, add a dummy arg.
        // if (aCmd == "show")  subCmdArgs.push_back("<dummy_arg>");

        std::vector<std::string> theArgs;
        theArgs.emplace_back("ClientInvoker");
        std::copy(subCmdArgs.begin(), subCmdArgs.end(), std::back_inserter(theArgs));

        // Create a Argv array from a vector of strings
        CommandLine cl(theArgs);

        if (clientEnv->debug()) {
            cout << "  PROCESSING COMMAND = '" << subCmd << "' " << cl << "\n";
        }

        // Treat each sub command  separately
        boost::program_options::variables_map group_vm;

        // 1) Parse the CLI options
        po::parsed_options parsed_options =
            po::command_line_parser(cl.tokens())
                .options(desc)
                .style(po::command_line_style::unix_style ^ po::command_line_style::allow_short)
                .extra_style_parser(ClientOptionsParser{})
                .run();

        // 2) Store the CLI options into the variable map
        po::store(parsed_options, group_vm);
        po::notify(group_vm);

        Cmd_ptr childCmd;
        cmdRegistry.parse(childCmd, group_vm, clientEnv);
        addChild(childCmd);
    }
}

bool GroupCTSCmd::isWrite() const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->isWrite())
            return true;
    }
    return false;
}

bool GroupCTSCmd::cmd_updates_defs() const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->cmd_updates_defs())
            return true;
    }
    return false;
}

void GroupCTSCmd::set_identity(ecf::Identity identity) {
    this->ClientToServerCmd::set_identity(identity);
    for (auto subCmd : cmdVec_) {
        subCmd->set_identity(identity);
    }
    ClientToServerCmd::set_identity(std::move(identity));
}

bool GroupCTSCmd::get_cmd() const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->get_cmd())
            return true;
    }
    return false;
}

PrintStyle::Type_t GroupCTSCmd::show_style() const {
    // Only return non default style( PrintStyle::NOTHING ) if sub command
    // contains a show cmd
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->show_cmd())
            return subCmd->show_style();
    }
    return PrintStyle::NOTHING;
}

bool GroupCTSCmd::task_cmd() const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->task_cmd())
            return true;
    }
    return false;
}

bool GroupCTSCmd::terminate_cmd() const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->terminate_cmd())
            return true;
    }
    return false;
}

bool GroupCTSCmd::why_cmd(std::string& nodePath) const {
    for (Cmd_ptr subCmd : cmdVec_) {
        if (subCmd->why_cmd(nodePath))
            return true;
    }
    return false;
}

void GroupCTSCmd::print(std::string& os) const {
    std::string ret;
    size_t the_size = cmdVec_.size();
    for (size_t i = 0; i < the_size; i++) {
        if (i != 0)
            ret += "; ";
        cmdVec_[i]->print_only(ret); //  avoid overhead of user@host for each child command
    }
    user_cmd(os, CtsApi::group(ret));
}

std::string GroupCTSCmd::print_short() const {
    std::string ret;
    size_t the_size = cmdVec_.size();
    for (size_t i = 0; i < the_size; i++) {
        if (i != 0)
            ret += "; ";
        ret +=
            cmdVec_[i]
                ->print_short(); // limit number of paths shown and avoid overhead of user@host for each child command
    }
    return CtsApi::group(ret);
}

bool GroupCTSCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<GroupCTSCmd*>(rhs);
    if (!the_rhs)
        return false;

    const std::vector<Cmd_ptr>& rhsCmdVec = the_rhs->cmdVec();
    if (cmdVec_.size() != rhsCmdVec.size())
        return false;

    for (size_t i = 0; i < cmdVec_.size(); i++) {
        if (!cmdVec_[i]->equals(rhsCmdVec[i].get())) {
            return false;
        }
    }

    return UserCmd::equals(rhs);
}

ecf::authentication_t GroupCTSCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t GroupCTSCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void GroupCTSCmd::addChild(Cmd_ptr childCmd) {
    assert(childCmd.get()); // Dont add NULL children
    cmdVec_.push_back(childCmd);
}

void GroupCTSCmd::setup_user_authentification(const std::string& user, const std::string& passwd) {
    UserCmd::setup_user_authentification(user, passwd);
    for (auto& i : cmdVec_) {
        i->setup_user_authentification(user, passwd);
    }
}

bool GroupCTSCmd::setup_user_authentification(AbstractClientEnv& env) {
    if (!UserCmd::setup_user_authentification(env))
        return false;
    for (auto& i : cmdVec_) {
        if (!i->setup_user_authentification(env))
            return false;
    }
    return true;
}

void GroupCTSCmd::setup_user_authentification() {
    UserCmd::setup_user_authentification();
    for (auto& i : cmdVec_) {
        i->setup_user_authentification();
    }
}

void GroupCTSCmd::add_edit_history(Defs* defs) const {
    for (Cmd_ptr subCmd : cmdVec_) {
        subCmd->add_edit_history(defs);
    }
}

void GroupCTSCmd::cleanup() {
    for (Cmd_ptr subCmd : cmdVec_) {
        subCmd->cleanup();
    }
}

// bool GroupCTSCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& errorMsg) const {
//     // Can only run Group cmd if all child commands authenticate
//     size_t cmd_vec_size = cmdVec_.size();
//     for (size_t i = 0; i < cmd_vec_size; i++) {
//         if (!cmdVec_[i]->authenticate(as, errorMsg)) {
//
//             // Log authentication failure:
//             std::string ss;
//             ss += "GroupCTSCmd::authenticate failed: for ";
//             cmdVec_[i]->print(ss);
//             std::stringstream stream;
//             stream << errorMsg;
//             ss += stream.str();
//             log(Log::ERR, ss); // will automatically add end of line
//
// #ifdef DEBUG_GROUP_CMD
//             std::cout << "GroupCTSCmd::authenticate failed for " << ss << "\n";
// #endif
//             return false;
//         }
//     }
//     return true;
// }

// in the server
void GroupCTSCmd::set_client_handle(int client_handle) const {
    size_t cmd_vec_size = cmdVec_.size();
    if (cmd_vec_size >= 2) {
        cmdVec_[1]->set_client_handle(client_handle);
    }
}

STC_Cmd_ptr GroupCTSCmd::doHandleRequest(AbstractServer* as) const {
#ifdef DEBUG_GROUP_CMD
    std::cout << "GroupCTSCmd::doHandleRequest cmdVec_.size() = " << cmdVec_.size() << "\n";
#endif
    // ecf::LogTimer timer(" GroupCTSCmd::doHandleRequest");

    as->update_stats().group_cmd_++;

    std::shared_ptr<GroupSTCCmd> theReturnedGroupCmd = std::make_shared<GroupSTCCmd>();

    // For the command to succeed all children MUST succeed
    size_t cmd_vec_size = cmdVec_.size();
    for (size_t i = 0; i < cmd_vec_size; i++) {
#ifdef DEBUG_GROUP_CMD
        std::cout << "  GroupCTSCmd::doHandleRequest calling ";
        string ret;
        cmdVec_[i]->print(ret);
        cout << ret << "\n"; // std::cout << "\n";
#endif

        // Let child know about Group command.
        // Only used by ClientHandleCmd and DeleteCmd to transfer client_handle to the sync cmd, in *this* group
        cmdVec_[i]->set_group_cmd(this);

        STC_Cmd_ptr theReturnCmd;
        try {
            auto& cmd = cmdVec_[i];

            if (auto valid = cmd->check_preconditions(as, theReturnCmd); valid) {
                // If command preconditions are valid, we just handle the request as usual
                theReturnCmd = cmdVec_[i]->doHandleRequest(as);
            }
            // Otherwise, theReturnCmd has been set in the process of checking the precondictions
        }
        catch (std::exception& e) {
            cmdVec_[i]->cleanup(); // recover memory asap, important when cmd has a large number of paths.
            theReturnedGroupCmd->addChild(std::make_shared<ErrorCmd>(e.what()));
            continue;
        }
        cmdVec_[i]->cleanup(); // recover memory asap, important when cmd has a large number of paths.

#ifdef DEBUG_GROUP_CMD
        std::cout << " return Cmd = ";
        theReturnCmd->print(std::cout);
        std::cout << " to client\n";
#endif

        if (theReturnCmd->is_returnable_in_group_cmd()) {
            theReturnedGroupCmd->addChild(theReturnCmd);
            continue;
        }
    }

    if (theReturnedGroupCmd->cmdVec().empty()) {
        // Nothing to return, i.e. no Defs, Node or Log file
        return PreAllocatedReply::ok_cmd();
    }

    return theReturnedGroupCmd;
}

const char* GroupCTSCmd::arg() {
    return CtsApi::groupArg();
}
const char* GroupCTSCmd::desc() {
    return "Allows a series of ';' separated commands to be grouped and executed as one.\n"
           "Some commands like halt, shutdown and terminate will prompt the user. To bypass the prompt\n"
           "provide 'yes' as an additional parameter. See example below.\n"
           "  arg = string\n"
           "Usage:\n"
           "   --group=\"halt=yes; reloadwsfile; restart;\"\n"
           "                                 # halt server,bypass the confirmation prompt,\n"
           "                                 # reload white list file, restart server\n"
           "   --group=\"get; show\"           # get server defs, and write to standard output\n"
           "   --group=\"get=/s1; show state\" # get suite 's1', and write state to standard output";
}

void GroupCTSCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(GroupCTSCmd::arg(), po::value<string>(), GroupCTSCmd::desc());
}

void GroupCTSCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    if (clientEnv->debug())
        cout << "  " << arg() << ": Group Cmd '" << vm[arg()].as<std::string>() << "'\n";

    // Parse and split commands and then parse individually. Assumes commands are separated by ';'
    std::string cmdSeries = vm[GroupCTSCmd::arg()].as<std::string>();

    cmd = std::make_shared<GroupCTSCmd>(cmdSeries, clientEnv);
}

std::ostream& operator<<(std::ostream& os, const GroupCTSCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(GroupCTSCmd)
CEREAL_REGISTER_DYNAMIC_INIT(GroupCTSCmd)

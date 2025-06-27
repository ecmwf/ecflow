/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

ReplaceNodeCmd::ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, defs_ptr client_defs, bool force)
    : createNodesAsNeeded_(createNodesAsNeeded),
      force_(force),
      pathToNode_(node_path) {
    if (!client_defs.get()) {
        throw std::runtime_error("ReplaceNodeCmd::ReplaceNodeCmd: client definition is empty");
    }

    // Client defs has been created in memory.
    // warn about naff expression and unresolved in-limit references to Limit's
    std::string errMsg, warningMsg;
    if (!client_defs->check(errMsg, warningMsg)) {
        throw std::runtime_error(errMsg);
    }

    // Make sure pathToNode exists in the client defs
    node_ptr nodeToReplace = client_defs->findAbsNode(node_path);
    if (!nodeToReplace.get()) {
        std::stringstream ss;
        ss << "ReplaceNodeCmd::ReplaceNodeCmd: Cannot replace child since path " << node_path;
        ss << ", does not exist in the client definition ";
        throw std::runtime_error(ss.str());
    }

    client_defs->write_as_string(clientDefs_, PrintStyle::NET);

    // Out put any warning's to standard output
    cout << warningMsg;
}

ReplaceNodeCmd::ReplaceNodeCmd(const std::string& node_path,
                               bool createNodesAsNeeded,
                               const std::string& path_to_defs,
                               bool force)
    : createNodesAsNeeded_(createNodesAsNeeded),
      force_(force),
      pathToNode_(node_path),
      path_to_defs_(path_to_defs) {
    // Parse the file and load the defs file into memory.
    std::string errMsg, warningMsg;
    defs_ptr client_defs = Defs::create();
    bool ok              = false;
    if (path_to_defs.find("suite") != std::string::npos && path_to_defs.find("endsuite") != std::string::npos) {
        ok = client_defs->restore_from_string(path_to_defs, errMsg, warningMsg);
    }
    else {
        ok = client_defs->restore(path_to_defs, errMsg, warningMsg);
    }
    if (!ok) {
        std::stringstream ss;
        ss << "ReplaceNodeCmd::ReplaceNodeCmd: Could not parse file " << path_to_defs << " : " << errMsg;
        throw std::runtime_error(ss.str());
    }

    // Make sure pathToNode exists in the client defs
    node_ptr nodeToReplace = client_defs->findAbsNode(node_path);
    if (!nodeToReplace.get()) {
        std::stringstream ss;
        ss << "ReplaceNodeCmd::ReplaceNodeCmd: Cannot replace child since path " << node_path;
        ss << ", does not exist in the client definition " << path_to_defs;
        throw std::runtime_error(ss.str());
    }

    client_defs->write_as_string(clientDefs_, PrintStyle::NET);

    // Out put any warning's to standard output
    cout << warningMsg;
}

bool ReplaceNodeCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<ReplaceNodeCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (!UserCmd::equals(rhs))
        return false;
    if (createNodesAsNeeded_ != the_rhs->createNodesAsNeeded()) {
        return false;
    }
    if (force_ != the_rhs->force())
        return false;
    if (pathToNode_ != the_rhs->pathToNode())
        return false;
    if (path_to_defs_ != the_rhs->path_to_defs())
        return false;
    if (clientDefs_ != the_rhs->the_client_defs())
        return false;
    return true;
}

STC_Cmd_ptr ReplaceNodeCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().replace_++;
    assert(isWrite()); // isWrite used in handleRequest() to control check pointing
    Defs* defs = as->defs().get();

    std::string errMsg, warningMsg;
    defs_ptr client_defs = Defs::create();
    if (!client_defs->restore_from_string(clientDefs_, errMsg, warningMsg)) {
        std::stringstream ss;
        ss << "ReplaceNodeCmd::doHandleRequest : Could not create client defs : " << errMsg;
        throw std::runtime_error(ss.str());
    }

    if (force_) {
        node_ptr node = defs->findAbsNode(pathToNode_);
        as->zombie_ctrl().add_user_zombies(node.get(), CtsApi::replace_arg());
    }

    // If we return a node_ptr then we have changed the data model, and therefore must flag node as changed.
    std::string errorMsg;
    node_ptr client_node_to_add = defs->replaceChild(pathToNode_, client_defs, createNodesAsNeeded_, force_, errorMsg);
    if (!client_node_to_add) {
        throw std::runtime_error(errorMsg);
    }

    // ECFLOW-835, flag node as changed, before check for trigger expressions.
    add_node_for_edit_history(defs, pathToNode_);

    // Although we have change the data model, Check if the trigger expressions are still valid.
    // Note:: trigger AST are not copied. If you use trigger in the test environment
    //        then copying the nodes will copy the trigger reference, which will be out of sync
    std::string warning_msg;
    if (!client_node_to_add->suite()->check(errorMsg, warning_msg)) {
        throw std::runtime_error(errorMsg);
    }

    return doJobSubmission(as);
}

bool ReplaceNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
    return do_authenticate(as, cmd, pathToNode_);
}

void ReplaceNodeCmd::print(std::string& os) const {
    std::string path_to_client_defs = path_to_defs_;
    if (path_to_client_defs.empty())
        path_to_client_defs = "<empty>"; // defs must have been loaded in memory via python api
    user_cmd(os, CtsApi::to_string(CtsApi::replace(pathToNode_, path_to_client_defs, createNodesAsNeeded_, force_)));
}
void ReplaceNodeCmd::print_only(std::string& os) const {
    std::string path_to_client_defs = path_to_defs_;
    if (path_to_client_defs.empty())
        path_to_client_defs = "<empty>"; // defs must have been loaded in memory via python api
    os += CtsApi::to_string(CtsApi::replace(pathToNode_, path_to_client_defs, createNodesAsNeeded_, force_));
}

const char* ReplaceNodeCmd::arg() {
    return CtsApi::replace_arg();
}
const char* ReplaceNodeCmd::desc() {
    /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
    return "Replaces a node in the server, with the given path\n"
           "Can also be used to add nodes in the server\n"
           "  arg1 = path to node\n"
           "         must exist in the client defs(arg2). This is also the node we want to\n"
           "         replace in the server\n"
           "  arg2 = path to client definition file\n"
           "         provides the definition of the new node\n"
           "  arg3 = (optional) [ parent | false ] (default = parent)\n"
           "         create parent families or suite as needed, when arg1 does not\n"
           "         exist in the server\n"
           "  arg4 = (optional) force (default = false) \n"
           "         Force the replacement even if it causes zombies to be created\n"
           "Replace can fail if:\n"
           "- The node path(arg1) does not exist in the provided client definition(arg2)\n"
           "- The client definition(arg2) must be free of errors\n"
           "- If the third argument is not provided, then node path(arg1) must exist in the server\n"
           "- Nodes to be replaced are in active/submitted state, in which case arg4(force) can be used\n\n"
           "Replace will preserve the suspended status, if this is not required please re-queue first\n"
           "After replace is done, we check trigger expressions. These are reported to standard output.\n"
           "It is up to the user to correct invalid trigger expressions, otherwise the tasks will *not* run.\n"
           "Please note, you can use --check to check trigger expression and limits in the server.\n"
           "For more information use --help check.\n\n"
           "Usage:\n"
           "  --replace=/suite/f1/t1 /tmp/client.def  parent      # Add/replace node tree /suite/f1/t1\n"
           "  --replace=/suite/f1/t1 /tmp/client.def  false force # replace t1 even if its active or submitted";
}

void ReplaceNodeCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(ReplaceNodeCmd::arg(), po::value<vector<string>>()->multitoken(), ReplaceNodeCmd::desc());
}
void ReplaceNodeCmd::create(Cmd_ptr& cmd,
                            boost::program_options::variables_map& vm,
                            AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (clientEnv->debug())
        dumpVecArgs(ReplaceNodeCmd::arg(), args);

    if (args.size() < 2) {
        std::stringstream ss;
        ss << "ReplaceNodeCmd: At least two arguments expected, found " << args.size()
           << " Please specify <path-to-Node>  <defs files> parent(optional) force(optional), i.e\n"
           << "--" << arg() << "=/suite/fa/t AdefsFile.def  parent force\n";
        throw std::runtime_error(ss.str());
    }

    std::string pathToNode     = args[0];
    std::string pathToDefsFile = args[1];
    bool createNodesAsNeeded   = true; // parent arg
    bool force                 = false;
    if (args.size() == 3 && args[2] == "false")
        createNodesAsNeeded = false;
    if (args.size() == 4 && args[3] == "force")
        force = true;

    cmd = std::make_shared<ReplaceNodeCmd>(pathToNode, createNodesAsNeeded, pathToDefsFile, force);
}

std::ostream& operator<<(std::ostream& os, const ReplaceNodeCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(ReplaceNodeCmd)
CEREAL_REGISTER_DYNAMIC_INIT(ReplaceNodeCmd)

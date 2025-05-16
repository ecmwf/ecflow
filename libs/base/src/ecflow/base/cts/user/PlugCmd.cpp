/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/PlugCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/Client.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/NodePath.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
    #include "ecflow/base/SslClient.hpp"
#endif

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

namespace {

/// Class to manage locking: Only unlock if acquired the lock,
class Lock {
public:
    Lock(const std::string& user, AbstractServer* as) : as_(as) { ok_ = as->lock(user); }
    ~Lock() {
        if (ok_)
            as_->unlock();
    }
    bool ok() const { return ok_; }

private:
    bool ok_;
    AbstractServer* as_;
};

static void restore(NodeContainer* container) {
    if (container && container->get_flag().is_set(ecf::Flag::ARCHIVED)) {
        container->restore();
        std::vector<family_ptr> family_vec = container->familyVec();
        for (family_ptr node : family_vec) {
            restore(node->isNodeContainer());
        }
    }
}

} // namespace

//=======================================================================================

bool PlugCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<PlugCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (source_ != the_rhs->source()) {
        return false;
    }
    if (dest_ != the_rhs->dest()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t PlugCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t PlugCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void PlugCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::plug(source_, dest_)));
}

void PlugCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::plug(source_, dest_));
}

STC_Cmd_ptr PlugCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().plug_++;
    Defs* defs = as->defs().get();

    Lock lock(user(), as);
    if (!lock.ok()) {
        std::string errorMsg = "Plug command failed. User ";
        errorMsg += as->lockedUser();
        errorMsg += " already has an exclusive lock";
        throw std::runtime_error(errorMsg);
    }

    node_ptr sourceNode = defs->findAbsNode(source_);
    if (!sourceNode.get())
        throw std::runtime_error("Plug command failed. Could not find source path " + source_);

    // Moving a node which is active, or submitted, will lead to zombie's. hence prevent
    if (sourceNode->state() == NState::ACTIVE || sourceNode->state() == NState::SUBMITTED) {
        std::string errorMsg = "Plug command failed. The source node ";
        errorMsg += source_;
        errorMsg += " is ";
        errorMsg += NState::toString(sourceNode->state());
        throw std::runtime_error(errorMsg);
    }

    if (sourceNode->isAlias()) {
        std::string errorMsg = "Plug command failed. The source node ";
        errorMsg += source_;
        errorMsg += " is a Alias. Alias cannot be moved";
        throw std::runtime_error(errorMsg);
    }

    // If the source node or *any* of its children are archived restore them first.
    // Must be done in a top down manner.
    restore(sourceNode->isNodeContainer());

    // Check to see if dest node is on the same server
    std::string host, port, destPath;
    node_ptr destNode = defs->findAbsNode(dest_);
    if (!destNode.get()) {

        // Dest could still be on the same server. Extract host and port
        // expect: host:port/suite/family/node
        if (!NodePath::extractHostPort(dest_, host, port)) {
            std::string errorMsg = "Plug command failed. The destination path ";
            errorMsg += dest_;
            errorMsg += " does not exist on server, and could not extract host/port from the destination path";
            throw std::runtime_error(errorMsg);
        }

        // Remove the host:port from the path
        destPath = NodePath::removeHostPortFromPath(dest_);

        std::pair<std::string, std::string> hostPortPair = as->hostPort();
        if ((hostPortPair.first == host || host == "localhost") && hostPortPair.second == port) {

            // Matches local server, try to find dest node again.
            destNode = defs->findAbsNode(destPath);
            if (!destNode) {
                std::string errorMsg = "Plug command failed. The destination path ";
                errorMsg += dest_;
                errorMsg += " does not exist on server ";
                errorMsg += hostPortPair.first;
                throw std::runtime_error(errorMsg);
            }
        }
        // dest_ is on another server.
    }

    if (!destNode.get()) {

        // Since host/port does not match local server, move source node to remote server
        try {
            if (destPath.empty()) {
                // Note destPath can be empty, when moving a suite
                if (!sourceNode->isSuite()) {
                    throw std::runtime_error(
                        "Destination path can only be empty when moving a whole suite to a new server");
                }
            }

            {
                // MoveCmd should inherit user and passwd, from PlugCmd
                Cmd_ptr cts_cmd = Cmd_ptr(new MoveCmd(as->hostPort(), sourceNode.get(), destPath));
                cts_cmd->setup_user_authentification(user(), passwd());

                // Server is acting like a client, Send MoveCmd to another server
                // The source should end up being copied, when sent to remote server
                ServerReply server_reply;
                boost::asio::io_context io;
#ifdef ECF_OPENSSL
                if (!as->ssl().empty()) {
                    ecf::Openssl openssl;
                    if (!openssl.enable_no_throw(host, port, as->ssl()))
                        throw std::runtime_error("PlugCmd::doHandleRequest Could not enable ssl for " + as->ssl());
                    openssl.init_for_client();

                    SslClient theClient(io, openssl.context(), cts_cmd, host, port);
                    io.run();
                    theClient.handle_server_response(server_reply, false /* debug */);
                    if (server_reply.client_request_failed()) {
                        throw std::runtime_error(server_reply.error_msg());
                    }
                }
                else {
#endif
                    Client theClient(io, cts_cmd, host, port);
                    io.run();
                    theClient.handle_server_response(server_reply, false /* debug */);
                    if (server_reply.client_request_failed()) {
                        throw std::runtime_error(server_reply.error_msg());
                    }
                }
#ifdef ECF_OPENSSL
            }
#endif

            // The move command was ok, remove the source node, and delete its memory
            sourceNode->remove();

            // Updated defs state
            defs->set_most_significant_state();

            return PreAllocatedReply::ok_cmd();
        }
        catch (std::exception& e) {
            std::stringstream ss;
            ss << "MoveCmd Failed for " << host << ":" << port << "  " << e.what() << "\n";
            throw std::runtime_error(ss.str());
        }
    }

    // source and destination on same defs file
    // See if its a sibling move:
    SuiteChanged1 suiteChanged(destNode->suite());
    SuiteChanged1 suiteChanged1(sourceNode->suite());

    if (sourceNode->parent() == destNode->parent()) {
        Node* parent = sourceNode->parent();
        if (parent)
            parent->move_peer(sourceNode.get(), destNode.get());
        else
            defs->move_peer(sourceNode.get(), destNode.get());
    }
    else {

        // If the destination is task, replace with its parent
        Node* theDestNode = destNode.get();
        if (theDestNode->isTask())
            theDestNode = theDestNode->parent();

        // Before we do remove the source node, check its ok to add it as a child
        std::string errorMsg;
        if (!theDestNode->isAddChildOk(sourceNode.get(), errorMsg)) {
            throw std::runtime_error("Plug command failed. " + errorMsg);
        }

        if (!theDestNode->addChild(sourceNode->remove())) {
            // This should never fail !!!! else we have lost/ and leaked source node !!!!
            throw std::runtime_error("Fatal error plug command failed.");
        }
    }

    add_node_for_edit_history(destNode);

    // Updated defs state
    defs->set_most_significant_state();

    return PreAllocatedReply::ok_cmd();
}

const char* PlugCmd::arg() {
    return CtsApi::plugArg();
}
const char* PlugCmd::desc() {
    return "Plug command is used to move nodes.\n"
           "The destination node can be on another server In which case the destination\n"
           "path should be of the form '<host>:<port>/suite/family/task\n"
           "  arg1 = path to source node\n"
           "  arg2 = path to the destination node\n"
           "This command can fail because:\n"
           "- Source node is in a 'active' or 'submitted' state\n"
           "- Another user already has an lock\n"
           "- source/destination paths do not exist on the corresponding servers\n"
           "- If the destination node path is empty, i.e. only host:port is specified,\n"
           "  then the source node must correspond to a suite.\n"
           "- If the source node is added as a child, then its name must be unique\n"
           "  amongst its peers\n"
           "Usage:\n"
           "  --plug=/suite macX:3141  # move the suite to ecFlow server on host(macX) and port(3141)";
}

void PlugCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(PlugCmd::arg(), po::value<vector<string>>()->multitoken(), PlugCmd::desc());
}

void PlugCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (ace->debug())
        dumpVecArgs(PlugCmd::arg(), args);

    if (args.size() != 2) {
        std::stringstream ss;
        ss << "PlugCmd: Two arguments are expected, found " << args.size() << "\n" << PlugCmd::desc() << "\n";
        throw std::runtime_error(ss.str());
    }

    std::string sourceNode = args[0];
    std::string destNode   = args[1];

    cmd = std::make_shared<PlugCmd>(sourceNode, destNode);
}

CEREAL_REGISTER_TYPE(PlugCmd)
CEREAL_REGISTER_DYNAMIC_INIT(PlugCmd)

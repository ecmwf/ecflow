// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/cts/user/PlugCmd.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/Client.hpp"
#include "ecflow/base/HelpCatalog.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/NodePath.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
    #include "ecflow/base/SslClient.hpp"
#endif

using namespace ecf;

namespace {

/// Class to manage locking: Only unlock if acquired the lock,
class Lock {
public:
    Lock(const std::string& user, AbstractServer* as)
        : as_(as) {
        ok_ = as->lock(user);
    }
    ~Lock() {
        if (ok_) {
            as_->unlock();
        }
    }
    bool ok() const { return ok_; }

private:
    bool ok_;
    AbstractServer* as_;
};

// Restores every archived container of a source, at any depth, top down; the archive files are removed unless they
// must remain (restoring a copy), and the paths of the archives read are recorded. A source moved under another
// parent, validated where it goes, is restored as archived (a transfer); a source reordered among its siblings stays
// where it is, and is restored as an explicit restore would (D3.5, D3.6)
void restore(NodeContainer* container,
             bool remove_archive,
             std::vector<std::string>& archives,
             NodeContainer::RestoreTrigger trigger = NodeContainer::RestoreTrigger::Transfer) {
    if (!container) {
        return;
    }
    if (container->get_flag().is_set(ecf::Flag::ARCHIVED)) {
        archives.push_back(container->archive_path());
        container->restore(remove_archive, trigger);
    }
    for (const family_ptr& family : container->familyVec()) {
        restore(family->isNodeContainer(), remove_archive, archives, trigger);
    }
}

// Returns whether a node, or a node below it, is archived
bool holds_archives(const Node& node) {
    if (node.get_flag().is_set(ecf::Flag::ARCHIVED)) {
        return true;
    }
    std::vector<node_ptr> children;
    node.immediateChildren(children);
    return std::any_of(children.begin(), children.end(), [](const node_ptr& c) { return holds_archives(*c); });
}

// Leaves the change numbers of the server untouched while it lives
class OutsideTheServer {
public:
    OutsideTheServer()
        : was_server_{Ecf::server()} {
        Ecf::set_server(false);
    }
    OutsideTheServer(const OutsideTheServer&)            = delete;
    OutsideTheServer& operator=(const OutsideTheServer&) = delete;
    ~OutsideTheServer() { Ecf::set_server(was_server_); }

private:
    bool was_server_;
};

///
/// Builds what a plug to another server sends: a copy of the source, its archived containers restored from their
/// archive files, which remain. The live source, its archive files and the change numbers of the server are left
/// untouched, so that a plug refused by the receiving server changes nothing.
///
/// @param[in] source The source, in the definitions of this server; it holds archived containers
/// @param[out] archives The archive files read, to remove once the plug has taken place
/// @return The scratch definitions holding the copy, at the path of the source, under a shallow copy of its
///         ancestors (so that the archive paths and ECF_HOME resolve as for the source), and the copy.
///
std::pair<defs_ptr, node_ptr> transfer_copy(const Node& source, std::vector<std::string>& archives) {
    OutsideTheServer outside;
    node_ptr copy = source.clone();

    // Recreate the ancestors, as NodeContainer::archive() does, with what resolves the archive paths
    // The scratch server holds what the archive paths need, so that nothing is added to the copy itself
    auto scratch = Defs::create();
    scratch->server_state().add_or_update_server_variable(
        ecf::environment::ECF_PORT, source.defs()->server_state().find_variable(ecf::environment::ECF_PORT));
    std::string home;
    if (source.findParentUserVariableValue(ecf::environment::ECF_HOME, home)) {
        scratch->server_state().add_or_update_server_variable(ecf::environment::ECF_HOME, home);
    }
    node_ptr top = copy;
    for (const Node* parent = source.parent(); parent; parent = parent->parent()) {
        node_ptr ancestor =
            parent->isSuite() ? node_ptr(Suite::create(parent->name())) : node_ptr(Family::create(parent->name()));
        ancestor->isNodeContainer()->addChild(top);
        top = ancestor;
    }
    scratch->addSuite(std::dynamic_pointer_cast<Suite>(top));

    restore(copy->isNodeContainer(), false, archives);
    return {scratch, copy};
}

} // namespace

//=======================================================================================

bool PlugCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<PlugCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
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

ecf::authorisation_t PlugCmd::authorise(const ecf::RequestContext& context) const {
    return implementation::do_authorise(*this, context);
}

void PlugCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::plug(source_, dest_)));
}

void PlugCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::plug(source_, dest_));
}

STC_Cmd_ptr
PlugCmd::denied_by_receiver(AbstractServer& server, const std::string& host, const std::string& port) const {
    // Nothing took place: no edit history is recorded, although the lock taken for the plug moved the change numbers
    use_EditHistoryMgr_ = false;
    ecf::LogFlusher flusher; // the message, logged by the reply, is written at once
    return PreAllocatedReply::denied_cmd(ecf::rejection_message(
        "Authorisation (user) failed, due to: the receiving server " + host + ":" + port + " denied the move", *this));
}

std::optional<std::string> PlugCmd::local_destination(const AbstractServer& server) const {
    const auto defs = server.defs();
    if (defs->findAbsNode(dest_)) {
        return dest_;
    }
    std::string host, port;
    if (!ecf::node::extract_host_and_port_from_path(dest_, host, port)) {
        return std::nullopt;
    }
    const auto here = server.hostPort();
    if ((here.first == host || host == "localhost") && here.second == port) {
        auto path = ecf::node::remove_host_and_port_from_path(dest_);
        if (defs->findAbsNode(path)) {
            return path;
        }
    }
    return std::nullopt;
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
    if (!sourceNode.get()) {
        throw std::runtime_error("Plug command failed. Could not find source path " + source_);
    }

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

    // Check to see if dest node is on the same server
    std::string host, port, destPath;
    node_ptr destNode = defs->findAbsNode(dest_);
    if (!destNode.get()) {

        // Dest could still be on the same server. Extract host and port
        // expect: host:port/suite/family/node
        if (!ecf::node::extract_host_and_port_from_path(dest_, host, port)) {
            std::string errorMsg = "Plug command failed. The destination path ";
            errorMsg += dest_;
            errorMsg += " does not exist on server, and could not extract host/port from the destination path";
            throw std::runtime_error(errorMsg);
        }

        // Remove the host:port from the path
        destPath = ecf::node::remove_host_and_port_from_path(dest_);

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

            // A copy with its archived containers restored: nothing changes here until the plug takes place
            std::vector<std::string> archives;
            {
                // The move is authorised by the receiving server under the identity of the requester, as
                // authenticated here, custom user or not (R11.9)
                std::shared_ptr<MoveCmd> move;
                if (holds_archives(*sourceNode)) {
                    const auto [scratch, payload] = transfer_copy(*sourceNode, archives);
                    move                          = std::make_shared<MoveCmd>(as->hostPort(), payload.get(), destPath);
                }
                else {
                    move = std::make_shared<MoveCmd>(as->hostPort(), sourceNode.get(), destPath);
                }
                move->setup_user_authentification(identity());
                Cmd_ptr cts_cmd = move;

                // Server is acting like a client, Send MoveCmd to another server
                // The source should end up being copied, when sent to remote server
                ServerReply server_reply;
                boost::asio::io_context io;
#ifdef ECF_OPENSSL
                if (!as->ssl().empty()) {
                    ecf::Openssl openssl;
                    if (!openssl.try_enable_for_peer(host, port, as->ssl())) {
                        throw std::runtime_error("PlugCmd::doHandleRequest Could not enable ssl for " + as->ssl());
                    }
                    openssl.init_for_client();

                    SslClient theClient(io, openssl.context(), cts_cmd, host, port);
                    io.run();
                    theClient.handle_server_response(server_reply, false /* debug */);
                    if (server_reply.permission_denied()) {
                        return denied_by_receiver(*as, host, port);
                    }
                    if (server_reply.client_request_failed()) {
                        throw std::runtime_error(server_reply.error_msg());
                    }
                }
                else {
#endif
                    Client theClient(io, cts_cmd, host, port);
                    io.run();
                    theClient.handle_server_response(server_reply, false /* debug */);
                    if (server_reply.permission_denied()) {
                        return denied_by_receiver(*as, host, port);
                    }
                    if (server_reply.client_request_failed()) {
                        throw std::runtime_error(server_reply.error_msg());
                    }
                }
#ifdef ECF_OPENSSL
            }
#endif

            // The move command was ok, remove the source node, and delete its memory and archives; its attributes
            // leave this server (the receiving server validated and logged them where they go)
            ecf::PermissionsChange permissions(*defs);
            permissions.record_removal(*sourceNode);
            sourceNode->remove();
            permissions.log(identity().username().value() + "@" + hostname());
            for (const auto& archive : archives) {
                std::error_code error;
                fs::remove(archive, error);
            }

            // Updated defs state
            defs->set_most_significant_state();

            return PreAllocatedReply::ok_cmd();
        }
        catch (std::exception& e) {
            throw std::runtime_error(MESSAGE("MoveCmd Failed for " << host << ":" << port << "  " << e.what() << "\n"));
        }
    }

    // source and destination on same defs file
    // See if its a sibling move:
    SuiteChanged1 suiteChanged(destNode->suite());
    SuiteChanged1 suiteChanged1(sourceNode->suite());

    // The destination is checked before the source is restored, so that a refused plug leaves the source unchanged
    Node* theDestNode = destNode.get();
    if (sourceNode->parent() != destNode->parent()) {
        // If the destination is task, replace with its parent
        if (theDestNode->isTask()) {
            theDestNode = theDestNode->parent();
        }
        std::string errorMsg;
        if (!theDestNode->isAddChildOk(sourceNode.get(), errorMsg)) {
            throw std::runtime_error("Plug command failed. " + errorMsg);
        }
    }

    // Moving the source under another parent changes the attributes applying to it: they are validated where the
    // source goes (R4.5), before anything changes
    ecf::PermissionsChange permissions(*defs);
    if (sourceNode->parent() != destNode->parent()) {
        permissions.record_removal(*sourceNode);
        if (holds_archives(*sourceNode)) {
            // The archived containers are validated too, on a copy restored from their archives, which remain
            std::vector<std::string> read;
            const auto [scratch, copy] = transfer_copy(*sourceNode, read);
            permissions.validate_move(*copy, sourceNode->parent(), theDestNode);
        }
        else {
            permissions.validate_move(*sourceNode, sourceNode->parent(), theDestNode);
        }
    }

    // If the source node or *any* of its children are archived restore them first. Must be done in a top down manner.
    std::vector<std::string> archives;
    restore(sourceNode->isNodeContainer(),
            true,
            archives,
            sourceNode->parent() == destNode->parent() ? NodeContainer::RestoreTrigger::Requested
                                                       : NodeContainer::RestoreTrigger::Transfer);

    if (sourceNode->parent() == destNode->parent()) {
        Node* parent = sourceNode->parent();
        if (parent) {
            parent->move_peer(sourceNode.get(), destNode.get());
        }
        else {
            defs->move_peer(sourceNode.get(), destNode.get());
        }
    }
    else {
        if (!theDestNode->addChild(sourceNode->remove())) {
            // This should never fail !!!! else we have lost/ and leaked source node !!!!
            throw std::runtime_error("Fatal error plug command failed.");
        }
    }
    permissions.log(identity().username().value() + "@" + hostname());

    add_node_for_edit_history(destNode);

    // Updated defs state
    defs->set_most_significant_state();

    return PreAllocatedReply::ok_cmd();
}

const char* PlugCmd::arg() {
    return CtsApi::plugArg();
}

void PlugCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(PlugCmd::arg(), boost::program_options::value<std::vector<std::string>>()->multitoken());
}

void PlugCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    auto args = vm[arg()].as<std::vector<std::string>>();

    if (ace->debug()) {
        dumpVecArgs(PlugCmd::arg(), args);
    }

    if (args.size() != 2) {
        throw std::runtime_error(MESSAGE("PlugCmd: Two arguments are expected, found "
                                         << args.size() << "\n"
                                         << HelpCatalog::description_for("plug").value_or(HelpCatalog::not_provided)
                                         << "\n"));
    }

    std::string sourceNode = args[0];
    std::string destNode   = args[1];

    cmd = std::make_shared<PlugCmd>(sourceNode, destNode);
}

CEREAL_REGISTER_TYPE(PlugCmd)
CEREAL_REGISTER_DYNAMIC_INIT(PlugCmd)

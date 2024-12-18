/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/MoveCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
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

} // namespace

//=======================================================================================

MoveCmd::MoveCmd(const std::pair<std::string, std::string>& host_port, Node* src, const std::string& dest)
    : src_node_(ecf::as_string(src, PrintStyle::NET)),
      src_host_(host_port.first),
      src_port_(host_port.second),
      src_path_(src->absNodePath()),
      dest_(dest) {
}

MoveCmd::MoveCmd()  = default;
MoveCmd::~MoveCmd() = default;

bool MoveCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<MoveCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (dest_ != the_rhs->dest()) {
        return false;
    }
    if (src_node_ != the_rhs->src_node()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t MoveCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t MoveCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void MoveCmd::print(std::string& os) const {
    std::string ss;
    ss += "Plug(Move) source(";
    ss += src_host_;
    ss += ":";
    ss += src_port_;
    ss += ":";
    ss += src_path_;
    ss += ") destination(";
    ss += dest_;
    ss += ")";
    user_cmd(os, ss);
}

bool MoveCmd::check_source() const {
    return !src_node_.empty();
}

STC_Cmd_ptr MoveCmd::doHandleRequest(AbstractServer* as) const {
    Defs* defs = as->defs().get();

    Lock lock(user(), as);
    if (!lock.ok()) {
        std::string errorMsg = "Plug(Move) command failed. User ";
        errorMsg += as->lockedUser();
        errorMsg += " already has an exclusive lock";
        throw std::runtime_error(errorMsg);
    }

    if (!check_source()) {
        throw std::runtime_error("Plug(Move) command failed. No source specified");
    }

    std::string error_msg;
    node_ptr src_node = Node::create(src_node_, error_msg);
    if (!error_msg.empty() || !src_node) {
        throw std::runtime_error("Plug(Move) command failed. Error in source:\n" + error_msg);
    }

    // destNode can be NULL when we are moving a suite
    node_ptr destNode;
    if (!dest_.empty()) {

        destNode = defs->findAbsNode(dest_);
        if (!destNode.get()) {
            std::string errorMsg = "Plug(Move) command failed. The destination path ";
            errorMsg += dest_;
            errorMsg += " does not exist on server";
            throw std::runtime_error(errorMsg);
        }
    }
    else {
        if (!src_node->isSuite()) {
            throw std::runtime_error("::Destination path can only be empty when moving a whole suite to a new server");
        }
    }

    if (destNode.get()) {

        // The destNode containing suite may be in a handle
        SuiteChanged0 suiteChanged(destNode);

        // If the destination is task, replace with its parent
        Node* thedestNode = destNode.get();
        if (thedestNode->isTask())
            thedestNode = thedestNode->parent();

        // check its ok to add
        std::string errorMsg;
        if (!thedestNode->isAddChildOk(src_node.get(), errorMsg)) {
            std::string msg = "Plug(Move) command failed. ";
            msg += errorMsg;
            throw std::runtime_error(msg);
        }

        // pass ownership
        if (!thedestNode->addChild(src_node)) {
            // This should never fail !!!! else we have lost/ and leaked source node !!!!
            throw std::runtime_error("Fatal error plug(move) command failed. cannot addChild");
        }

        add_node_for_edit_history(destNode);
    }
    else {

        if (!src_node->isSuite())
            throw std::runtime_error("plug(move): Source node was expected to be a suite");

        // convert node_ptr to suite_ptr
        suite_ptr the_source_suite = std::dynamic_pointer_cast<Suite>(src_node);

        // The sourceSuite may be in a handle or pre-registered suite
        SuiteChanged suiteChanged(the_source_suite);

        defs->addSuite(the_source_suite);

        add_node_for_edit_history(the_source_suite);
    }

    defs->set_most_significant_state();

    // Ownership for src_node has been passed on.
    return PreAllocatedReply::ok_cmd();
}

const char* MoveCmd::arg() {
    return "move";
}
const char* MoveCmd::desc() {
    return "The move command is an internal cmd, Called by the plug cmd. Does not appear on public api.";
}

void MoveCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(MoveCmd::arg(), po::value<vector<string>>()->multitoken(), MoveCmd::desc());
}

void MoveCmd::create(Cmd_ptr&, boost::program_options::variables_map&, AbstractClientEnv*) const {
    assert(false);
}

std::ostream& operator<<(std::ostream& os, const MoveCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(MoveCmd)
CEREAL_REGISTER_DYNAMIC_INIT(MoveCmd)

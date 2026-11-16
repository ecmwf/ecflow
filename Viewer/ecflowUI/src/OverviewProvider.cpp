// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "OverviewProvider.hpp"

#include "ConnectState.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VFileInfo.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "VSState.hpp"
#include "VTriggerAttr.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

namespace {

///
/// @brief Returns a key of the header of the Info panel, padded to a common width and followed by " : ".
///
/// The keys are aligned on the width of the longest one: "permissions" when that line is shown, otherwise the width
/// used since before Permissions (7, as in "status  : ").
///
/// @param[in] key The key, e.g. "name"
/// @param[in] withPermissions Whether the header holds the line "permissions"
/// @return The padded key, e.g. "name    : "
///
std::string headerKey(const std::string& key, bool withPermissions) {
    static const std::size_t plainWidth       = 7;
    static const std::size_t permissionsWidth = std::string("permissions").size();
    const std::size_t width                   = withPermissions ? permissionsWidth : plainWidth;
    return key + std::string(width > key.size() ? width - key.size() : 0, ' ') + " : ";
}

} // namespace

OverviewProvider::OverviewProvider(InfoPresenter* owner)
    : InfoProvider(owner, VTask::NoTask) {
}

//==================================================
// Server
//==================================================

void OverviewProvider::visit(VInfoServer* info) {
    reply_->reset();

    // Build the second part of the server info. We do not need
    // information from the ClientInvoker for this!!
    std::stringstream ss;
    serverInfo(info, ss);
    reply_->text(ss.str());

    // If not connected we reply immediately!
    if (info->server()->connectState()->state() != ConnectState::Normal) {
        owner_->infoReady(reply_);
        return;
    }

    // Define a task for getting the stats from the server.
    // We need ClientInvoker for this
    task_ = VTask::create(VTask::StatsTask, this);

    // Run the task in the server. When it finish taskFinished() is called. The text returned
    // in the reply will be prepended to the string we generated above.
    info->server()->run(task_);
}

// Node
void OverviewProvider::visit(VInfoNode* ni) {
    reply_->reset();
    std::stringstream ss;
    nodeInfo(ni, ss);
    reply_->text(ss.str());
    owner_->infoReady(reply_);
}

void OverviewProvider::visit(VInfoAttribute* /*ptr*/) {
}

void OverviewProvider::taskChanged(VTask_ptr task) {
    if (task_ != task) {
        return;
    }

    switch (task->status()) {
        case VTask::FINISHED:
            // We prepend the results to the existing text
            reply_->prependText(task->reply()->text());
            owner_->infoReady(reply_);
            // We do not need the task anymore.
            task_.reset();
            break;
        case VTask::ABORTED:
        case VTask::CANCELLED:
        case VTask::REJECTED:
            reply_->prependText(task->reply()->text());
            owner_->infoFailed(reply_);
            // We do not need the task anymore.
            task_.reset();
            break;
        default:
            break;
    }
}

void OverviewProvider::serverInfo(VInfoServer* info, std::stringstream& f) {
    static const std::string inc = "  ";

    ServerHandler* server = info->server();
    if (!server) {
        return;
    }
    VServer* snode = server->vRoot();

    ConnectState* cst = server->connectState();

    // If the server is not connected!!
    if (cst->state() != ConnectState::Normal) {
        f << cst->describe() << "\n";
        f << inc << "Name    : " << server->name() << "\n";
        f << inc << "Host    : " << server->host() << "\n";
        f << inc << "Port    : " << server->port() << "\n";

        f << inc << "Client protocol : " << ecf::to_ui_designation(server->protocol()) << "\n";

        // The peer protocol is stated only when it was positively determined by probing the
        // endpoint. Naming a protocol that was merely guessed would send the reader to change the
        // wrong setting.
        const ecf::ConnectionDiagnosis& diagnosis = cst->diagnosis();
        if (diagnosis.peer_protocol) {
            f << inc << "Server protocol : " << ecf::to_ui_designation(diagnosis.peer_protocol.value())
              << "  (determined by probing the server)\n";
        }

        if (!server->user().empty()) {
            f << inc << "Custom user : " << server->user() << "\n";
        }

        if (cst->state() == ConnectState::Lost) {
            f << inc << "Last connection attempt  : " << VFileInfo::formatDate(cst->lastLostTime()).toStdString()
              << "\n";
            f << "\n";
            if (!cst->errorMessage().empty()) {
                f << "Error message:\n";
                f << cst->errorMessage();
            }
        }
        else if (cst->state() == ConnectState::Disconnected) {
            f << inc << "Disconnected  : " << VFileInfo::formatDate(cst->lastDisconnectTime()).toStdString() << "\n";
        }
        else if (cst->state() == ConnectState::VersionIncompatible) {
            f << inc << "Incompatible server version! " << cst->errorMessage() << "\n";
        }
        else if (cst->state() == ConnectState::SslCertificateError) {
            f << inc << "SSL certificate error! " << cst->errorMessage() << "\n";
        }
        else if (cst->state() == ConnectState::FailedClient) {
            f << inc << "Client creation error! " << cst->errorMessage() << "\n";
        }
        else if (cst->state() == ConnectState::ProtocolMismatch) {
            f << inc << "Last connection attempt : " << VFileInfo::formatDate(cst->lastLostTime()).toStdString()
              << "\n";
            f << "\n";
            f << "Diagnosis:\n";
            f << ecf::explain(diagnosis) << "\n";
            if (!diagnosis.detail.empty()) {
                f << "\n";
                f << inc << "Low-level error : " << diagnosis.detail << "\n";
            }
        }
        return;
    }

    // if(!ServerDefsAccess(server).defs()) return;

    std::string typeName = "server";
    std::string nodeName = server->name();
    std::string statusName(VSState::toName(server).toStdString());
    std::string flags(snode->flagsAsStr());

    // The effective roles at server level (the server attribute is not shown as a row in the tree)
    const auto permissions     = effectivePermissions(server, "/");
    const bool withPermissions = permissions.has_value();

    // Header
    f << headerKey("name", withPermissions) << nodeName << "\n";
    f << headerKey("type", withPermissions) << typeName << "\n";
    f << headerKey("status", withPermissions) << statusName << "\n";

    if (!flags.empty()) {
        f << headerKey("flags", withPermissions) << flags << "\n";
    }

    if (permissions) {
        f << headerKey("permissions", withPermissions) << *permissions << "\n";
    }

    f << "----------\n";

    if (snode->isFlagSet(ecf::Flag::LOG_ERROR)) {
        const std::string& errTxt = snode->findVariable("ECF_LOG_ERROR", true);
        if (!errTxt.empty()) {
            f << "log error : " + errTxt + "\n";
            f << "----------\n";
        }
    }

    if (snode->isFlagSet(ecf::Flag::CHECKPT_ERROR)) {
        const std::string& errTxt = snode->findVariable("ECF_CHECKPT_ERROR", true);
        if (!errTxt.empty()) {
            f << "checkpoint error : " + errTxt + "\n";
            f << "----------\n";
        }
    }

    // Start block: Type, name
    f << typeName << " " << server->name() << "\n";

    // Generated variables
    std::vector<Variable> gvar;
    snode->genVariables(gvar);
    for (auto it = gvar.begin(); it != gvar.end(); ++it) {
        f << inc << "# edit " << (*it).name() << " '" << (*it).value() << "'\n";
    }

    // Variables
    std::vector<Variable> var;
    snode->variables(var);
    for (auto it = var.begin(); it != var.end(); ++it) {
        f << inc << "edit " << (*it).name() << " '" << (*it).value() << "'\n";
    }

    // Print children
    VNode* vr = server->vRoot();
    for (int i = 0; i < vr->numOfChildren(); i++) {
        f << inc << vr->childAt(i)->nodeType() << " " << vr->childAt(i)->strName() << "\n";
    }

    // End block
    f << "end" << typeName << " # " << nodeName << "\n";
}

std::optional<std::string> OverviewProvider::effectivePermissions(ServerHandler* server, const std::string& path) {
    if (server->isAccessDenied()) {
        return std::nullopt;
    }
    ServerDefsAccess defsAccess(server); // relinquishes its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (!defs) {
        return std::nullopt;
    }
    try {
        // Computed on the client's copy with the rules of the server: every attribute on the path is held by the
        // copy, since a node is only sent with all its ancestors and their attributes
        if (const auto users = ecf::effective_permissions_of_every_user(*defs, path); users) {
            std::string line;
            for (const auto& [user, roles] : *users) {
                line += (line.empty() ? "" : ", ") + user + ":" + roles.to_string();
            }
            return line;
        }
    }
    catch (const std::invalid_argument&) {
        // The node is gone from the copy (e.g. during an update): nothing is shown
    }
    return std::nullopt;
}

void OverviewProvider::nodeInfo(VInfoNode* info, std::stringstream& f) {
    ServerHandler* server = info->server();
    if (!server) {
        return;
    }
    // if(!ServerDefsAccess(server).defs()) return;

    VNode* node = info->node();
    if (!node) {
        return;
    }

    static const std::string inc = "  ";

    std::string typeName = node->nodeType();
    std::string nodeName(node->name().toStdString());
    std::string statusName(node->stateName().toStdString());
    std::string flags(node->flagsAsStr());

    node_ptr nn = node->node();

    const auto permissions     = effectivePermissions(server, nn->absNodePath());
    const bool withPermissions = permissions.has_value();

    // Header
    f << headerKey("name", withPermissions) << nodeName << "\n";
    f << headerKey("type", withPermissions) << typeName << "\n";
    f << headerKey("status", withPermissions) << statusName << "\n";

    if (!flags.empty()) {
        f << headerKey("flags", withPermissions) << flags << "\n";
    }

    auto state_change_time = nn->state_change_time();
    if (!state_change_time.is_special()) {
        f << headerKey("at", withPermissions) << boost::posix_time::to_simple_string(state_change_time) << "\n";
    }

    if (permissions) {
        f << headerKey("permissions", withPermissions) << *permissions << "\n";
    }

    f << "----------\n";

    if (node->isAborted()) {
        const std::string& abTxt = node->abortedReason();
        if (!abTxt.empty()) {
            f << "aborted reason : " + abTxt + "\n";
            f << "----------\n";
        }
    }

    // Start block: Type, name
    f << typeName << " " << nodeName << "\n";

    // Clock information for suites
    if (Suite* suite = nn->isSuite()) {
        // Suite* suite = dynamic_cast<Suite*>(nn);
        //  f << "clock    : ";
        if (suite->clockAttr()) {
            f << ecf::as_string(*suite->clockAttr(), PrintStyle::DEFS);
        }
    }

    // Default status: the status the node should have when the begin/re-queue is called
    // if(st  != DState::QUEUED && st != DState::UNKNOWN)
    f << inc << "defstatus " << node->defaultStateName().toStdString() << "\n";

    // Zombies attribute
    const std::vector<ZombieAttr>& vect = nn->zombies();
    for (const auto& it : vect) {
        f << inc << it.toString() << "\n";
    }

    // For suspended nodes
    if (nn->isSuspended()) {
        f << inc << "# " << typeName << " " << nodeName << " is " << statusName << "\n";
    }

    if (nn->hasTimeDependencies()) {
        f << inc << "# time-date-dependencies: ";
        if (nn->isTimeFree()) {
            f << "free\n";
        }
        else {
            f << "holding\n";
        }
    }

    // Generated variables
    std::vector<Variable> gvar;
    node->genVariables(gvar);
    for (auto it = gvar.begin(); it != gvar.end(); ++it) {
        f << inc << "# edit " << (*it).name() << " '" << (*it).value() << "'\n";
    }

    // Variables
    std::vector<Variable> var;
    node->variables(var);
    for (auto it = var.begin(); it != var.end(); ++it) {
        f << inc << "edit " << (*it).name() << " '" << (*it).value() << "'\n";
    }

    // Other attributes
    const std::vector<VAttribute*>& attr = node->attr();
    for (auto it : attr) {
        if (it->typeName() != "var" && it->typeName() != "genvar") {
            f << inc << it->definition().toStdString() << "\n";

            if (it->typeName() == "trigger" || it->typeName() == "complete") {
                if (auto* tgAtt = static_cast<VTriggerAttr*>(it)) {
                    std::string trigger_ast = tgAtt->ast_str();
                    if (!trigger_ast.empty()) {
                        f << inc << trigger_ast;
                        if (trigger_ast[trigger_ast.size() - 1] != '\n') {
                            f << "\n";
                        }
                    }
                }
            }
        }
    }

    // Print children
    for (int i = 0; i < node->numOfChildren(); i++) {
        f << inc << node->childAt(i)->nodeType() << " " << node->childAt(i)->strName() << "\n";
    }

    // Here we should print some additional information from the attributes as well. It is not clear exactly what!

    // End block
    f << "end" << typeName << " # " << nodeName << "\n";
}

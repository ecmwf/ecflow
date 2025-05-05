/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "OverviewProvider.hpp"

#include "ConnectState.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VFileInfo.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "VSState.hpp"
#include "VTriggerAttr.hpp"
#include "ecflow/node/Suite.hpp"

OverviewProvider::OverviewProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::NoTask) {
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
    if (task_ != task)
        return;

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
    if (!server)
        return;
    VServer* snode = server->vRoot();

    ConnectState* cst = server->connectState();

    // If the server is not connected!!
    if (cst->state() != ConnectState::Normal) {
        f << cst->describe() << "\n";
        f << inc << "Name    : " << server->name() << "\n";
        f << inc << "Host    : " << server->host() << "\n";
        f << inc << "Port    : " << server->port() << "\n";

        f << inc << "Protocol : " << ecf::to_ui_designation(server->protocol()) << " : enabled\n";

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
        else if (cst->state() == ConnectState::SslIncompatible) {
            f << inc << "Possible SSL-related incompatibility! " << cst->errorMessage() << "\n";
        }
        else if (cst->state() == ConnectState::SslCertificateError) {
            f << inc << "SSL certificate error! " << cst->errorMessage() << "\n";
        }
        else if (cst->state() == ConnectState::FailedClient) {
            f << inc << "Client creation error! " << cst->errorMessage() << "\n";
        }
        return;
    }

    // if(!ServerDefsAccess(server).defs()) return;

    using namespace boost::posix_time;
    using namespace boost::gregorian;

    std::string typeName = "server";
    std::string nodeName = server->name();
    std::string statusName(VSState::toName(server).toStdString());
    std::string flags(snode->flagsAsStr());

    // Header
    f << "name    : " << nodeName << "\n";
    f << "type    : " << typeName << "\n";
    f << "status  : " << statusName << "\n";

    if (!flags.empty())
        f << "flags   : " << flags << "\n";

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
        f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    // Variables
    std::vector<Variable> var;
    snode->variables(var);
    for (auto it = var.begin(); it != var.end(); ++it) {
        f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    // Print children
    VNode* vr = server->vRoot();
    for (int i = 0; i < vr->numOfChildren(); i++) {
        f << inc << vr->childAt(i)->nodeType() << " " << vr->childAt(i)->strName() << "\n";
    }

    // End block
    f << "end" << typeName << " # " << nodeName << "\n";
}

void OverviewProvider::nodeInfo(VInfoNode* info, std::stringstream& f) {
    ServerHandler* server = info->server();
    if (!server)
        return;
    // if(!ServerDefsAccess(server).defs()) return;

    VNode* node = info->node();
    if (!node)
        return;

    static const std::string inc = "  ";

    using namespace boost::posix_time;
    using namespace boost::gregorian;

    std::string typeName = node->nodeType();
    std::string nodeName(node->name().toStdString());
    std::string statusName(node->stateName().toStdString());
    std::string flags(node->flagsAsStr());

    // Header
    f << "name    : " << nodeName << "\n";
    f << "type    : " << typeName << "\n";
    f << "status  : " << statusName << "\n";

    if (!flags.empty()) {
        f << "flags   : " << flags << "\n";
    }

    node_ptr nn = node->node();

    boost::posix_time::ptime state_change_time = nn->state_change_time();
    if (!state_change_time.is_special()) {
        f << "at      : " << boost::posix_time::to_simple_string(state_change_time) << "\n";
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
            std::string s;
            suite->clockAttr().get()->print(s);
            f << s; // f << "\n";
        }
    }

    // Default status: the status the node should have when the begin/re-queue is called
    // if(st  != DState::QUEUED && st != DState::UNKNOWN)
    f << inc << "defstatus " << node->defaultStateName().toStdString() << "\n";

    // Zombies attribute
    const std::vector<ZombieAttr>& vect = nn->zombies();
    for (const auto& it : vect)
        f << inc << it.toString() << "\n";

    // For suspended nodes
    if (nn->isSuspended()) {
        f << inc << "# " << typeName << " " << nodeName << " is " << statusName << "\n";
    }

    if (nn->hasTimeDependencies()) {
        f << inc << "# time-date-dependencies: ";
        if (nn->isTimeFree())
            f << "free\n";
        else
            f << "holding\n";
    }

    // Generated variables
    std::vector<Variable> gvar;
    node->genVariables(gvar);
    for (auto it = gvar.begin(); it != gvar.end(); ++it) {
        f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    // Variables
    std::vector<Variable> var;
    node->variables(var);
    for (auto it = var.begin(); it != var.end(); ++it) {
        f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
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

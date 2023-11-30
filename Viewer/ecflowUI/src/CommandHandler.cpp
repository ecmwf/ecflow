/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "CommandHandler.hpp"

#include <QMessageBox>
#include <QString>
#include <boost/program_options/parsers.hpp>

#include "ActionHandler.hpp"
#include "ServerHandler.hpp"
#include "ShellCommand.hpp"
#include "Str.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VAttribute.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
    #include <QRegExp>
#endif
#include <map>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

// Send the same command for a list of objects (nodes/servers) specified in a VInfo vector.
// The command is specified as a string.

std::string CommandHandler::executeCmd_ = "ecflow_client --run <full_name>";
std::string CommandHandler::rerunCmd_   = "ecflow_client --force queued <full_name>";
std::string CommandHandler::killCmd_    = "ecflow_client --kill <full_name>";

void CommandHandler::run(std::vector<VInfo_ptr> info, const std::string& cmd) {
    UI_FUNCTION_LOG

    if (cmd.empty()) {
        // UiLog().err() << " command is not recognised. Check the menu definition.";
        UserMessage::message(UserMessage::ERROR, true, "Empty command is not recognised. Check the menu definition.");
        return;
    }

    UiLog().dbg() << "command='" << cmd << "'";

    std::map<ServerHandler*, std::string> targetNodeNames;
    std::map<ServerHandler*, std::string> targetNodeFullNames;
    std::map<ServerHandler*, std::string> targetParentFullNames;

    std::vector<ServerHandler*> targetServers;

    // Figure out what objects (node/server) the command should be applied to
    for (const auto& i : info) {
        std::string nodeFullName;
        std::string nodeName;
        std::string parentFullName;

        if (cmd.find("<node_name>") != std::string::npos) {
            nodeName = i->name();
        }

        if (cmd.find("<full_name>") != std::string::npos) {
            if (i->isNode()) {
                nodeFullName = i->node()->absNodePath();
            }
            else if (i->isServer()) {
                i->server()->longName();
            }
            else if (i->isAttribute()) {
                parentFullName = i->node()->absNodePath();
            }
        }

        if (cmd.find("<parent_name>") != std::string::npos) {
            if (i->isNode()) {
                if (VNode* p = i->node()->parent()) {
                    parentFullName = p->absNodePath();
                }
            }
            else if (i->isAttribute()) {
                parentFullName = i->node()->absNodePath();
            }
        }

        // Store the names per target servers
        targetNodeNames[i->server()] += " " + nodeName;
        targetNodeFullNames[i->server()] += " " + nodeFullName;
        targetParentFullNames[i->server()] += " " + parentFullName;

        // add this to our list of target servers?
        if (std::find(targetServers.begin(), targetServers.end(), i->server()) == targetServers.end()) {
            targetServers.push_back(i->server());
        }
    }

    // for each target server, construct and send its command
    for (const auto& serverHandler : targetServers) {
        std::string realCommand = cmd;

        // replace placeholders with real node names
        std::string placeholder("<full_name>");
        ecf::Str::replace_all(realCommand, placeholder, targetNodeFullNames[serverHandler]);

        placeholder = "<node_name>";
        ecf::Str::replace_all(realCommand, placeholder, targetNodeNames[serverHandler]);

        placeholder = "<parent_name>";
        ecf::Str::replace_all(realCommand, placeholder, targetParentFullNames[serverHandler]);

        // Shell command
        if (realCommand.find("sh ") == 0) {
            substituteVariables(realCommand, info);
            UiLog().dbg() << " final shell command: " << realCommand;
            ShellCommand::run(realCommand, cmd);
            return;
        }

        UiLog().dbg() << " final command: " << realCommand;

        // set up and run the thread for server communication
        serverHandler->runCommand(realCommand);
    }
}

// Send a command to a server. The command is specified as a string vector, while the node or server for that
// the command will be applied is specified in a VInfo object.
void CommandHandler::run(VInfo_ptr info, const std::vector<std::string>& cmd) {
    UI_FUNCTION_LOG

    std::vector<std::string> realCommand = cmd;

    if (realCommand.empty()) {
        // UiLog().err() << " command is not recognised!";
        UserMessage::message(UserMessage::ERROR, true, "command is not recognised.");
    }

    UiLog().dbg() << "command: " << commandToString(realCommand);

    // Get the name of the object for that the command will be applied
    std::string nodeFullName;
    std::string nodeName;
    ServerHandler* serverHandler = info->server();

    if (info->isNode() || info->isAttribute()) {
        nodeFullName = info->node()->node()->absNodePath();
        nodeName     = info->node()->node()->name();
        // UserMessage::message(UserMessage::DBG, false, std::string("  --> for node: ") + nodeFullName + " (server: " +
        // info[i]->server()->longName() + ")");
    }
    else if (info->isServer()) {
        nodeFullName = "/";
        nodeName     = "/";
        // UserMessage::message(UserMessage::DBG, false, std::string("  --> for server: ") + nodeFullName);
    }

    // Replace placeholders with real node names
    for (std::size_t i = 0; i < cmd.size(); i++) {
        if (realCommand[i] == "<full_name>")
            realCommand[i] = nodeFullName;
        else if (realCommand[i] == "<node_name>")
            realCommand[i] = nodeName;
    }

    UiLog().dbg() << " final command: " << commandToString(realCommand);

    // set up and run the thread for server communication
    serverHandler->runCommand(realCommand);
}

void CommandHandler::run(VInfo_ptr info, const std::string& cmd) {
    std::vector<VInfo_ptr> infos = {info};
    run(infos, cmd);
}

void CommandHandler::openLinkInBrowser(VInfo_ptr info) {
    if (info && info->isAttribute()) {
        if (VAttribute* a = info->attribute()) {
            std::string str_val;
            if (a->value("label_value", str_val)) {
                QString s = QString::fromStdString(str_val);
                QString url;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
                QRegularExpression rx("(http:\\/\\/\\S+|https:\\/\\/\\S+)");
                auto match = rx.match(s);
                if (match.hasMatch()) {
                    url = match.captured(1);
                }
#else
                QRegExp rx("(http:\\/\\/\\S+|https:\\/\\/\\S+)");
                if (rx.indexIn(s) >= 0) {
                    url = rx.cap(1);
                }
#endif
                if (!url.isEmpty()) {
                    QString browser;
                    if (VProperty* prop = VConfig::instance()->find("menu.web.defaultBrowser")) {
                        browser = prop->valueAsString();
                    }

                    if (browser.isEmpty())
                        browser = "firefox";

                    std::string cmd = "sh " + browser.toStdString() + " " + url.toStdString();
                    ShellCommand::run(cmd, cmd, false);
                }
            }
        }
    }
}

void CommandHandler::executeAborted(const std::vector<VNode*>& nodes) {
    std::vector<VInfo_ptr> info_vec;
    for (size_t i = 0; i < nodes.size(); i++) {
        info_vec.push_back(VInfoNode::create(nodes[i]));
        assert(nodes[i]);
        assert(nodes[i]->isTask());
    }

    CommandHandler::run(info_vec, executeCmd_);
}

void CommandHandler::rerunAborted(const std::vector<VNode*>& nodes) {
    std::vector<VInfo_ptr> info_vec;
    for (size_t i = 0; i < nodes.size(); i++) {
        info_vec.push_back(VInfoNode::create(nodes[i]));
        assert(nodes[i]);
        assert(nodes[i]->isTask());
    }

    CommandHandler::run(info_vec, rerunCmd_);
}

bool CommandHandler::kill(const std::vector<VNode*>& nodes, bool confirm) {
    std::vector<VInfo_ptr> info_vec;
    for (size_t i = 0; i < nodes.size(); i++) {
        info_vec.push_back(VInfoNode::create(nodes[i]));
        assert(nodes[i]);
        // assert(nodes[i]->isTask());
    }

    if (confirm) {
        if (!ActionHandler::confirmCommand(
                info_vec, "Confirm <b>killing</b> of <full_name>", "", killCmd_, info_vec.size())) {
            return false;
        }
    }

    CommandHandler::run(info_vec, killCmd_);

    return true;
}

std::string CommandHandler::commandToString(const std::vector<std::string>& cmd) {
    std::string s;
    for (const auto& it : cmd) {
        if (!s.empty())
            s += " ";
        s += it;
    }
    return s;
}

void CommandHandler::substituteVariables(std::string& cmd, const std::vector<VInfo_ptr>& info) {
    if (info.size() > 0) {
        VNode* n = info[0]->node();
        if (!n || n->isAttribute())
            return;

        // this can lock the mutex on the defs
        n->substituteVariableValue(cmd);
    }
}

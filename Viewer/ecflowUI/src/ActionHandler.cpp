/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ActionHandler.hpp"

#include <algorithm>

#include <QAction>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    #include <QGuiApplication>
#else
    #include <QApplication>
    #include <QTextDocument>
#endif

#include "AddLabelDialog.hpp"
#include "CommandHandler.hpp"
#include "CustomCommandDialog.hpp"
#include "MenuHandler.hpp"
#include "ServerHandler.hpp"
#include "TextFormat.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VNodeMover.hpp"
#include "VNodeStateDiag.hpp"
#include "VReportMaker.hpp"
#include "ecflow/core/Str.hpp"

#define _UI_ACTIONHANDLER_DEBUG

ActionHandler::ActionHandler(QObject* actionSender, QWidget* menuParent)
    : QObject(actionSender),
      actionSender_(actionSender),
      menuParent_(menuParent) {
    connect(this, SIGNAL(viewCommand(VInfo_ptr, QString)), actionSender_, SLOT(slotViewCommand(VInfo_ptr, QString)));

    connect(this,
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)),
            actionSender_,
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)));

    connect(this,
            SIGNAL(dashboardCommand(VInfo_ptr, QString)),
            actionSender_,
            SIGNAL(dashboardCommand(VInfo_ptr, QString)));

    MenuHandler::setupShortcut(actionSender_, menuParent_, menuParent_->property("view").toString().toStdString());
}

void ActionHandler::contextMenu(const std::vector<VInfo_ptr>& nodeLst, QPoint pos) {
    std::vector<VInfo_ptr> filteredNodes;
    filterNodes(nodeLst, filteredNodes);
    std::string view = menuParent_->property("view").toString().toStdString();
    if (MenuItem* item = MenuHandler::invokeMenu("Node", filteredNodes, pos, menuParent_, view)) {
        handleCommand(item, filteredNodes);
    }
}

void ActionHandler::runCommand(const std::vector<VInfo_ptr>& nodeLst, int commandId) {
    if (MenuItem* item = MenuHandler::findItemById(commandId)) {
        std::vector<VInfo_ptr> filteredNodes;
        filterNodes(nodeLst, filteredNodes);
        if (item->isValidFor(filteredNodes, allowShortcutsForHiddenItems_)) {
            handleCommand(item, filteredNodes);
        }
    }
}

void ActionHandler::filterNodes(const std::vector<VInfo_ptr>& nodesLst, std::vector<VInfo_ptr>& filteredNodes) const {
    // deal with tricky cases - if the user selects a combination of 'normal' nodes
    // and attribute nodes, we want to ignore the attribute nodes, so we will remove
    // them from the list here and pretend they were not selected

    // count how many attributes and non-attributes are selected
    long numAttrs = 0, numNonAttrNodes = 0;
    for (auto& itNodes : nodesLst) {
        if (itNodes->isAttribute()) {
            numAttrs++;
        }
        else {
            numNonAttrNodes++;
        }
    }

    if (numAttrs > 0 && numNonAttrNodes > 0) // just keep the non-attribute nodes
    {
        for (auto& itNodes : nodesLst) {
            if (!(itNodes->isAttribute())) {
                filteredNodes.push_back(itNodes);
            }
        }
    }
    else // keep all the nodes
    {
        filteredNodes = nodesLst;
    }
}

void ActionHandler::handleCommand(MenuItem* item, const std::vector<VInfo_ptr>& filteredNodes) {
    if (item) {
#ifdef _UI_ACTIONHANDLER_DEBUG
        UiLog().dbg() << "ActionHandler::contextMenu --> item=" + item->name();
#endif
        UI_ASSERT(filteredNodes.size() > 0, "filteredNodes is empty");

        if (item->handler() == "info_panel") {
            Q_EMIT infoPanelCommand(filteredNodes.at(0), QString::fromStdString(item->command()));
            return;
        }
        else if (item->handler() == "dashboard") {
            Q_EMIT dashboardCommand(filteredNodes.at(0), QString::fromStdString(item->command()));
            return;
        }
        else if (item->handler() == "tree" || item->handler() == "table" || item->handler() == "trigger" ||
                 item->handler() == "graph") {
            Q_EMIT viewCommand(filteredNodes.at(0), QString::fromStdString(item->command()));
            return;
        }

        /*if(action->iconText() == "Set as root")
        {
            //Q_EMIT viewCommand(filteredNodes,"set_as_root");
        }*/
        else if (item->command() == "copy") {
            QString txt;
            for (const auto& filteredNode : filteredNodes) {
                if (filteredNode) {
                    if (!txt.isEmpty()) {
                        txt += ",";
                    }
                    txt += QString::fromStdString(filteredNode->path());
                }
            }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QClipboard* cb = QGuiApplication::clipboard();
            cb->setText(txt, QClipboard::Clipboard);
            cb->setText(txt, QClipboard::Selection);
#else
            QClipboard* cb = QApplication::clipboard();
            cb->setText(txt, QClipboard::Clipboard);
            cb->setText(txt, QClipboard::Selection);
#endif
        }

        else if (item->command() == "add_label") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    AddLabelDialog labelDialog(filteredNodes[0], "");
                    labelDialog.exec();
                }
            }
        }

        else if (item->command() == "add_jira_label") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    AddLabelDialog labelDialog(filteredNodes[0], "Jira");
                    labelDialog.exec();
                }
            }
        }

        else if (item->command() == "create_jsd_ticket") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    VReportMaker::sendReport(filteredNodes[0]);
                }
            }
        }

        else if (item->command() == "open_link_in_browser") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    CommandHandler::openLinkInBrowser(filteredNodes[0]);
                }
            }
        }

        else if (item->command() == "check_ui_node_state") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    VNodeStateDiag diag(filteredNodes[0]);
                }
            }
        }

        else if (item->command() == "execute_aborted") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    VNode* n = filteredNodes[0]->node();
                    assert(n);
                    if (n->isSuite() || n->isFamily()) {
                        std::vector<VNode*> nodes;
                        n->collectAbortedTasks(nodes);
                        if (confirmCommand(item, filteredNodes, CommandHandler::executeCmd(), nodes.size())) {
                            CommandHandler::executeAborted(nodes);
                        }
                    }
                }
            }
        }

        else if (item->command() == "rerun_aborted") {
            if (filteredNodes.size() == 1) {
                if (filteredNodes[0] && filteredNodes[0]->node()) {
                    VNode* n = filteredNodes[0]->node();
                    assert(n);
                    if (n->isSuite() || n->isFamily()) {
                        std::vector<VNode*> nodes;
                        n->collectAbortedTasks(nodes);
                        if (confirmCommand(item, filteredNodes, CommandHandler::rerunCmd(), nodes.size())) {
                            CommandHandler::rerunAborted(nodes);
                        }
                    }
                }
            }
        }

        else if (item->command() == "mark_for_move") {
            if (filteredNodes.size() > 1) {
                UserMessage::message(UserMessage::ERROR, true, "Only one node can be marked for move at a time");
                return;
            }

            VNodeMover::markNodeForMove(filteredNodes[0]);
        }

        else if (item->command() == "move_marked") {
            if (filteredNodes.size() > 1) {
                UserMessage::message(UserMessage::ERROR, true, "Only one destination node should be selected");
                return;
            }

            VNodeMover::moveMarkedNode(filteredNodes[0]);
        }

        else {
            CustomCommandDialog* customCommandDialog = nullptr;

            if (item->command() == "custom") // would expect this to be 'Custom...' but it's just 'Custom'
            {
                // invoke the custom command dialogue
                customCommandDialog = new CustomCommandDialog(nullptr);
                customCommandDialog->setNodes(filteredNodes);
                if (customCommandDialog->exec() == QDialog::Accepted) {
                    // the user could have changed the node selection within the custom editor
                    customCommandDialog->selectedNodes();

                    // the dialogue contains a 'fake' menu item created from the custom command
                    item = &(customCommandDialog->menuItem());
                }
                else {
                    // user cancelled the custom command dialogue
                    delete customCommandDialog;
                    return;
                }
            }

            // bool ok=true;
            if (item->isCustom()) {
                MenuHandler::interceptCommandsThatNeedConfirmation(item);
            }

            bool ok = confirmCommand(item, filteredNodes);

            if (ok) {
                CommandHandler::run(filteredNodes, item->command());
            }

            if (customCommandDialog) {
                delete customCommandDialog;
            }
        }
    }
}

bool ActionHandler::confirmCommand(MenuItem* item,
                                   const std::vector<VInfo_ptr>& filteredNodes,
                                   const std::string& commandDescStr,
                                   std::size_t taskNum) {
    bool needQuestion = item && !item->question().empty() && item->shouldAskQuestion(filteredNodes);

    // We can control if a confrmation is needed for a command from the config dialogue
    if (needQuestion && !item->questionControl().empty()) {
        if (VProperty* prop = VConfig::instance()->find(item->questionControl())) {
            needQuestion = prop->value().toBool();
        }
    }

    if (needQuestion) {
        std::string cmdStr;
        if (!commandDescStr.empty()) {
            cmdStr = commandDescStr;
        }
        else if (!item->command().empty()) {
            cmdStr = item->command();
        }

        return ActionHandler::confirmCommand(filteredNodes, item->question(), item->warning(), cmdStr, taskNum);
    }
    return true;
}

bool ActionHandler::confirmCommand(const std::vector<VInfo_ptr>& filteredNodes,
                                   const std::string& questionIn,
                                   const std::string& warning,
                                   const std::string& commandDescStr,
                                   std::size_t taskNum) {
    std::string fullNames("<ul>");
    std::string nodeNames("<ul>");
    if (filteredNodes.size() == 1) {
        fullNames = "<b>" + filteredNodes[0]->path() + "</b>";
        nodeNames = "<b>" + filteredNodes[0]->name() + "</b>";
    }
    else {
        int numNodes       = filteredNodes.size();
        int numItemsToList = std::min(numNodes, 5);

        for (int i = 0; i < numItemsToList; i++) {
            fullNames += "<li><b>";
            fullNames += filteredNodes[i]->path();
            fullNames += "</b></li>";

            nodeNames += "<li><b>";
            nodeNames += filteredNodes[i]->name();
            nodeNames += "</b></li>";
        }
        if (numItemsToList < static_cast<int>(filteredNodes.size())) {
            std::string numExtra = QString::number(numNodes - numItemsToList).toStdString();
            fullNames += "<b>...and " + numExtra + " more </b></li>";
            nodeNames += "<b>...and " + numExtra + " more </b></li>";
        }
        fullNames += "</ul>";
        nodeNames += "</ul>";
    }

    std::string question = questionIn;

    std::string placeholder("<full_name>");
    ecf::Str::replace_all(question, placeholder, fullNames);
    placeholder = "<node_name>";
    ecf::Str::replace_all(question, placeholder, nodeNames);
    if (taskNum > 0) {
        placeholder = "<task_num>";
        ecf::Str::replace_all(question, placeholder, "<b>" + QString::number(taskNum).toStdString() + "</b>");
    }

    QString msg = QString::fromStdString(question);

    QString warningStr = QString::fromStdString(warning);
    if (!warningStr.isEmpty()) {
        if (!msg.contains("<ul>")) {
            msg += "<br><br>";
        }

        msg += "<i>warning: " + Viewer::formatText(warningStr, QColor(196, 103, 36)) + "</i><br>";
    }

    QString cmdStr = QString::fromStdString(commandDescStr);

    if (!cmdStr.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        cmdStr = cmdStr.toHtmlEscaped();
#else
        cmdStr = Qt::escape(cmdStr);
#endif
        if (!warningStr.isEmpty()) {
            msg += "<br>";
        }
        else if (!msg.contains("<ul>")) {
            msg += "<br><br>";
        }

        msg += "<i>command: " + Viewer::formatText(cmdStr, QColor(41, 78, 126)) + "</i>";
        msg += "<br>";
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Confirm command"));
    msgBox.setText(msg);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    return (msgBox.exec() == QMessageBox::Ok);
}

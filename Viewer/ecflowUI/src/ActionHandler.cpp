//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <algorithm>

#include "ActionHandler.hpp"

#include <QtGlobal>
#include <QAction>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#else
#include <QApplication>
#include <QTextDocument>
#endif

#include "AddLabelDialog.hpp"
#include "CommandHandler.hpp"
#include "VNode.hpp"
#include "Str.hpp"
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"
#include "CustomCommandDialog.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VNodeMover.hpp"
#include "VNodeStateDiag.hpp"
#include "VReportMaker.hpp"

#define _UI_ACTIONHANDLER_DEBUG

ActionHandler::ActionHandler(QObject *actionSender,QWidget* menuParent) :
    QObject(actionSender),
    actionSender_(actionSender),
    menuParent_(menuParent)
{
	connect(this,SIGNAL(viewCommand(VInfo_ptr,QString)),
            actionSender_,SLOT(slotViewCommand(VInfo_ptr,QString)));

	connect(this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            actionSender_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

	connect(this,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
            actionSender_,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

	//makeShortcut();
}

void ActionHandler::contextMenu(std::vector<VInfo_ptr> nodesLst,QPoint pos)
{
    // deal with tricky cases - if the user selects a combination of 'normal' nodes
    // and attribute nodes, we want to ignore the attribute nodes, so we will remove
    // them from the list here and pretend they were not selected

    // count how many attributes and non-attributes are selected
    long numAttrs=0, numNonAttrNodes=0;
    for (std::vector<VInfo_ptr>::iterator itNodes = nodesLst.begin(); itNodes != nodesLst.end(); ++itNodes)
    {
        if ((*itNodes)->isAttribute())
            numAttrs++;
        else
            numNonAttrNodes++;
    }

    std::vector<VInfo_ptr> filteredNodes;
    if (numAttrs > 0 && numNonAttrNodes > 0)  // just keep the non-attribute nodes
    {
        for (std::vector<VInfo_ptr>::iterator itNodes = nodesLst.begin(); itNodes != nodesLst.end(); ++itNodes)
        {
            if (!((*itNodes)->isAttribute()))
                filteredNodes.push_back(*itNodes);
        }
    }
    else  // keep all the nodes
    {
        filteredNodes = nodesLst;
    }



    std::string view=menuParent_->property("view").toString().toStdString();
    MenuItem* item=MenuHandler::invokeMenu("Node", filteredNodes, pos,  menuParent_,view);

    if(item)
    {

#ifdef _UI_ACTIONHANDLER_DEBUG
        UiLog().dbg() << "ActionHandler::contextMenu --> item=" + item->name();
#endif
    	if(item->handler() == "info_panel")
    	{
            Q_EMIT infoPanelCommand(filteredNodes.at(0),QString::fromStdString(item->command()));
    		return;
    	}
    	else if(item->handler() == "dashboard")
    	{
            Q_EMIT dashboardCommand(filteredNodes.at(0),QString::fromStdString(item->command()));
    		return;
    	}
        else if(item->handler() == "tree" || item->handler() == "table" || item->handler() == "trigger")
    	{
            Q_EMIT viewCommand(filteredNodes.at(0),QString::fromStdString(item->command()));
    		return;
    	}

    	/*if(action->iconText() == "Set as root")
        {
            //Q_EMIT viewCommand(filteredNodes,"set_as_root");
        }*/
    	else if(item->command() == "copy")
    	{
    		 QString txt;
             for(std::vector<VInfo_ptr>::const_iterator it=filteredNodes.begin(); it != filteredNodes.end(); ++it)
    		 {
    			 if(*it)
    			 {
    				 if(!txt.isEmpty())
    				    txt+=",";
    				 txt+=QString::fromStdString((*it)->path());
    			 }
    		 }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    		 QClipboard* cb=QGuiApplication::clipboard();
    		 cb->setText(txt, QClipboard::Clipboard);
    		 cb->setText(txt, QClipboard::Selection);
#else
    		 QClipboard* cb=QApplication::clipboard();
    		 cb->setText(txt, QClipboard::Clipboard);
    		 cb->setText(txt, QClipboard::Selection);
#endif
    	}

        else if(item->command() == "add_label")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    AddLabelDialog labelDialog(filteredNodes[0],"");
                    labelDialog.exec();
                }
            }
        }

        else if(item->command() == "add_jira_label")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    AddLabelDialog labelDialog(filteredNodes[0],"Jira");
                    labelDialog.exec();
                }
            }
        }

        else if(item->command() == "create_jsd_ticket")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    VReportMaker::sendReport(filteredNodes[0]);
                }
            }
        }

        else if(item->command() == "open_link_in_browser")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    CommandHandler::openLinkInBrowser(filteredNodes[0]);
                }
            }
        }

        else if(item->command() == "check_ui_node_state")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    VNodeStateDiag diag(filteredNodes[0]);
                }
            }
        }

        else if(item->command() == "execute_aborted")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    CommandHandler::executeAborted(filteredNodes[0]);
                }
            }
        }

        else if(item->command() == "rerun_aborted")
        {
            if(filteredNodes.size() == 1)
            {
                if(filteredNodes[0] && filteredNodes[0]->node())
                {
                    CommandHandler::rerunAborted(filteredNodes[0]);
                }
            }
        }

        else if(item->command() == "mark_for_move")
        {
            if(filteredNodes.size() > 1)
            {
                UserMessage::message(UserMessage::ERROR, true, "Only one node can be marked for move at a time");
                return;
            }

            VNodeMover::markNodeForMove(filteredNodes[0]);
        }

        else if(item->command() == "move_marked")
        {
            if (filteredNodes.size() > 1)
            {
                UserMessage::message(UserMessage::ERROR, true, "Only one destination node should be selected");
                return;
            }

            VNodeMover::moveMarkedNode(filteredNodes[0]);
        }

        else
        {
            CustomCommandDialog *customCommandDialog = NULL;

            if(item->command() == "custom")  // would expect this to be 'Custom...' but it's just 'Custom'
            {
                // invoke the custom command dialogue
                customCommandDialog = new CustomCommandDialog(0);
                customCommandDialog->setNodes(filteredNodes);
                if (customCommandDialog->exec() == QDialog::Accepted)
                {
                    // the user could have changed the node selection within the custom editor
                    customCommandDialog->selectedNodes();

                    // the dialogue contains a 'fake' menu item created from the custom command
                    item = &(customCommandDialog->menuItem());
                }
                else
                {
                    // user cancelled the custom command dialogue
                    delete customCommandDialog;
                    return;
                }
            }

            bool ok=true;
            if (item->isCustom())
                MenuHandler::interceptCommandsThatNeedConfirmation(item);

            bool needQuestion=item && !item->question().empty() && item->shouldAskQuestion(filteredNodes);

            //We can control if a confrmation is needed for a command from the config dialogue
            if(needQuestion && !item->questionControl().empty())
                if(VProperty* prop=VConfig::instance()->find(item->questionControl()))
                    needQuestion=prop->value().toBool();

            if(needQuestion)
        	{
                std::string fullNames("<ul>");
                std::string nodeNames("<ul>");
                if (filteredNodes.size() == 1)
                {
                    fullNames = filteredNodes[0]->path();
                    nodeNames = "<b>" + filteredNodes[0]->name() + "</b>";
                }
                else
                {                    
                    int numNodes = filteredNodes.size();
                    int numItemsToList = std::min(numNodes, 5);

                    for(int i=0; i < numItemsToList; i++)
                    {
                        fullNames += "<li><b>";
                        fullNames += filteredNodes[i]->path();
                        fullNames += "</b></li>";

                        nodeNames += "<li><b>";
                        nodeNames += filteredNodes[i]->name();
                        nodeNames += "</b></li>";                    
                    }
                    if(numItemsToList < static_cast<int>(filteredNodes.size()))
                    {                  
                        std::string numExtra = QString::number(numNodes-numItemsToList).toStdString();

                        fullNames += "<b>...and " + numExtra + " more </b></li>";
                        nodeNames += "<b>...and " + numExtra + " more </b></li>";
                    }
                    fullNames += "</ul>";
                    nodeNames += "</ul>";
                }

                std::string question(item->question());

                std::string placeholder("<full_name>");
                ecf::Str::replace_all(question, placeholder, fullNames);
                placeholder = "<node_name>";
                ecf::Str::replace_all(question, placeholder, nodeNames);

                QString msg=QString::fromStdString(question);

                QString warning=QString::fromStdString(item->warning());
                if(!warning.isEmpty())
                {
                    if(!msg.contains("<ul>"))
                        msg+="<br><br>";

                    msg+="<i>warning: " + Viewer::formatText(warning,QColor(196,103,36)) + "</i><br>";
                }

                if(!item->command().empty())
                {
                    QString cmdStr=QString::fromStdString(item->command());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                    cmdStr=cmdStr.toHtmlEscaped();
#else
                    cmdStr=Qt::escape(cmdStr);
#endif
                    if(!warning.isEmpty())
                        msg+="<br>";
                    else if(!msg.contains("<ul>"))
                        msg+="<br><br>";

                    msg+="<i>command: "  + Viewer::formatText(cmdStr,QColor(41,78,126)) + "</i>";
                    msg+="<br>";
                }

                QMessageBox msgBox;               
                msgBox.setText(msg);
                msgBox.setTextFormat(Qt::RichText);
                msgBox.setIcon(QMessageBox::Question);
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if (msgBox.exec() == QMessageBox::Cancel)
                {
                    ok=false;
                }
            }

            if(ok)
                CommandHandler::run(filteredNodes,item->command());

            if (customCommandDialog)
                   delete customCommandDialog;
        }
    }

/*
	QMenu *menu=new QMenu(parent_);

	QList<QAction*> acLst;

	QAction *ac=new QAction("Requeue",parent_);
	acLst << ac;

	ac=new QAction("Submit",parent_);
	acLst << ac;

	ac=new QAction("Set as root",parent_);
		acLst << ac;

	if(QAction* res=QMenu::exec(acLst,pos,0,parent_))
	{

		if(res->iconText() == "Set as root")
		{
			emit viewCommand(filteredNodes,"set_as_root");
		}
		else
			ServerHandler::command(filteredNodes,res->iconText().toStdString());
	}

	delete menu;
*/
}

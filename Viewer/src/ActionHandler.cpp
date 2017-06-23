//============================================================================
// Copyright 2009-2017 ECMWF.
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
#endif

#include "VNode.hpp"
#include "Str.hpp"
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"
#include "CustomCommandDialog.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"

#define _UI_ACTIONHANDLER_DEBUG

ActionHandler::ActionHandler(QWidget *view) : QObject(view), parent_(view)
{
	connect(this,SIGNAL(viewCommand(VInfo_ptr,QString)),
			parent_,SLOT(slotViewCommand(VInfo_ptr,QString)));

	connect(this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
			parent_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

	connect(this,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
			parent_,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

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



    std::string view=parent_->property("view").toString().toStdString();
    MenuItem* item=MenuHandler::invokeMenu("Node", filteredNodes, pos,  parent_,view);

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

        else if(item->command() == "mark_for_move")
        {
            if (filteredNodes.size() > 1)
            {
                UserMessage::message(UserMessage::ERROR, true, "Only one node can be marked for move at a time");
                return;
            }

            VNode::setNodeMarkedForMove(filteredNodes[0]->serverAlias(), filteredNodes[0]->relativePath());


            // suspend if not already suspended
            ServerHandler* shSource = ServerHandler::findServer(VNode::nodeMarkedForMoveServerAlias());
            assert(shSource);
            VServer* vs = shSource->vRoot();
            VNode* vnodeSource = vs->find(VNode::nodeMarkedForMoveRelPath());
            if (!vnodeSource->isSuspended())
            {
                std::string suspendCommand = "ecflow_client --suspend <full_name> ";
                shSource->command(VNode::nodeMarkedForMoveRelPath(), suspendCommand);
            }

            UserMessage::message(UserMessage::INFO, true, "Node " + VNode::nodeMarkedForMoveServerAlias() + ":/" +
                                                                    VNode::nodeMarkedForMoveRelPath() + " suspended and marked for move.");
        }

        else if(item->command() == "move_marked")
        {
            if (filteredNodes.size() > 1)
            {
                UserMessage::message(UserMessage::ERROR, true, "Only one destination node should be selected");
                return;
            }

            // if same server, then error
            // NO - ecflowview had this restriction, but it does not seem to be necessary
            //if (filteredNodes[0]->serverAlias() == aliasOfMarkedServer)
            //{
            //    UserMessage::message(UserMessage::ERROR, true, "Cannot move node to the same server");
            //    return;
            //}

            // get a ServerHandler for the server
            std::string aliasOfMarkedServer(VNode::nodeMarkedForMoveServerAlias());
            ServerHandler* shSource = ServerHandler::findServer(aliasOfMarkedServer);
            if (shSource == NULL)
            {
                UserMessage::message(UserMessage::ERROR, true, "The source server " + aliasOfMarkedServer + " must be loaded into the UI");
                return;
            }

            // can only do this if the source (marked) node is suspended
            std::string pathOfMarkedNode(VNode::nodeMarkedForMoveRelPath());
            VServer* vs = shSource->vRoot();
            assert(vs);
            VNode* vnodeSource = vs->find(pathOfMarkedNode);
            if (!vnodeSource)
            {
                UserMessage::message(UserMessage::ERROR, true, "The source node " + pathOfMarkedNode + " no longer exists on server " + aliasOfMarkedServer);
                return;
            }

            if (!vnodeSource->isSuspended())
            {
                UserMessage::message(UserMessage::ERROR, true, "Node " + VNode::nodeMarkedForMoveServerAlias() + ":/" +
                                                                         VNode::nodeMarkedForMoveRelPath() +
                                                                         " must be suspended first.");
                return;
            }

            // tell the user what we're about to do
            ServerHandler *shDest = filteredNodes[0]->server();
            bool ok = UserMessage::confirm("About to move node " +
                                           pathOfMarkedNode + " from server " +
                                           aliasOfMarkedServer + " (" + shSource->host() + ":" + shSource->port() + ") to " +
                                           filteredNodes[0]->serverAlias() + " (" + shDest->host() + ":" + shDest->port() + ") "
                                           "/" + filteredNodes[0]->relativePath() +  ". Ok?");
            // do it (?)
            if (ok)
            {
                std::string plugCommand;
                plugCommand = "ecflow_client --plug <full_name> " + shDest->host() + ":" + shDest->port() + filteredNodes[0]->relativePath();
                shSource->command(pathOfMarkedNode, plugCommand);
                shDest->reset();  // the dest server will have a big update, and we don't want to wait for the next sync to see it
                VNode::clearNodeMarkedForMove();
            }
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

            if(item && !item->question().empty() && item->shouldAskQuestion(filteredNodes))
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
                    if(numItemsToList < filteredNodes.size())
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

                QMessageBox msgBox;
                msgBox.setText(QString::fromStdString(question));
                msgBox.setTextFormat(Qt::RichText);
                msgBox.setIcon(QMessageBox::Question);
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if (msgBox.exec() == QMessageBox::Cancel)
                {
                    ok=false;
                }
            }

            if(ok)
                ServerHandler::command(filteredNodes,item->command());
                //ServerHandler::command(filteredNodes,action->iconText().toStdString(), true);

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

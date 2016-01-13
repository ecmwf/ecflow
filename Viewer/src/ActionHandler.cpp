//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <algorithm>

#include "ActionHandler.hpp"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

#include "Str.hpp"
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"
#include "CustomCommandDialog.hpp"

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

    std::string view=parent_->property("view").toString().toStdString();
    QAction *action = MenuHandler::invokeMenu("Node", nodesLst,pos,  parent_,view);

    if(action)
    {
    	MenuItem* item=MenuHandler::findItem(action);
    	
        if(!item)
            return;
        
        
    	if(item->handler() == "info_panel")
    	{
    		Q_EMIT infoPanelCommand(nodesLst.at(0),QString::fromStdString(item->command()));
    		return;
    	}
    	else if(item->handler() == "dashboard")
    	{
    		Q_EMIT dashboardCommand(nodesLst.at(0),QString::fromStdString(item->command()));
    		return;
    	}
    	else if(item->handler() == "tree")
    	{
    		Q_EMIT viewCommand(nodesLst.at(0),QString::fromStdString(item->command()));
    		return;
    	}

    	/*if(action->iconText() == "Set as root")
        {
            //Q_EMIT viewCommand(nodesLst,"set_as_root");
        }*/
        
        else if(item->command() == "custom")  // would expect this to be 'Custom...' but it's just 'Custom'
        {
            CustomCommandDialog customCommandDialog(0);
            if (customCommandDialog.exec() == QDialog::Accepted)
            {
                ServerHandler::command(nodesLst, customCommandDialog.command().toStdString());
            }
        }
        else
        {
        	bool ok=true;
        	if(item && !item->question().empty() && item->shouldAskQuestion(nodesLst))
        	{
                std::string nodeNames("<ul>");
                if (nodesLst.size() == 1)
                {
                    nodeNames = nodesLst[0]->path();
                }
                else
                {
                    const int maxItems = 5;  // list no more than this number of nodes
                    int numNodes = nodesLst.size();
                    int numItemsToList = std::min(numNodes, 5);

                    for(int i=0; i < numItemsToList; i++)
                    {
                        nodeNames += "<li><b>";
                        nodeNames += nodesLst[i]->path();
                        nodeNames += "</b></li>";
                        //if (i < nodesLst.size()-1)
                        //    nodeNames += "<br>";
                    }
                    if(numItemsToList < nodesLst.size())
                    {
                        std::string numExtra;  // to convert from int to string
                        std::ostringstream ss;
                        ss << (numNodes-numItemsToList);
                        numExtra = ss.str();

                        nodeNames += "<b>...and " + numExtra + " more </b></li>";
                    }
                    nodeNames += "</ul>";
                }

                std::string question(item->question());

			    std::string placeholder("<full_name>");
			    ecf::Str::replace_all(question, placeholder, nodeNames);
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
                ServerHandler::command(nodesLst,item->command());
                //ServerHandler::command(nodesLst,action->iconText().toStdString(), true);
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
			emit viewCommand(nodesLst,"set_as_root");
		}
		else
			ServerHandler::command(nodesLst,res->iconText().toStdString());
	}

	delete menu;
*/
}

//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


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
	connect(this,SIGNAL(viewCommand(std::vector<VInfo_ptr>,QString)),
			parent_,SLOT(slotViewCommand(std::vector<VInfo_ptr>,QString)));


	connect(this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
				parent_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

	//makeShortcut();
}

void ActionHandler::contextMenu(std::vector<VInfo_ptr> nodesLst,QPoint pos)
{
    QAction *action = MenuHandler::invokeMenu("Node", nodesLst,pos,  parent_);

    if(action)
    {
    	MenuItem* item=MenuHandler::findItem(action);
    	if(item)
    	{
    		if(item->handler() == "info_panel")
    		{
    			Q_EMIT infoPanelCommand(nodesLst.at(0),QString::fromStdString(item->command()));
    			return;
    		}
    	}

    	if(action->iconText() == "Set as root")
        {
            Q_EMIT viewCommand(nodesLst,"set_as_root");
        }
        else if(action->iconText() == "Custom")  // would expect this to be 'Custom...' but it's just 'Custom'
        {
            CustomCommandDialog customCommandDialog(0);
            if (customCommandDialog.exec() == QDialog::Accepted)
            {
                ServerHandler::command(nodesLst, customCommandDialog.command().toStdString(), false);
            }
        }
        else
        {
        	bool ok=true;
        	if(item && !item->question().empty())
        	{
                std::string nodeNames("<ul>");
                for(int i=0; i < nodesLst.size(); i++)
                {
                    nodeNames += "<li><b>";
                    nodeNames += nodesLst[i]->path();
                    nodeNames += "</b></li>";
                    //if (i < nodesLst.size()-1)
                    //    nodeNames += "<br>";
                }
                nodeNames += "</ul>";

                std::string question(item->question());

			    std::string placeholder("<full_name>");
			    ecf::Str::replace_all(question, placeholder, nodeNames);
                placeholder = "<node_name>";
			    ecf::Str::replace_all(question, placeholder, nodeNames);

                QMessageBox msgBox;
                msgBox.setText(QString::fromStdString(question));
                msgBox.setTextFormat(Qt::RichText);
                msgBox.setIcon(QMessageBox::Question);
                if (msgBox.exec())
                {
                    ok=false;
                }
            }

            if(ok)
                ServerHandler::command(nodesLst,action->iconText().toStdString(), true);
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

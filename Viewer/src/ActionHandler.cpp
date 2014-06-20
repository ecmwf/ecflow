#include "ActionHandler.hpp"

#include <QAction>
#include <QMenu>
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"

ActionHandler::ActionHandler(QWidget *view) : QObject(view), parent_(view)
{
	connect(this,SIGNAL(viewCommand(std::vector<ViewNodeInfo_ptr>,QString)),
			parent_,SLOT(slotViewCommand(std::vector<ViewNodeInfo_ptr>,QString)));

	//makeShortcut();
}

void ActionHandler::contextMenu(std::vector<ViewNodeInfo_ptr> nodesLst,QPoint pos)
{


    QAction *action = MenuHandler::invokeMenu("Node", nodesLst,pos,  parent_);

    if(action)
    {
        if(action->iconText() == "Set as root")
        {
            emit viewCommand(nodesLst,"set_as_root");
        }
        else
        {
            ServerHandler::command(nodesLst,action->iconText().toStdString());
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

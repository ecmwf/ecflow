#include "ActionHandler.hpp"

#include <QAction>
#include <QMenu>
#include "ServerHandler.hpp"

ActionHandler::ActionHandler(QWidget *view) : parent_(view)
{
	//makeShortcut();
}

void ActionHandler::contextMenu(std::vector<ServerHandler*> serverLst,std::vector<Node*> nodesLst,QPoint pos)
{
	QMenu *menu=new QMenu(parent_);

	QList<QAction*> acLst;

	QAction *ac=new QAction("Requeue",parent_);
	acLst << ac;

	ac=new QAction("Submit",parent_);
	acLst << ac;

	if(QAction* res=QMenu::exec(acLst,pos,0,parent_))
	{
		ServerHandler::command(serverLst,nodesLst,res->iconText().toStdString());
	}

	delete menu;

}

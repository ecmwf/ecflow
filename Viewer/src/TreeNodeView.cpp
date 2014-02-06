//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeView.hpp"

#include <QDebug>
#include <QScrollBar>

//#include "Defs.hpp"
//#include "ClientInvoker.hpp"
//#include "Node.hpp"

#include "ActionHandler.hpp"
#include "TreeNodeModel.hpp"


TreeNodeView::TreeNodeView(QString ,QWidget* parent) : QTreeView(parent)
{
		model_=new TreeNodeModel(this);
		setModel(model_);


		//Context menu
		setContextMenuPolicy(Qt::CustomContextMenu);

		connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                this, SLOT(slotContextMenu(const QPoint &)));

		//Selection
		connect(this,SIGNAL(clicked(const QModelIndex&)),
				this,SLOT(slotSelectItem(const QModelIndex)));

		connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
				this,SLOT(slotDoubleClickItem(const QModelIndex)));

		actionHandler_=new ActionHandler(this);

		expandAll();


}
//Collects the selected list of indexes
QModelIndexList TreeNodeView::selectedList()
{
  	QModelIndexList lst;
  	foreach(QModelIndex idx,selectedIndexes())
	  	if(idx.column() == 0)
		  	lst << idx;
	return lst;
}

void TreeNodeView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		ViewNodeInfo_ptr info=model_->nodeInfo(lst.front());
		if(!info->isEmpty())
		{
			emit selectionChanged(info);
		}
	}
}

void TreeNodeView::slotDoubleClickItem(const QModelIndex&)
{
}

void TreeNodeView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TreeNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
  	//Node actions
  	if(indexClicked.isValid() && indexClicked.column() == 0)   //indexLst[0].isValid() && indexLst[0].column() == 0)
	{
	  	qDebug() << "context menu" << indexClicked;

  		std::vector<ViewNodeInfo_ptr> nodeLst;
		for(int i=0; i < indexLst.count(); i++)
		{
			ViewNodeInfo_ptr info=model_->nodeInfo(indexLst[i]);
			if(!info->isEmpty())
				nodeLst.push_back(info);
		}

		actionHandler_->contextMenu(nodeLst,globalPos);
	}

	//Desktop actions
	else
	{
	}
}

void TreeNodeView::slotViewCommand(std::vector<ViewNodeInfo_ptr> nodeLst,QString cmd)
{

	if(nodeLst.size() == 0)
		return;

	if(cmd == "set_as_root")
	{
		qDebug() << "set as root";
		model_->setRootNode(nodeLst.at(0)->node());
		expandAll();
	}
}


/*
void TreeNodeView::printDefTree(const std::string &server, int port)
{
    ClientInvoker client(server, port);
    client.allow_new_client_old_server(1);

    std::string server_version;
    client.server_version();
    server_version = client.server_reply().get_string();
    std::cout << "ecflow server version: " << server_version << "\n";


    client.sync_local();
    defs_ptr defs = client.defs();

    const std::vector<suite_ptr> &suites = defs->suiteVec();


	size_t numSuites = suites.size();
    std::cout << "Num suites: " << numSuites << std::endl;
	for (size_t s = 0; s < numSuites; s++)
    {
        QString suiteName(suites[s]->name().c_str());
        QTreeWidgetItem *suiteItem = new QTreeWidgetItem;
        suiteItem->setText(s, suiteName);
        treeWidget_->insertTopLevelItem(s, suiteItem);
        suiteItem->setExpanded(true);

        const std::vector<node_ptr> &nodes = suites[s]->nodeVec();
        for (size_t n = 0; n < nodes.size(); n++)
        {
            printNode(nodes[n], 2, suiteItem);
        }
    }
}

void TreeNodeView::printNode(node_ptr node, int indent, QTreeWidgetItem *parent)
{
    QString spaces;
    for (size_t i = 0; i < indent; i++)
    {
        spaces += "  ";
    }

    QString description;
    if (node->isFamily())
        description += " (FAMILY)";

    if (node->isTask())
        description += " (TASK)";

    QString nodeName(node->name().c_str());

    QTreeWidgetItem *nodeItem = new QTreeWidgetItem(parent);
    nodeItem->setText(0, nodeName);
    nodeItem->setExpanded(true);

    std::vector<node_ptr> nodes;
    node->immediateChildren(nodes);
    for (size_t n = 0; n < nodes.size(); n++) // starts at 1 because it includes the current node
    {
        printNode(nodes[n], indent+2, nodeItem);
    }

}
*/




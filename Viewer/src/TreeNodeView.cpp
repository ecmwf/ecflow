//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeView.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QScrollBar>

//#include "Defs.hpp"
//#include "ClientInvoker.hpp"
//#include "Node.hpp"

#include "ActionHandler.hpp"
#include "TreeNodeModel.hpp"

TreeNodeViewDelegate::TreeNodeViewDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
	hoverPen_=QPen(QColor(201,201,201));
	hoverBrush_=QBrush(QColor(250,250,250,210));
	selectPen_=QPen(QColor(125,162,206));
	selectBrush_=QBrush(QColor(193,220,252,210));

	/*QString group("itemview_main");
	editPen_=QPen(MvQTheme::colour(group,"edit_pen"));
	editBrush_=MvQTheme::brush(group,"edit_brush");
	hoverPen_=QPen(MvQTheme::colour(group,"hover_pen"));
	hoverBrush_=MvQTheme::brush(group,"hover_brush");
	selectPen_=QPen(MvQTheme::colour(group,"select_pen"));
	selectBrush_=MvQTheme::brush(group,"select_brush");*/

}

void TreeNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const
{
	//Background
	QStyleOptionViewItemV4 vopt(option);
	initStyleOption(&vopt, index);

	const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
	const QWidget* widget = vopt.widget;

	//Save painter state
	painter->save();

	if(index.column() == 0)
	{
		QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());

		if(option.state & QStyle::State_Selected)
		{
			//QRect fillRect=option.rect.adjusted(0,1,-1,-textRect.height()-1);
			painter->fillRect(fullRect,selectBrush_);
			painter->setPen(selectPen_);
			painter->drawLine(fullRect.topLeft(),fullRect.topRight());
			painter->drawLine(fullRect.bottomLeft(),fullRect.bottomRight());
		}
		else if(option.state & QStyle::State_MouseOver)
		{
			//QRect fillRect=option.rect.adjusted(0,1,-1,-1);
			painter->fillRect(fullRect,hoverBrush_);
			painter->setPen(hoverPen_);
			painter->drawLine(fullRect.topLeft(),fullRect.topRight());
			painter->drawLine(fullRect.bottomLeft(),fullRect.bottomRight());
		}

	}

	if(index.column() == 0)
	{
		QString text=index.data(Qt::DisplayRole).toString();
		QColor bg=index.data(Qt::BackgroundRole).value<QColor>();

		QFont font;
		QFontMetrics fm(font);
		int textWidth=fm.width(text);
		int offset=2;

		//QRect fillRect=QRect(option.rect.x(),option.rect.y(),painter->device()->width(),option.rect.height());

		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
		textRect.setWidth(textWidth);

		QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
		textRect.moveLeft(textRect.x()+offset);

		if(fillRect.left() < option.rect.right())
		{
			if(fillRect.right()>=option.rect.right())
				fillRect.setRight(option.rect.right());

			painter->fillRect(fillRect,bg);
					painter->setPen(QColor(180,180,180));
					painter->drawRect(fillRect);

			if(textRect.left() < option.rect.right())
			{
				if(textRect.right()>=option.rect.right())
					textRect.setRight(option.rect.right());

				if(bg == QColor(Qt::red))
					painter->setPen(Qt::white);
				else
					painter->setPen(Qt::black);
				painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
			}
		}
	}
	else if(index.column() < 3)
	{
		QString text=index.data(Qt::DisplayRole).toString();
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
		painter->setPen(Qt::black);
		painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	}

	painter->restore();

	//else
	//	QStyledItemDelegate::paint(painter,option,index);
}

QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);
	return size+QSize(0,4);
}


TreeNodeView::TreeNodeView(QString ,ServerFilter* serverFilter,ViewFilter* viewFilter,QWidget* parent) : QTreeView(parent)
{
		model_=new TreeNodeModel(serverFilter,this);

		filterModel_=new TreeNodeFilterModel(viewFilter,this);
		filterModel_->setSourceModel(model_);
		filterModel_->setDynamicSortFilter(true);

		setModel(filterModel_);

		TreeNodeViewDelegate *delegate=new TreeNodeViewDelegate(this);
		setItemDelegate(delegate);

		//setRootIsDecorated(false);
		setAllColumnsShowFocus(true);
	    setUniformRowHeights(true);
	    setMouseTracking(true);
		setSelectionMode(QAbstractItemView::ExtendedSelection);

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

QWidget* TreeNodeView::realWidget()
{
	return this;
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
		ViewNodeInfo_ptr info=model_->nodeInfo(filterModel_->mapToSource(lst.front()));
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
			ViewNodeInfo_ptr info=model_->nodeInfo(filterModel_->mapToSource(indexLst[i]));
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


void TreeNodeView::reload()
{
	model_->reload();
	expandAll();
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




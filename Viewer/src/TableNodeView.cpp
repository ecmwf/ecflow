//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeView.hpp"

#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QScrollBar>

#include "ActionHandler.hpp"
#include "FilterWidget.hpp"
#include "NodeFilterModel.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeViewDelegate.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

TableNodeView::TableNodeView(NodeFilterModel* model,NodeFilterDef* filterDef,QWidget* parent) : QTreeView(parent), NodeViewBase(model,filterDef)
{
	setProperty("style","nodeView");

	setRootIsDecorated(false);

	setSortingEnabled(true);
	sortByColumn(0,Qt::AscendingOrder);

	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
	setMouseTracking(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	//!!!!We need to do it because:
	//The background colour between the views left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this are empty and we will fill it appropriately in our delegate.
	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,Qt::transparent);
	setPalette(pal);

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


	//Header

	//setHeader(new TableNodeHeader(this));


	//Set header ContextMenuPolicy
	header()->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(header(),SIGNAL(customContextMenuRequested(const QPoint &)),
                this, SLOT(slotHeaderContextMenu(const QPoint &)));

	//for(int i=0; i < model_->columnCount(QModelIndex())-1; i++)
	//  	resizeColumnToContents(i);

	/*connect(header(),SIGNAL(sectionMoved(int,int,int)),
                this, SLOT(slotMessageTreeColumnMoved(int,int,int)));*/


	QTreeView::setModel(model_);

    //Create delegate to the view
    TableNodeViewDelegate *delegate=new TableNodeViewDelegate(this);
    setItemDelegate(delegate);
}

void TableNodeView::setModel(NodeFilterModel *model)
{
	model_= model;

	//Set the model.
	QTreeView::setModel(model_);
}

QWidget* TableNodeView::realWidget()
{
	return this;
}

//Collects the selected list of indexes
QModelIndexList TableNodeView::selectedList()
{
  	QModelIndexList lst;
  	Q_FOREACH(QModelIndex idx,selectedIndexes())
	  	if(idx.column() == 0)
		  	lst << idx;
	return lst;
}

void TableNodeView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		VInfo_ptr info=model_->nodeInfo(lst.front());
		if(info && !info->isEmpty())
		{
			Q_EMIT selectionChanged(info);
		}
	}
}

VInfo_ptr TableNodeView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(lst.front());
	}
	return VInfo_ptr();
}

void TableNodeView::slotDoubleClickItem(const QModelIndex&)
{
}

void TableNodeView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TableNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
	//Node actions
	  	if(indexClicked.isValid() && indexClicked.column() == 0)   //indexLst[0].isValid() && indexLst[0].column() == 0)
		{
		  	qDebug() << "context menu" << indexClicked;

	  		std::vector<VInfo_ptr> nodeLst;
			for(int i=0; i < indexLst.count(); i++)
			{
				VInfo_ptr info=model_->nodeInfo(indexLst[i]);
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

void TableNodeView::slotViewCommand(std::vector<VInfo_ptr> nodeLst,QString cmd)
{

	if(nodeLst.size() == 0)
		return;

	if(cmd == "set_as_root")
	{
		qDebug() << "set as root";
		//model_->setRootNode(nodeLst.at(0)->node());
		//expandAll();
	}
}





//=========================================
// Header
//=========================================

void TableNodeView::slotHeaderContextMenu(const QPoint &position)
{
	int section=header()->logicalIndexAt(position);

	if(section< 0 || section >= header()->count())
		return;

	QList<QAction*> lst;
	QMenu *menu=new QMenu(this);
	QAction *ac;

	for(int i=0; i <header()->count(); i++)
	{
	  	QString name=header()->model()->headerData(i,Qt::Horizontal).toString();
		ac=new QAction(menu);
		ac->setText(name);
		ac->setCheckable(true);
		ac->setData(i);

		if(i==0)
		{
		  	ac->setChecked(true);
			ac->setEnabled(false);
		}
		else
		{
			ac->setChecked(!(header()->isSectionHidden(i)));
		}

		menu->addAction(ac);
	}





	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	//VParamFilterMenu stateFilterMenu(menu,filterDef_->nodeState(),VParamFilterMenu::ColourDecor);





	ac=menu->exec(header()->mapToGlobal(position));
	if(ac && ac->isEnabled() && ac->isCheckable())
	{
	  	int i=ac->data().toInt();
	  	header()->setSectionHidden(i,!ac->isChecked());
		//MvQDesktopSettings::headerVisible_[i]=ac->isChecked();
		//broadcastHeaderChange();
	}
	delete menu;
}

void TableNodeView::readSettings(VSettings* vs)
{
	std::vector<std::string> array;
	vs->get("columns",array);

	for(std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
		std::string id=*it;
		for(int i=0; i < model_->columnCount(QModelIndex()); i++)
		{
			if(model_->headerData(i,Qt::Horizontal,Qt::UserRole).toString().toStdString() == id)
			{
				header()->setSectionHidden(i,false);
				break;
			}
		}
	}
}


TableNodeHeader::TableNodeHeader(QWidget *parent) : QHeaderView(Qt::Horizontal, parent)
{
     connect(this, SIGNAL(sectionResized(int, int, int)),
    		 this, SLOT(slotSectionResized(int)));

     //connect(this, SIGNAL(sectionMoved(int, int, int)), this,
     //        SLOT(handleSectionMoved(int, int, int)));

     //setMovable(true);
}

void TableNodeHeader::showEvent(QShowEvent *e)
{
    for(int i=0;i<count();i++)
    {
    	if(i==1 && !combo_[i])
    	{

    			QComboBox *box = new QComboBox(this);
    			combo_[i] = box;
    	}


       if(combo_[i])
       {
    	   combo_[i]->setGeometry(sectionViewportPosition(i),height()/2 ,
                                sectionSize(i) - 16, height()/2);
    	   combo_[i]->show();
       }
    }

    QHeaderView::showEvent(e);
}

void TableNodeHeader::slotSectionResized(int i)
{
    for (int j=visualIndex(i);j<count();j++)
    {
        int logical = logicalIndex(j);

        if(combo_[logical])
        {
        	combo_[logical]->setGeometry(sectionViewportPosition(logical), height()/2,
                                   sectionSize(logical) - 16, height());
        }
   }
}

QSize TableNodeHeader::sizeHint() const
{
    QSize s = size();
    //s.setHeight(headerSections[0]->minimumSizeHint().height() + 35);
    s.setHeight(2*35);
    return s;
}


//Param name must be unique
/*	for(std::set<VParam*>::const_iterator it=filter_->all().begin(); it != filter_->all().end(); ++it)
	{
		addAction((*it)->label(),
				  (*it)->name());
	}

*/
/*
QMenu *menu=new QMenu(this);
	menu->setTearOffEnabled(true);

	menu->addAction(actionBreadcrumbs);
	QMenu *menuState=menu->addMenu(tr("Status"));

	menuState->setTearOffEnabled(true);

	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	stateFilterMenu_=new VParamFilterMenu(menuState,states_,VParamFilterMenu::ColourDecor);
*/

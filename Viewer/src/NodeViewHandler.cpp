/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodeViewHandler.hpp"
#include "NodeViewBase.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeFilterModel.hpp"
#include "NodePathWidget.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeView.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeView.hpp"
#include "VSettings.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>


QWidget* NodeWidget::widget()
{
	return view_->realWidget();
}

VInfo_ptr NodeWidget::currentSelection()
{
	return view_->currentSelection();
}

void NodeWidget::currentSelection(VInfo_ptr info)
{
	view_->currentSelection(info);
}

void NodeWidget::reload()
{
	model_->reload();
}

void NodeWidget::active(bool b)
{
	model_->active(b);
	view_->realWidget()->setEnabled(b);
}

bool NodeWidget::active() const
{
	return model_->active();
}

TreeNodeWidget::TreeNodeWidget(VConfig* config,QWidget* parent)
{
	QVBoxLayout* layout=new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(1,1,1,1);

	model_=new TreeNodeModel(config,parent);
	filterModel_=new NodeFilterModel(model_,parent);
	view_= new TreeNodeView(filterModel_,parent);
	bc_ = new NodePathWidget(this);

	connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
	    		bc_,SLOT(setPath(VInfo_ptr)));

	connect(bc_,SIGNAL(selected(VInfo_ptr)),
			view_->realWidget(),SLOT(slotSetCurrent(VInfo_ptr)));

	connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

	QHBoxLayout* hb=new QHBoxLayout();
	hb->addWidget(bc_);
	hb->addStretch(1);

	layout->addLayout(hb);
	layout->addWidget(view_->realWidget());

}

void TreeNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type","tree");
	vs->put("dockId",id_);
}

void TreeNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != "tree")
	{
		return;
	}
}

TableNodeWidget::TableNodeWidget(VConfig* config,QWidget * parent)
{
	QVBoxLayout* layout=new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(1,1,1,1);

	model_=new TableNodeModel(config,parent);
	filterModel_=new NodeFilterModel(model_,parent);
	view_= new TableNodeView(filterModel_,parent);
	bc_ = new NodePathWidget(this);

	connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
		    	bc_,SLOT(setPath(VInfo_ptr)));

	connect(bc_,SIGNAL(selected(VInfo_ptr)),
				view_->realWidget(),SLOT(slotSetCurrent(VInfo_ptr)));

	QHBoxLayout* hb=new QHBoxLayout();
	hb->addWidget(bc_);
	hb->addStretch(1);

	layout->addLayout(hb);
	layout->addWidget(view_->realWidget());
}

void TableNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type","table");
	vs->put("dockId",id_);
}

void TableNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != "table")
	{
		return;
	}
}

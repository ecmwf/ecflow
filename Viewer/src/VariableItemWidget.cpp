//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableItemWidget.hpp"

#include <QDebug>
#include <QVBoxLayout>

//#include "Defs.hpp"
//#include "ClientInvoker.hpp"
//#include "Node.hpp"

#include "VariableModel.hpp"

//========================================================
//
// VariableView
//
//========================================================

VariableView::VariableView(QWidget* parent) : QTreeView(parent)
{
	model_=new VariableModel(this);

	setModel(model_);

	//setRootIsDecorated(false);
	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
	setMouseTracking(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	//Context menu
	setContextMenuPolicy(Qt::CustomContextMenu);

	//Selection
	connect(this,SIGNAL(clicked(const QModelIndex&)),
			this,SLOT(slotSelectItem(const QModelIndex)));

}


void VariableView::slotSelectItem(const QModelIndex&)
{

}

void VariableView::reload(ViewNodeInfo_ptr info)
{
	model_->setData(info);
	expandAll();
}

//========================================================
//
// VariableItemWidget
//
//========================================================

VariableItemWidget::VariableItemWidget(QWidget *parent)
{
	QVBoxLayout *layout=new QVBoxLayout(this);

	view_=new VariableView(this);
	layout->addWidget(view_);
}

QWidget* VariableItemWidget::realWidget()
{
	return this;
}

void VariableItemWidget::reload(ViewNodeInfo_ptr info)
{
	view_->reload(info);
}


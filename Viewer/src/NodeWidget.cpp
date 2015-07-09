/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodeWidget.hpp"
#include "NodeViewBase.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeFilterModel.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"

#include <QWidget>

NodeWidget::NodeWidget(QWidget* parent) : DashboardWidget(parent),
   model_(0),
   filterModel_(0),
   view_(0),
   data_(0),
   icons_(0),
   atts_(0),
   filterDef_(0),
   states_(0)
{
	//Define the icon filter for the model. It controls what icons
	//are displayed next to the nodes. This is exposed via a menu.
	icons_=new IconFilter;

	//Define the attribute filter for the model. It controls what attributes
	//are displayed for a given node. This is exposed via a menu.
	atts_=new AttributeFilter;

	//This defines how to filter the nodes in the tree. We only want to filter according to node status.
	filterDef_=new NodeFilterDef(NodeFilterDef::NodeState);

	//The node status filter is exposed via a menu. So we need a reference to it.
	states_=filterDef_->nodeState();
}

NodeWidget::~NodeWidget()
{
	//We only need to delete the non-qobject members
	delete data_;
	delete icons_;
	delete atts_;
	delete filterDef_;
}

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
	active(true);
	//model_->reload();
}

void NodeWidget::rerender()
{
	view_->rerender();
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

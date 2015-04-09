//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "Node.hpp"

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
}

QWidget* TriggerItemWidget::realWidget()
{
	return this;
}

void TriggerItemWidget::reload(VInfo_ptr nodeInfo)
{
	loaded_=true;

	if(nodeInfo.get() != 0 && nodeInfo->isNode())
	{
		//Node* n=nodeInfo->node();
	}
	else
	{

	}

}

void TriggerItemWidget::clearContents()
{
	loaded_=false;
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("trigger");

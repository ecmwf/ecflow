//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "VNode.hpp"
#include "TriggerView.hpp"

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
}

QWidget* TriggerItemWidget::realWidget()
{
	return this;
}

void TriggerItemWidget::reload(VInfo_ptr nodeInfo)
{
	clearContents();

	active_=true;

    if(nodeInfo && nodeInfo->isNode())
	{        
            nodeInfo->node()->triggers();
    }

	else
	{

	}

}

void TriggerItemWidget::clearContents()
{
	InfoPanelItem::clear();
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");

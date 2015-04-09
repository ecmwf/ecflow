//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "EditItemWidget.hpp"

#include "Node.hpp"

//========================================================
//
// EditItemWidget
//
//========================================================

EditItemWidget::EditItemWidget(QWidget *parent) : QWidget(parent)
{
}

QWidget* EditItemWidget::realWidget()
{
	return this;
}

void EditItemWidget::reload(VInfo_ptr nodeInfo)
{
	loaded_=true;
}

void EditItemWidget::clearContents()
{
}


static InfoPanelItemMaker<EditItemWidget> maker1("edit");

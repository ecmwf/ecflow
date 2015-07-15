//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LineEdit.hpp"

#include "TimeItemWidget.hpp"

#include "Node.hpp"

//========================================================
//
// TimeItemWidget
//
//========================================================

TimeItemWidget::TimeItemWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  
}

QWidget* TimeItemWidget::realWidget()
{
	return this;
}

void TimeItemWidget::reload(VInfo_ptr nodeInfo)
{
	loaded_=true;
}

void TimeItemWidget::clearContents()
{
	InfoPanelItem::clear();
}


static InfoPanelItemMaker<TimeItemWidget> maker1("time");

//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef MESSAGEITEMWIDGET_HPP_
#define MESSAGEITEMWIDGET_HPP_

#include "InfoPanelItem.hpp"
#include "TextItemWidget.hpp"
#include "ViewNodeInfo.hpp"

class MessageItemWidget : public TextItemWidget, public InfoPanelItem
{
public:
	MessageItemWidget(QWidget *parent=0);

	void reload(ViewNodeInfo_ptr);
	QWidget* realWidget();
	void clearContents();
};

#endif


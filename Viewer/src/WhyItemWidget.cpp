//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WhyItemWidget.hpp"

#include "Node.hpp"
#include "VNode.hpp"

//========================================================
//
// WhyItemWidget
//
//========================================================

WhyItemWidget::WhyItemWidget(QWidget *parent) : TextItemWidget(parent)
{
}

QWidget* WhyItemWidget::realWidget()
{
	return this;
}

void WhyItemWidget::reload(VInfo_ptr info)
{
	loaded_=true;
	info_=info;

	if(info_ && info_.get() && info_->isNode())
	{
		textEdit_->setPlainText(why(info_->node()));
	}
	else
	{
		textEdit_->clear();
	}
}

void WhyItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

QString WhyItemWidget::why(VNode* node) const
{
	QString s;

	if(node && node->node())
	{
		Node *n=node->node();
		std::vector<std::string> theReasonWhy;
		n->bottom_up_why(theReasonWhy);
		for (std::vector<std::string>::const_iterator it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
			s+=QString::fromStdString(*it) + "\n";
	}

    return s;
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");

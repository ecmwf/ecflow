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

#include "VConfig.hpp"
#include "VNode.hpp"


//========================================================
//
// WhyItemWidget
//
//========================================================

WhyItemWidget::WhyItemWidget(QWidget *parent) : CodeItemWidget(parent)
{
	messageLabel_->hide();
	fileLabel_->hide();
	textEdit_->setShowLineNumbers(false);

	//Editor font
	textEdit_->setFontProperty(VConfig::instance()->find("panel.why.font"));

}

WhyItemWidget::~WhyItemWidget()
{
}


QWidget* WhyItemWidget::realWidget()
{
	return this;
}

void WhyItemWidget::reload(VInfo_ptr info)
{
    assert(enabled_);

    if(suspended_)
        return;

    clearContents();
	info_=info;

    if(info_)
	{
		textEdit_->setPlainText(why());
	}
}

void WhyItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
}

QString WhyItemWidget::why() const
{
	QString s;

	std::vector<std::string> theReasonWhy;

	if(info_ && info_.get())
	{
		if(info_->isServer())
		{
			info_->node()->why(theReasonWhy);
		}
		else if(info_->isNode() && info_->node())
		{
			info_->node()->why(theReasonWhy);
		}
	}

	for (std::vector<std::string>::const_iterator it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
		s+=QString::fromStdString(*it) + "\n";

    return s;
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");


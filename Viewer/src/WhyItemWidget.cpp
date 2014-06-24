//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WhyItemWidget.hpp"

#include "Defs.hpp"
#include "DState.hpp"
#include "Node.hpp"
#include "ServerHandler.hpp"
#include "Suite.hpp"
#include "Variable.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

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

void WhyItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		QString txt;
		textEdit_->setPlainText(why(n));
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

QString WhyItemWidget::why(Node* n) const
{
	QString s;

	if(n)
	{
		std::vector<std::string> theReasonWhy;
		n->bottom_up_why(theReasonWhy);
		for (std::vector<std::string>::const_iterator it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
			s+=QString::fromStdString(*it) + "\n";
	}

    return s;
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");

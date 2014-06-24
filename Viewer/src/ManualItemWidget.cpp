//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ManualItemWidget.hpp"

#include "Node.hpp"
#include "ServerHandler.hpp"

//========================================================
//
// ManualItemWidget
//
//========================================================

ManualItemWidget::ManualItemWidget(QWidget *parent) :TextItemWidget(parent)
{
}

QWidget* ManualItemWidget::realWidget()
{
	return this;
}

void ManualItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		QString txt;
		std::string fName,msg,err;
		if(ServerHandler* s=nodeInfo->server())
		{
			if(s->readManual(n,fName,msg,err))
				textEdit_->setPlainText(QString::fromStdString(msg));
			else
				textEdit_->setPlainText(QString::fromStdString(err));
		}
	}
	else
	{
		textEdit_->clear();
	}
}

void ManualItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

static InfoPanelItemMaker<ManualItemWidget> maker1("manual");

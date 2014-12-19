//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MessageItemWidget.hpp"



//========================================================
//
// MessageItemWidget
//
//========================================================

MessageItemWidget::MessageItemWidget(QWidget *parent) : TextItemWidget(parent)
{
}

QWidget* MessageItemWidget::realWidget()
{
	return this;
}

void MessageItemWidget::reload(VInfo_ptr nodeInfo)
{
	loaded_=true;
	/*if(nodeInfo.get() != 0 && nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();
		if(ServerHandler* s=nodeInfo->server())
		{
			NodeInfoQuery_ptr query(new NodeInfoQuery(n,NodeInfoQuery::MESSAGE,this));
			s->query(query);
		}
	}
	else
	{
		textEdit_->clear();
	}*/

}

void MessageItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

void MessageItemWidget::infoReady(VReply* reply)
{
	/*if(reply && reply->sender() == this)
	{
		if(reply->done())
		{
			textEdit_->setPlainText(QString::fromStdString(reply->text()));
		}
		else
		{
			textEdit_->setPlainText(QString::fromStdString(reply->errorText()));
		}
	}*/
}

static InfoPanelItemMaker<MessageItemWidget> maker1("message");


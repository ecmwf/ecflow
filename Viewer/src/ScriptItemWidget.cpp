//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ScriptItemWidget.hpp"

#include "Node.hpp"
#include "ServerHandler.hpp"

//========================================================
//
// ScriptItemWidget
//
//========================================================

ScriptItemWidget::ScriptItemWidget(QWidget *parent) : TextItemWidget(parent)
{
}

QWidget* ScriptItemWidget::realWidget()
{
	return this;
}

void ScriptItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	
	if(nodeInfo.get() != 0 && nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		if(nodeInfo->isNode())
		{
			Node* n=nodeInfo->node();
			if(ServerHandler* s=nodeInfo->server())
			{
				NodeInfoQuery_ptr query(new NodeInfoQuery(n,NodeInfoQuery::SCRIPT,this));
				s->query(query);
			}
		}	
	}
	else
	{
		textEdit_->clear();
	}
}

void ScriptItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

void ScriptItemWidget::queryFinished(NodeInfoQuery_ptr reply)
{
	if(reply && reply->sender() == this)
	{
		if(reply->done())
		{
			textEdit_->setPlainText(QString::fromStdString(reply->text()));
		}
		else
		{
			textEdit_->setPlainText(QString::fromStdString(reply->errorText()));
		}
	}
}


static InfoPanelItemMaker<ScriptItemWidget> maker1("script");
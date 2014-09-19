//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "JobItemWidget.hpp"

#include "Node.hpp"
#include "ServerHandler.hpp"


//========================================================
//
// JobItemWidget
//
//========================================================

JobItemWidget::JobItemWidget(QWidget *parent) : TextItemWidget(parent)
{
}

QWidget* JobItemWidget::realWidget()
{
	return this;
}

void JobItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;

	if(nodeInfo.get() != 0 && nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();
		if(ServerHandler* s=nodeInfo->server())
		{
			NodeInfoQuery_ptr query(new NodeInfoQuery(n,NodeInfoQuery::JOB,this));
			s->query(query);
		}
	}
	else
	{
		textEdit_->clear();
	}
}

void JobItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}


void JobItemWidget::queryFinished(NodeInfoQuery_ptr reply)
{
	std::cout << "reply for job" << std::endl;

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

static InfoPanelItemMaker<JobItemWidget> maker1("job");

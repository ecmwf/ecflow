//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "MessageItemWidget.hpp"

#include "Defs.hpp"
#include "DState.hpp"
#include "Node.hpp"
#include "ServerHandler.hpp"
#include "Suite.hpp"
#include "Variable.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

//========================================================
//
// MessageItemWidget
//
//========================================================

MessageItemWidget::MessageItemWidget(QWidget *parent) : QPlainTextEdit(parent)
{
}

QWidget* MessageItemWidget::realWidget()
{
	return this;
}

void MessageItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		QString txt;
		if(ServerHandler* s=nodeInfo->server())
		{
			const std::vector<std::string>& msg=s->messages(n);
			for(std::vector<std::string>::const_iterator it=msg.begin(); it != msg.end(); it++)
			{
				txt+=QString::fromStdString(*it) + "\n";
			}
		}

		setPlainText(txt);
	}
	else
	{
		clear();
	}

}

void MessageItemWidget::clearContents()
{
	loaded_=false;
	clear();
}
static InfoPanelItemMaker<MessageItemWidget> maker1("message");


//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoItemWidget.hpp"

#include "Defs.hpp"
#include "DState.hpp"
#include "Node.hpp"
#include "Suite.hpp"
#include "Variable.hpp"

#include "Highlighter.hpp"
#include "InfoProvider.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

//========================================================
//
// InfoItemWidget
//
//========================================================

InfoItemWidget::InfoItemWidget(QWidget *parent) : TextItemWidget(parent)
{
	QFont f;
	//f.setFamily("Monospace");
	//f.setFamily("Courier");
	f.setFixedPitch(true);
	textEdit_->setFont(f);

	Highlighter* ih=new Highlighter(textEdit_->document(),"info");
}

QWidget* InfoItemWidget::realWidget()
{
	return this;
}

void InfoItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;

	if(nodeInfo.get() != 0 && nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();
		ServerHandler *server=nodeInfo->server();

		std::stringstream ss;
		InfoProvider::info(n,ss);

		//textEdit_->clear();
		QString s=QString::fromStdString(ss.str());
		//textEdit_->appendHtml(s);
		textEdit_->setPlainText(s);

	 //std::stringstream ss;
	 //n.info(ss);
	}
	else
	{
		textEdit_->clear();
	}

}

void InfoItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}


static InfoPanelItemMaker<InfoItemWidget> maker1("info");

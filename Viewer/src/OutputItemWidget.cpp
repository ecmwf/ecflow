//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputItemWidget.hpp"

#include "Node.hpp"
#include "ServerHandler.hpp"

//========================================================
//
// OutputItemWidget
//
//========================================================

OutputItemWidget::OutputItemWidget(QWidget *parent) :TextItemWidget(parent)
{
}

QWidget* OutputItemWidget::realWidget()
{
	return this;
}

void OutputItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		QString txt;
		std::string fName,msg,err;
		if(ServerHandler* s=nodeInfo->server())
		{
			if(s->readFile(n,"ECF_JOBOUT",fName,msg,err))
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

void OutputItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

static InfoPanelItemMaker<OutputItemWidget> maker1("output");

/*
std::string filename(Node *n)
{
	std::string jobout;
	n->findGenVariableValue("ECF_JOBOUT",jobout);


	//output variable may contain micro

	if(file_) free(file_);
	file_ = strdup(jobout.c_str());
	load(n);
	XmListDeleteAllItems(list_);

	output_lister ol(list_);
	n.serv().dir(n,file_,ol);

	std::string remote = n.variable("ECF_OUT");
	std::string job    = n.variable("ECF_JOB");

	if (!remote.empty() && !job.empty()) {
   // display both remote and local dir
	if (remote == job)
	{
		output_lister rem(list_);
		n.serv().dir(n,job.c_str(),rem);
	}
 }
 new search_me(*this);

*/


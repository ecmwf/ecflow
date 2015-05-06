//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ZombieItemWidget.hpp"

#include "ServerHandler.hpp"
//#include "SuiteFilter.hpp"
//#include "SuiteModel.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// ZombieItemWidget
//
//========================================================

ZombieItemWidget::ZombieItemWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	//infoProvider_=new EditProvider(this);

	//connect(submitTb_,SIGNAL(clicked(bool)),
	//		this,SLOT(on_submitTb__clicked(bool)));

	//model_=new SuiteModel(this);

	//suiteView->setModel(model_);

}

QWidget* ZombieItemWidget::realWidget()
{
	return this;
}

void ZombieItemWidget::reload(VInfo_ptr info)
{
	loaded_=true;
	info_=info;

	/*if(info_.get() && info_->isServer() && info_->server())
	{
		model_->setData(info_->server()->suiteFilter());
	}
	else
	{
		clearContents();
	}*/
}

void ZombieItemWidget::clearContents()
{
	//model_->setData(0);
}

void ZombieItemWidget::on_autoTb_toggled(bool)
{

}

void ZombieItemWidget::on_enableTb_toggled(bool val)
{
	/*if(SuiteFilter* sf=model_->filter())
	{
		sf->setEnabled(val);
	}*/
}

void ZombieItemWidget::on_okTb_clicked(bool)
{
	/*if(info_.get() && info_->isServer() && info_->server())
	{
		info_->server()->updateSuiteFilter(model_->filter());
	}*/
}

static InfoPanelItemMaker<ZombieItemWidget> maker1("zombie");

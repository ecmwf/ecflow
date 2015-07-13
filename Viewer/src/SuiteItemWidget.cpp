//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteItemWidget.hpp"

#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "SuiteModel.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

//========================================================
//
// SuiteItemWidget
//
//========================================================

SuiteItemWidget::SuiteItemWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	//infoProvider_=new EditProvider(this);

	//connect(submitTb_,SIGNAL(clicked(bool)),
	//		this,SLOT(on_submitTb__clicked(bool)));

	model_=new SuiteModel(this);

	suiteView->setModel(model_);

	updateWidgetState();

}

QWidget* SuiteItemWidget::realWidget()
{
	return this;
}

void SuiteItemWidget::reload(VInfo_ptr info)
{
	loaded_=true;
	info_=info;

	if(info_.get() && info_->isServer() && info_->server())
	{
		if(SuiteFilter *sf=info_->server()->suiteFilter())
		{
			model_->setData(sf);
			enableTb->setChecked(sf->isEnabled());
			autoTb->setChecked(sf->autoAddNewSuites());
			updateWidgetState();
		}
		else
		{
			model_->setData(0);
		}
	}
	else
	{
		clearContents();
	}
}

void SuiteItemWidget::clearContents()
{
	model_->setData(0);
}

void SuiteItemWidget::updateWidgetState()
{
	if(enableTb->isChecked())
	{
		autoTb->setEnabled(true);
		selectAllTb->setEnabled(true);
		unselectAllTb->setEnabled(true);

		if(SuiteFilter* sf=model_->filter())
		{
			autoTb->setChecked(sf->autoAddNewSuites());
		}
	}
	else
	{
		autoTb->setEnabled(false);
		selectAllTb->setEnabled(false);
		unselectAllTb->setEnabled(false);
	}
}

void SuiteItemWidget::on_autoTb_clicked(bool val)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->setAutoAddNewSuites(val);
	}
}

void SuiteItemWidget::on_enableTb_clicked(bool val)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->setEnabled(val);
		model_->reloadData();
	}

	updateWidgetState();
}

void SuiteItemWidget::on_selectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->selectAll();
		model_->reloadData();
	}
}

void SuiteItemWidget::on_unselectAllTb_clicked(bool)
{
	if(SuiteFilter* sf=model_->filter())
	{
		sf->unselectAll();
		model_->reloadData();
	}
}

void SuiteItemWidget::on_okTb_clicked(bool)
{
	if(info_.get() && info_->isServer() && info_->server())
	{
		info_->server()->updateSuiteFilter(model_->filter());
	}
}

void SuiteItemWidget::suiteFilterChanged()
{
	model_->reloadData();
}

static InfoPanelItemMaker<SuiteItemWidget> maker1("suite");

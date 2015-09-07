//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyDialog.hpp"

#include "ChangeNotifyModel.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <QDebug>

ChangeNotifyDialog::ChangeNotifyDialog(QWidget *parent) :
	QDialog(parent),
	model_(0)
{
	setupUi(this);

	model_=new ChangeNotifyModel(this);
	treeView_->setModel(model_);
}

void ChangeNotifyDialog::init(VProperty* prop, ChangeNotifyModel* model)
{
	model_=model;
	treeView_->setModel(model_);

	if(prop)
	{
		QString sh="QLabel{ \
		   background: " + prop->paramToColour("background").name() + "; \
		   color: " + prop->paramToColour("foreground").name() + "; \
	       border: 1px solid " +  prop->paramToColour("border").name() + "; \
	       padding: 4 px; }";

		label_->setStyleSheet(sh);
		label_->setText(prop->param("labelText"));
		setWindowTitle(prop->param("title"));
	}
}


void ChangeNotifyDialog::on_closePb__clicked(bool b)
{
	hide();
}

void ChangeNotifyDialog::on_clearCPb__clicked(bool b)
{
	hide();
	model_->data()->clear();
}

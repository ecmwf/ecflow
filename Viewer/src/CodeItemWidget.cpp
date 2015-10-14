//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CodeItemWidget.hpp"

CodeItemWidget::CodeItemWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	externalTb_->hide();

	fileLabel_->setProperty("fileInfo","1");

	searchLine_->setEditor(textEdit_);

	searchLine_->setVisible(false);
}


void CodeItemWidget::removeSpacer()
{
	//Remove the first spcer item!!
	for(int i=0; horizontalLayout->count(); i++)
	{
		if(QSpacerItem* sp=horizontalLayout->itemAt(i)->spacerItem())
		{
			horizontalLayout->takeAt(i);
			delete sp;
			break;
		}
	}
}


void CodeItemWidget::on_searchTb_toggled(bool b)
{
	searchLine_->setVisible(b);
	if(b)
	{
		searchLine_->setFocus();
	}
}

void CodeItemWidget::on_gotoLineTb__clicked()
{
	textEdit_->gotoLine();
}

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerGraphWidget.hpp"


TriggerGraphWidget::TriggerGraphWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	model_ = new TriggerGraphModel(this);
	triggerItemsTreeView_->setModel(model_);
}

TriggerGraphWidget::~TriggerGraphWidget()
{
}

void TriggerGraphWidget::setTriggerExpression(std::string &exp)
{
	selectedNodeTextEdit_->setPlainText(QString::fromStdString(exp));
}

void TriggerGraphWidget::clear()
{
	model_->clearData();
}

void TriggerGraphWidget::beginUpdate()
{
	model_->beginUpdate();
}

void TriggerGraphWidget::endUpdate()
{
	model_->endUpdate();

}

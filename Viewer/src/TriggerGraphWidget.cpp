//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerGraphWidget.hpp"

#include "Highlighter.hpp"
#include "TriggerGraphModel.hpp"
#include "TriggerViewDelegate.hpp"

TriggerGraphWidget::TriggerGraphWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

    //Highlighter* ih=new Highlighter(triggerTextEdit_->document(),"trigger");
    //triggerTextEdit_->setReadOnly(true);
    //triggerTextEdit_->setBackgroundVisible(true);

    triggerLabel_->setProperty("trigger","1");
    triggeredLabel_->setProperty("trigger","1");

    triggerModel_ = new TriggerGraphModel(this);
    triggerView_->setModel(triggerModel_);

    triggerView_->setHeaderHidden(true);
    triggerView_->setRootIsDecorated(false);
    triggerView_->setSortingEnabled(false);
    triggerView_->setAllColumnsShowFocus(true);
    triggerView_->setUniformRowHeights(true);
    triggerView_->setMouseTracking(true);
    triggerView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    triggerView_->setItemDelegate(new TriggerViewDelegate(triggerView_));

    triggeredModel_ = new TriggerGraphModel(this);
    triggeredView_->setModel(triggeredModel_);
}

TriggerGraphWidget::~TriggerGraphWidget()
{
}

void TriggerGraphWidget::setTriggerExpression(std::string &exp)
{
    //triggerTextEdit_->setPlainText(QString::fromStdString(exp));
}

void TriggerGraphWidget::clear()
{
    triggerModel_->clearData();
}

void TriggerGraphWidget::beginTriggerUpdate()
{
    triggerModel_->beginUpdate();
}

void TriggerGraphWidget::endTriggerUpdate()
{
    triggerModel_->endUpdate();

}

void TriggerGraphWidget::setTriggerCollector(TriggerListCollector *tc)
{
    triggerModel_->setTriggerCollector(tc);
}


//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphWidget.hpp"

#include "ui_TriggerGraphWidget.h"

#include <algorithm>

#include "TriggerGraphView.hpp"
#include "TriggerItemWidget.hpp"
#include "UiLog.hpp"
#include "VSettings.hpp"


TriggerGraphWidget::TriggerGraphWidget(QWidget* parent) :
    QWidget(parent),
    ui_(new Ui::TriggerGraphWidget)
{
    ui_->setupUi(this);

    ui_->zoomSlider->setRange(ui_->view->minZoomLevel(),
                               ui_->view->maxZoomLevel());
    ui_->zoomSlider->setValue(ui_->view->defaultZoomLevel());

    adjustButtons();

    //relay commands
    connect(ui_->view,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

    connect(ui_->view,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
            this,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

    connect(ui_->view,SIGNAL(linkSelected(VInfo_ptr)),
            this,SIGNAL(linkSelected(VInfo_ptr)));

    connect(ui_->zoomSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setZoomLevel(int)));

    connect(ui_->zoomResetTb, SIGNAL(clicked()),
            this, SLOT(resetZoomLevel()));

    ui_->legendLabel->setProperty("legend", "1");
    updateLegend();
}

TriggerGraphWidget::~TriggerGraphWidget()
{
    clear();
}

void TriggerGraphWidget::clear()
{
    info_.reset();
    ui_->view->clear();
}

void TriggerGraphWidget::setInfo(VInfo_ptr info, bool dependency)
{
    info_=info;
    scan(dependency);
}

void TriggerGraphWidget::adjust(VInfo_ptr info, bool dependency, TriggerTableCollector* tc1, TriggerTableCollector* tc2)
{
    if (!info) {
        clear();
    } else if(info_ != info) {
        setInfo(info, dependency);
        //scan();
//        beginTriggerUpdate();
//        setTriggerCollector(tc1,tc2);
//        endTriggerUpdate();
    }
}

void TriggerGraphWidget::setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2)
{
//    model_->setTriggerCollectors(tc1, tc2);
//    triggerTc_ = tc1;
//    triggeredTc_ = tc2;
}


void TriggerGraphWidget::beginTriggerUpdate()
{
    //model_->beginUpdate();
}

void TriggerGraphWidget::endTriggerUpdate()
{
}

void TriggerGraphWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
    ui_->view->nodeChanged(node, aspect);
}

void TriggerGraphWidget::scan(bool dependency)
{
    if (!info_ || !info_->node())
        return;

    VNode *node = info_->node();
    Q_ASSERT(node);
    ui_->view->show(node, dependency);

//    UiLog().dbg() << model_->rowCount() << layout_->nodes_.size();

//    if(VNode *p = node->parent()) {
//        layout_->addRelation(p, node, nullptr, TriggerCollector::Hierarchy, nullptr);
//        scan(p);
//    }

}

void TriggerGraphWidget::setTriggeredScanner(TriggeredScanner* scanner)
{
    ui_->view->setTriggeredScanner(scanner);
}

bool TriggerGraphWidget::dependency() const
{
    return ui_->view->dependency();
}

void TriggerGraphWidget::updateLegend()
{
    QPixmap pix = ui_->view->makeLegendPixmap();
    ui_->legendLabel->setPixmap(pix);
}

void TriggerGraphWidget::setZoomLevel(int v)
{
    ui_->view->setZoomLevel(v);
    adjustButtons();
}

void TriggerGraphWidget::resetZoomLevel()
{
    ui_->zoomSlider->setValue(ui_->view->defaultZoomLevel());
}

void TriggerGraphWidget::adjustButtons()
{
    ui_->zoomResetTb->setEnabled(
                ui_->view->zoomLevel() != ui_->view->defaultZoomLevel());
}

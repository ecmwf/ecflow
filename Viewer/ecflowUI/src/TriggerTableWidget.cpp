/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TriggerTableWidget.hpp"

#include "Highlighter.hpp"
#include "TriggerItemWidget.hpp"
#include "TriggerTableModel.hpp"
#include "TriggerViewDelegate.hpp"
#include "TriggeredScanner.hpp"
#include "VSettings.hpp"

TriggerTableWidget::TriggerTableWidget(QWidget* parent) : QWidget(parent) {
    setupUi(this);

    nodeCollector_      = new TriggerTableCollector(false);
    triggerCollector_   = new TriggerTableCollector(false);
    triggeredCollector_ = new TriggerTableCollector(false);

    depInfoCloseTb_->setProperty("triggertitle", "1");
    depInfoCloseTb_->parent()->setProperty("triggertitle", "1");

    // Format labels
    triggerLabel_->setProperty("triggertitle", "1");
    triggeredLabel_->setProperty("triggertitle", "1");
    depLabel_->setProperty("triggertitle", "1");

    depLabelText_ = tr(" Dependency details");
    depLabel_->setText(depLabelText_);

    // Node - model + view
    nodeModel_ = new TriggerTableModel(TriggerTableModel::NodeMode, this);
    nodeView_->setTableModel(nodeModel_);

    // Set the height of the node display area
    nodeView_->enableOneRowMode();

    // Set the size of the left and right arrow labels
    leftArrowLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    leftArrowLabel_->setFixedHeight(nodeView_->height());
    leftArrowLabel_->setFixedWidth(nodeView_->height());

    rightArrowLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rightArrowLabel_->setFixedHeight(nodeView_->height());
    rightArrowLabel_->setFixedWidth(nodeView_->height());

    // Trigger - model + view
    triggerModel_ = new TriggerTableModel(TriggerTableModel::TriggerMode, this);
    triggerView_->setTableModel(triggerModel_);

    // triggered - model + view
    triggeredModel_ = new TriggerTableModel(TriggerTableModel::TriggeredMode, this);
    triggeredView_->setTableModel(triggeredModel_);

    // normal selection
    connect(
        triggerView_, SIGNAL(selectionChanged(TriggerTableItem*)), this, SLOT(slotTriggerSelection(TriggerTableItem*)));

    connect(triggerView_, SIGNAL(clicked(TriggerTableItem*)), this, SLOT(slotTriggerClicked(TriggerTableItem*)));

    connect(triggeredView_,
            SIGNAL(selectionChanged(TriggerTableItem*)),
            this,
            SLOT(slotTriggeredSelection(TriggerTableItem*)));

    connect(triggeredView_, SIGNAL(clicked(TriggerTableItem*)), this, SLOT(slotTriggeredClicked(TriggerTableItem*)));

    // lookup selection
    connect(triggerView_, SIGNAL(linkSelected(VInfo_ptr)), this, SIGNAL(linkSelected(VInfo_ptr)));

    connect(triggeredView_, SIGNAL(linkSelected(VInfo_ptr)), this, SIGNAL(linkSelected(VInfo_ptr)));

    // relay commands
    connect(
        nodeView_, SIGNAL(infoPanelCommand(VInfo_ptr, QString)), this, SIGNAL(infoPanelCommand(VInfo_ptr, QString)));

    connect(
        nodeView_, SIGNAL(dashboardCommand(VInfo_ptr, QString)), this, SIGNAL(dashboardCommand(VInfo_ptr, QString)));

    connect(
        triggerView_, SIGNAL(infoPanelCommand(VInfo_ptr, QString)), this, SIGNAL(infoPanelCommand(VInfo_ptr, QString)));

    connect(
        triggerView_, SIGNAL(dashboardCommand(VInfo_ptr, QString)), this, SIGNAL(dashboardCommand(VInfo_ptr, QString)));

    connect(triggeredView_,
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)),
            this,
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)));

    connect(triggeredView_,
            SIGNAL(dashboardCommand(VInfo_ptr, QString)),
            this,
            SIGNAL(dashboardCommand(VInfo_ptr, QString)));

    // anchor clicked in text browser
    connect(depBrowser_, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(anchorClicked(const QUrl&)));
}

TriggerTableWidget::~TriggerTableWidget() {
    delete nodeCollector_;
    delete triggerCollector_;
    delete triggeredCollector_;
}

void TriggerTableWidget::clear() {
    info_.reset();

    nodeModel_->clearData();
    triggerModel_->clearData();
    triggeredModel_->clearData();

    depLabel_->setText(depLabelText_);
    depBrowser_->clear();

    // At this point the tables are cleared so it is safe to clear the collectors
    triggerCollector_->clear();
    triggeredCollector_->clear();
}

void TriggerTableWidget::clearSelection() {
    lastSelectedItem_.reset();
}

void TriggerTableWidget::setInfo(VInfo_ptr info, bool dependency) {
    clear();

    info_       = info;
    dependency_ = dependency;

    if (!info_ || !info_->isNode() || !info_->node()) {
        return;
    }

    VNode* n = info_->node();
    Q_ASSERT(n);

    nodeModel_->beginUpdate();
    nodeCollector_->clear();
    if (info_) {
        nodeCollector_->add(info_->item(), nullptr, TriggerCollector::Normal);
    }
    nodeModel_->setTriggerCollector(nodeCollector_);
    nodeModel_->endUpdate();

    beginTriggerUpdate();

    // collect the list of triggers of this node
    triggerCollector_->setDependency(dependency_);
    n->triggers(triggerCollector_);

    triggeredCollector_->setDependency(dependency_);
    n->triggered(triggeredCollector_, triggeredScanner_);

    triggerModel_->setTriggerCollector(triggerCollector_);
    triggeredModel_->setTriggerCollector(triggeredCollector_);

    endTriggerUpdate();

    resumeSelection();
}

void TriggerTableWidget::slotTriggerClicked(TriggerTableItem* item) {
    Q_ASSERT(item);

    if (!depBrowser_->document()->isEmpty() && lastSelectedItem_ && lastSelectedItem_->hasData()) {
        if (item->item() && item->item()->sameContents(lastSelectedItem_->item())) {
            return;
        }
    }

    slotTriggerSelection(item);
}

void TriggerTableWidget::slotTriggerSelection(TriggerTableItem* item) {
    Q_ASSERT(item);
    Q_ASSERT(item->item());

    lastSelectedItem_ = VInfo::createFromItem(item->item());

    if (!depInfoWidget_->isVisible()) {
        return;
    }

    QString txt = tr("&nbsp; <TGTYPE> <TG> triggers these parents/children of <CTYPE> <CITEM>");
    QString tgName, tgType;
    QColor col(255, 255, 255);

    VItem* currentItem = nullptr;
    if (info_) {
        currentItem = info_->item();
    }

    if (currentItem) {
        tgName = currentItem->name();
        tgType = QString::fromStdString(currentItem->typeName());
    }
    txt.replace("<CITEM>", "<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<CTYPE>", tgType);

    tgName.clear();
    tgType.clear();
    if (item->item()) {
        tgName = item->item()->name();
        tgType = QString::fromStdString(item->item()->typeName());
    }
    txt.replace("<TG>", "<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<TGTYPE>", tgType);

    depLabel_->setText(txt);
    depBrowser_->reloadItem(item);
}

void TriggerTableWidget::slotTriggeredClicked(TriggerTableItem* item) {
    Q_ASSERT(item);

    if (!depBrowser_->document()->isEmpty() && lastSelectedItem_ && lastSelectedItem_->hasData()) {
        if (item->item() && item->item()->sameContents(lastSelectedItem_->item())) {
            return;
        }
    }

    slotTriggeredSelection(item);
}

void TriggerTableWidget::slotTriggeredSelection(TriggerTableItem* item) {
    Q_ASSERT(item);
    Q_ASSERT(item->item());

    lastSelectedItem_ = VInfo::createFromItem(item->item());

    if (!depInfoWidget_->isVisible()) {
        return;
    }

    QString txt = tr("&nbsp; <TYPE> <ITEM> is triggered by these parents/children of <CTYPE> <CITEM>");
    QString tgType, tgName;
    QColor col(255, 255, 255);

    VItem* currentItem = nullptr;
    if (info_) {
        currentItem = info_->item();
    }

    if (currentItem) {
        tgName = currentItem->name();
        tgType = QString::fromStdString(currentItem->typeName());
    }
    txt.replace("<CITEM>", "<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<CTYPE>", tgType);

    tgName.clear();
    tgType.clear();
    if (item->item()) {
        tgName = item->item()->name();
        tgType = QString::fromStdString(item->item()->typeName());
    }
    txt.replace("<ITEM>", "<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<TYPE>", tgType);

    depLabel_->setText(txt);
    depBrowser_->reloadItem(item);
}

void TriggerTableWidget::slotShowDependencyInfo(bool b) {
    depInfoWidget_->setVisible(b);

    // When the depinfo panel becomes visible we need to update
    // its contents
    if (b && lastSelectedItem_ && lastSelectedItem_->hasData()) {
        if (TriggerTableCollector* tc = triggerModel_->triggerCollector()) {
            if (TriggerTableItem* ti = tc->find(lastSelectedItem_->item())) {
                slotTriggerSelection(ti);
                return;
            }
        }
        if (TriggerTableCollector* tc = triggeredModel_->triggerCollector()) {
            if (TriggerTableItem* ti = tc->find(lastSelectedItem_->item())) {
                slotTriggeredSelection(ti);
                return;
            }
        }
    }
}

void TriggerTableWidget::on_depInfoCloseTb__clicked() {
    if (depInfoWidget_->isVisible()) {
        Q_EMIT depInfoWidgetClosureRequested();
    }
}

void TriggerTableWidget::anchorClicked(const QUrl& link) {
    VInfo_ptr info = VInfo::createFromPath(info_->server(), link.toString().toStdString());
    if (info) {
        Q_EMIT linkSelected(info);
    }
}

void TriggerTableWidget::beginTriggerUpdate() {
    triggerModel_->beginUpdate();
    triggeredModel_->beginUpdate();
}

void TriggerTableWidget::endTriggerUpdate() {
    triggerModel_->endUpdate();
    triggeredModel_->endUpdate();
}

void TriggerTableWidget::resumeSelection() {
    // try to reselect the last selected item
    if (lastSelectedItem_) {
        lastSelectedItem_->regainData();
        if (lastSelectedItem_->hasData()) {
            if (TriggerTableCollector* tc = triggerModel_->triggerCollector()) {
                if (TriggerTableItem* ti = tc->findByContents(lastSelectedItem_->item())) {
                    triggerView_->setCurrentItem(ti);
                    return;
                }
            }

            if (TriggerTableCollector* tc = triggeredModel_->triggerCollector()) {
                if (TriggerTableItem* ti = tc->findByContents(lastSelectedItem_->item())) {
                    triggeredView_->setCurrentItem(ti);
                    return;
                }
            }
        }
        else {
            lastSelectedItem_.reset();
        }
    }
}

void TriggerTableWidget::setTriggeredScanner(TriggeredScanner* scanner) {
    triggeredScanner_ = scanner;
}

void TriggerTableWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect) {
    // The node view only contains one item (=info_) so we simply rerender it to get the
    // update
    if (info_ && info_->node() == node) {
        nodeView_->rerender();
    }

    triggerModel_->nodeChanged(node, aspect);
    triggeredModel_->nodeChanged(node, aspect);
}

void TriggerTableWidget::writeSettings(VComboSettings* vs) {
    vs->beginGroup("triggerTable");
    vs->putQs("splitter1", splitter1_->saveState());
    vs->putQs("splitter2", splitter2_->saveState());
    vs->endGroup();
}

void TriggerTableWidget::readSettings(VComboSettings* vs) {
    vs->beginGroup("triggerTable");
    if (vs->containsQs("splitter1")) {
        splitter1_->restoreState(vs->getQs("splitter1").toByteArray());
    }
    if (vs->containsQs("splitter2")) {
        splitter2_->restoreState(vs->getQs("splitter2").toByteArray());
    }
    vs->endGroup();
}

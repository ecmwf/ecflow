//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerTableWidget.hpp"

#include "Highlighter.hpp"
#include "TriggerItemWidget.hpp"
#include "TriggerTableModel.hpp"
#include "TriggerViewDelegate.hpp"

TriggerTableWidget::TriggerTableWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

    //The collectors
    triggerCollector_=new TriggerTableCollector(false);
    triggeredCollector_=new TriggerTableCollector(false);

    depInfoCloseTb_->setProperty("triggertitle","1");
    depInfoCloseTb_->parent()->setProperty("triggertitle","1");

    //Format labels
    triggerLabel_->setProperty("triggertitle","1");
    triggeredLabel_->setProperty("triggertitle","1");
    depLabel_->setProperty("triggertitle","1");

    depLabelText_=tr("Dependency details");
    depLabel_->setText(depLabelText_);

    //Trigger - model + view
    triggerModel_ = new TriggerTableModel(TriggerTableModel::TriggerMode,this);
    triggerView_->setModel(triggerModel_);

    //triggered - model + view
    triggeredModel_ = new TriggerTableModel(TriggerTableModel::TriggeredMode,this);
    triggeredView_->setModel(triggeredModel_);

    connect(triggerView_,SIGNAL(selectionChanged(TriggerTableItem*)),
            this,SLOT(slotTriggerSelection(TriggerTableItem*)));

    connect(triggeredView_,SIGNAL(selectionChanged(TriggerTableItem*)),
            this,SLOT(slotTriggeredSelection(TriggerTableItem*)));
}

TriggerTableWidget::~TriggerTableWidget()
{
}

void TriggerTableWidget::clear()
{
    info_.reset();
    triggerModel_->clearData();
    triggeredModel_->clearData();
}

void TriggerTableWidget::setInfo(VInfo_ptr info)
{
    info_=info;
}

void TriggerTableWidget::slotTriggerSelection(TriggerTableItem* item)
{
    QString txt=tr("&nbsp;The parents/children of <CTYPE> <CITEM> that <TGTYPE> <TG> triggers");
    QString tgName,tgType;
    QColor col(255,255,255);

    VItem* currentItem=0;
    if(info_)
        currentItem=info_->item();

    if(currentItem)
    {
        tgName=currentItem->name();
        tgType=QString::fromStdString(currentItem->typeName());
    }
    txt.replace("<CITEM>","<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<CTYPE>",tgType);

    tgName.clear();
    tgType.clear();
    if(item->item())
    {
        tgName=item->item()->name();
        tgType=QString::fromStdString(item->item()->typeName());
    }
    txt.replace("<TG>","<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<TGTYPE>",tgType);

    depLabel_->setText(txt);
    depBrowser_->reload(item);
}

void TriggerTableWidget::slotTriggeredSelection(TriggerTableItem* item)
{
    QString txt=tr("&nbsp;The parents/children of <CTYPE> <CITEM> that triggers <TYPE> <ITEM>");
    QString tgType,tgName;
    QColor col(255,255,255);

    VItem* currentItem=0;
    if(info_)
        currentItem=info_->item();

    if(currentItem)
    {
        tgName=currentItem->name();
        tgType=QString::fromStdString(currentItem->typeName());
    }
    txt.replace("<CITEM>","<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<CTYPE>",tgType);

    tgName.clear();
    tgType.clear();
    if(item->item())
    {
        tgName=item->item()->name();
        tgType=QString::fromStdString(item->item()->typeName());
    }
    txt.replace("<ITEM>","<font color=\'" + col.name() + "\'><b>" + tgName + "</b></font>");
    txt.replace("<TYPE>",tgType);

    depLabel_->setText(txt);
    depBrowser_->reload(item);
}

void TriggerTableWidget::slotShowDependencyInfo(bool b)
{
    depInfoWidget_->setVisible(b);
}

void TriggerTableWidget::on_depInfoCloseTb__clicked()
{
    if(depInfoWidget_->isVisible())
        Q_EMIT  depInfoWidgetClosureRequested();
}

void TriggerTableWidget::beginTriggerUpdate()
{
    triggerModel_->beginUpdate();
    triggeredModel_->beginUpdate();
}

void TriggerTableWidget::endTriggerUpdate()
{
    triggerModel_->endUpdate();
    triggeredModel_->endUpdate();
}

void TriggerTableWidget::setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2)
{
    triggerModel_->setTriggerCollector(tc1);
    triggeredModel_->setTriggerCollector(tc2);
}

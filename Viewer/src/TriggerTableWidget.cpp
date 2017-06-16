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
#include "VSettings.hpp"

TriggerTableWidget::TriggerTableWidget(QWidget *parent) :
    QWidget(parent)
{
	setupUi(this);

    depInfoCloseTb_->setProperty("triggertitle","1");
    depInfoCloseTb_->parent()->setProperty("triggertitle","1");

    //Format labels
    triggerLabel_->setProperty("triggertitle","1");
    triggeredLabel_->setProperty("triggertitle","1");
    depLabel_->setProperty("triggertitle","1");

    depLabelText_=tr(" Dependency details");
    depLabel_->setText(depLabelText_);

    //Trigger - model + view
    triggerModel_ = new TriggerTableModel(TriggerTableModel::TriggerMode,this);
    triggerView_->setModel(triggerModel_);

    //triggered - model + view
    triggeredModel_ = new TriggerTableModel(TriggerTableModel::TriggeredMode,this);
    triggeredView_->setModel(triggeredModel_);

    //normal selection
    connect(triggerView_,SIGNAL(selectionChanged(TriggerTableItem*)),
            this,SLOT(slotTriggerSelection(TriggerTableItem*)));

    connect(triggerView_,SIGNAL(clicked(TriggerTableItem*)),
            this,SLOT(slotTriggerClicked(TriggerTableItem*)));

    connect(triggeredView_,SIGNAL(selectionChanged(TriggerTableItem*)),
            this,SLOT(slotTriggeredSelection(TriggerTableItem*)));

    connect(triggeredView_,SIGNAL(clicked(TriggerTableItem*)),
            this,SLOT(slotTriggeredClicked(TriggerTableItem*)));

    //lookup selection
    connect(triggerView_,SIGNAL(linkSelected(VInfo_ptr)),
            this,SIGNAL(linkSelected(VInfo_ptr)));

    connect(triggeredView_,SIGNAL(linkSelected(VInfo_ptr)),
            this,SIGNAL(linkSelected(VInfo_ptr)));

    //relay commands
    connect(triggerView_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

    connect(triggerView_,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
            this,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

    connect(triggeredView_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

    connect(triggeredView_,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
            this,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

    //anchor clicked in text browser
    connect(depBrowser_,SIGNAL(anchorClicked(const QUrl&)),
            this,SLOT(anchorClicked(const QUrl&)));
}

TriggerTableWidget::~TriggerTableWidget()
{
}

void TriggerTableWidget::clear()
{
    info_.reset();

    triggerModel_->clearData();
    triggeredModel_->clearData();

    depLabel_->setText(depLabelText_);
    depBrowser_->clear();
}

void TriggerTableWidget::clearSelection()
{
    lastSelectedItem_.reset();
}

void TriggerTableWidget::setInfo(VInfo_ptr info)
{
    info_=info;
}

void TriggerTableWidget::slotTriggerClicked(TriggerTableItem* item)
{
    Q_ASSERT(item);

    if(!depBrowser_->document()->isEmpty() && lastSelectedItem_ && lastSelectedItem_->hasData())
    {
        if(item->item() && item->item()->sameContents(lastSelectedItem_->item()))
            return;
    }

    slotTriggerSelection(item);
}

void TriggerTableWidget::slotTriggerSelection(TriggerTableItem* item)
{
    Q_ASSERT(item);
    Q_ASSERT(item->item());

    lastSelectedItem_=VInfo::createFromItem(item->item());

    if(!depInfoWidget_->isVisible())
        return;

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


void TriggerTableWidget::slotTriggeredClicked(TriggerTableItem* item)
{
    Q_ASSERT(item);

    if(!depBrowser_->document()->isEmpty() && lastSelectedItem_ && lastSelectedItem_->hasData())
    {
        if(item->item() && item->item()->sameContents(lastSelectedItem_->item()))
            return;
    }

    slotTriggeredSelection(item);
}

void TriggerTableWidget::slotTriggeredSelection(TriggerTableItem* item)
{
    Q_ASSERT(item);
    Q_ASSERT(item->item());

    lastSelectedItem_=VInfo::createFromItem(item->item());

    if(!depInfoWidget_->isVisible())
        return;

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

    //When the depinfo panel becomes visible we need to update
    //its contents
    if(b && lastSelectedItem_  && lastSelectedItem_->hasData())
    {
        if(TriggerTableCollector *tc=triggerModel_->triggerCollector())
        {
            if(TriggerTableItem *ti=tc->find(lastSelectedItem_->item()))
            {
                slotTriggerSelection(ti);
                return;
            }
        }
        if(TriggerTableCollector *tc=triggeredModel_->triggerCollector())
        {
            if(TriggerTableItem *ti=tc->find(lastSelectedItem_->item()))
            {
                slotTriggeredSelection(ti);
                return;
            }
        }
    }
}

void TriggerTableWidget::on_depInfoCloseTb__clicked()
{
    if(depInfoWidget_->isVisible())
        Q_EMIT  depInfoWidgetClosureRequested();
}

void TriggerTableWidget::anchorClicked(const QUrl& link)
{
    VInfo_ptr info=VInfo::createFromPath(info_->server(),link.toString().toStdString());
    if(info)
        Q_EMIT linkSelected(info);
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

void TriggerTableWidget::resumeSelection()
{
    //try to reselect the last selected item
    if(lastSelectedItem_)
    {
        lastSelectedItem_->regainData();
        if(lastSelectedItem_->hasData())
        {
            if(TriggerTableCollector* tc=triggerModel_->triggerCollector())
            {
                if(TriggerTableItem* ti=tc->findByContents(lastSelectedItem_->item()))
                {
                    triggerView_->setCurrentItem(ti);
                    return;
                }
            }

            if(TriggerTableCollector* tc=triggeredModel_->triggerCollector())
            {
                if(TriggerTableItem* ti=tc->findByContents(lastSelectedItem_->item()))
                {
                    triggeredView_->setCurrentItem(ti);
                    return;
                }
            }
        }
        else
        {
            lastSelectedItem_.reset();
        }
    }
}

void TriggerTableWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
    triggerModel_->nodeChanged(node,aspect);
    triggeredModel_->nodeChanged(node,aspect);
}


void TriggerTableWidget::writeSettings(VComboSettings* vs)
{
    vs->beginGroup("triggerTable");
    vs->putQs("splitter1",splitter1_->saveState());
    vs->putQs("splitter2",splitter2_->saveState());
    vs->endGroup();
}

void TriggerTableWidget::readSettings(VComboSettings* vs)
{
    vs->beginGroup("triggerTable");
    if(vs->containsQs("splitter1"))
    {
        splitter1_->restoreState(vs->getQs("splitter1").toByteArray());
    }
    if(vs->containsQs("splitter2"))
    {
        splitter2_->restoreState(vs->getQs("splitter2").toByteArray());
    }
    vs->endGroup();
}

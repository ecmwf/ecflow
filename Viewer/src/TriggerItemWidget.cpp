//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "TriggerView.hpp"
#include "TriggeredScanner.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"

#include <QActionGroup>

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    messageLabel_->hide();
    messageLabel_->setShowTypeTitle(false);

    //View mode
    modeAg_=new QActionGroup(this);
    modeAg_->addAction(actionTextMode_);
    modeAg_->addAction(actionTableMode_);

    textTb_->setDefaultAction(actionTextMode_);
    tableTb_->setDefaultAction(actionTableMode_);

    connect(modeAg_,SIGNAL(triggered(QAction*)),
            this,SLOT(slotViewMode(QAction*)));

    //textTb_->hide();
    //graphTb_->hide();
    triggerView_->hide();

    //Dependencies
    dependTb_->setChecked(false);

    //Scanner
    scanner_=new TriggeredScanner(this);


    connect(scanner_,SIGNAL(scanStarted()),
            this,SLOT(scanStarted()));

    connect(scanner_,SIGNAL(scanFinished()),
            this,SLOT(scanFinished()));

    connect(scanner_,SIGNAL(scanProgressed(int)),
            this,SLOT(scanProgressed(int)));

    textBrowser_->setOwner(this);
}

QWidget* TriggerItemWidget::realWidget()
{
	return this;
}

void TriggerItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    info_=info;
    //messageLabel_->hide();

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {
        textBrowser_->load();
    }
}

void TriggerItemWidget::load()
{
    if(info_ && info_->isNode() && info_->node())
    {        
        textBrowser_->load();
    }
}

void TriggerItemWidget::clearContents()
{
    InfoPanelItem::clear();
    textBrowser_->clear();
}

void TriggerItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        if(suspended_)
           textBrowser_->suspend();
    }

    checkActionState();
}

void TriggerItemWidget::checkActionState()
{
    if(suspended_)
    {
         dependTb_->setEnabled(false);
         return;
    }

    dependTb_->setEnabled(true);
}

void TriggerItemWidget::on_dependTb__toggled(bool)
{   
    load();
}

void TriggerItemWidget::slotViewMode(QAction* ac)
{
    if(ac == actionTextMode_)
    {
        textBrowser_->setTextMode();
    }
    else if(ac == actionTableMode_)
    {
        textBrowser_->setTableMode();
    }
}

bool TriggerItemWidget::dependency() const
{
    return dependTb_->isChecked();
}
#if 0
void TriggerItemWidget::infoProgressStart(const std::string& text,int max)
{
    messageLabel_->showInfo(QString::fromStdString(text));
    messageLabel_->startProgress(max);
}

void TriggerItemWidget::infoProgress(const std::string& text,int value)
{
    messageLabel_->progress(QString::fromStdString(text),value);
}

#endif

void TriggerItemWidget::scanStarted()
{
    messageLabel_->showInfo("Mapping trigger connections in the whole tree ...");
    messageLabel_->startProgress(100);
}

void TriggerItemWidget::scanFinished()
{
    messageLabel_->stopProgress();
    messageLabel_->hide();
}

void TriggerItemWidget::scanProgressed(int value)
{
    std::string text="";
    messageLabel_->progress(QString::fromStdString(text),value);
}

void TriggerItemWidget::writeSettings(VSettings* vs)
{
#if 0
    vs->beginGroup("triggers");
    vs->putAsBool("dependency",dependency());
    vs->endGroup();
#endif
}

void TriggerItemWidget::readSettings(VSettings* vs)
{
#if 0
    vs->beginGroup("triggers");
    dependTb_->setChecked(vs->getAsBool("dependency",dependency()));
    vs->endGroup();
#endif
}

//-------------------------
// Update
//-------------------------

void TriggerItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    if(!info_ || !info_->isNode())
        return;

    //Changes in the nodes
    for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    {
        if(*it == ecf::Aspect::ADD_REMOVE_ATTR || *it == ecf::Aspect::NODE_VARIABLE ||
            *it == ecf::Aspect::EXPR_TRIGGER || *it == ecf::Aspect::EXPR_COMPLETE)
        {
            textBrowser_->nodeChanged(n);
            return;
        }
    }
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");

//============================================================================
// Copyright 2016 ECMWF.
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

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    textTb_->hide();
    graphTb_->hide();
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

    textBrowser_->setScanner(scanner_);
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
        load();
    }
}

void TriggerItemWidget::load()
{
    if(info_ && info_->isNode() && info_->node())
    {        
        textBrowser_->load(info_,dependency());
    }
}

void TriggerItemWidget::clearContents()
{
	InfoPanelItem::clear();
}


void TriggerItemWidget::on_dependTb__toggled(bool)
{   
    load();
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
    messageLabel_->showInfo("Scan");
    messageLabel_->startProgress(100);
}

void TriggerItemWidget::scanFinished()
{
    messageLabel_->stopProgress();
    messageLabel_->hide();
}

void TriggerItemWidget::scanProgressed(int value)
{
    std::string text="A";
    messageLabel_->progress(QString::fromStdString(text),value);
}

void TriggerItemWidget::writeSettings(VSettings* vs)
{
    vs->beginGroup("triggers");
    vs->putAsBool("dependency",dependency());
    vs->endGroup();
}

void TriggerItemWidget::readSettings(VSettings* vs)
{
    vs->beginGroup("triggers");
    dependTb_->setChecked(vs->getAsBool("dependency",dependency()));
    vs->endGroup();
}


static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");

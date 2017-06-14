//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "Highlighter.hpp"
#include "TriggerCollector.hpp"
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

    //textTb_->setDefaultAction(actionTextMode_);
    //tableTb_->setDefaultAction(actionTableMode_);

    connect(modeAg_,SIGNAL(triggered(QAction*)),
            this,SLOT(slotViewMode(QAction*)));

    //textTb_->hide();
    //graphTb_->hide();

    dependInfoTb_->setChecked(false);
    on_dependInfoTb__toggled(false);

    //Expression
    exprTb_->setChecked(true);

    Highlighter* ih=new Highlighter(exprTe_->document(),"trigger");
    exprTe_->setReadOnly(true);
    exprTe_->setBackgroundVisible(true);

    //Set the height of the trigger expression display area
    QFont fTe;
    fTe.setBold(true);
    QFontMetrics fm(fTe);
    exprTe_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    exprTe_->setFixedHeight(fm.size(0,"A\nA\nA").height()+fm.height()/2);

    //Dependencies
    dependTb_->setChecked(false);

    connect(triggerTable_,SIGNAL(depInfoWidgetClosureRequested()),
            this,SLOT(slotHandleDefInfoWidgetClosure()));

    //The collectors
    tgCollector_=new TriggerTableCollector(false);
    tgdCollector_=new TriggerTableCollector(false);

    //Scanner
    scanner_=new TriggeredScanner(this);

    connect(scanner_,SIGNAL(scanStarted()),
            this,SLOT(scanStarted()));

    connect(scanner_,SIGNAL(scanFinished()),
            this,SLOT(scanFinished()));

    connect(scanner_,SIGNAL(scanProgressed(int)),
            this,SLOT(scanProgressed(int)));

   //textBrowser_->setOwner(this);
   // textBrowser_->hide();
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

    triggerTable_->setInfo(info_);

    //messageLabel_->hide();

    //Info must be a node
    load();

#if 0
    if(info_ && info_->isNode() && info_->node())
    {
        textBrowser_->load();
    }
#endif
}

void TriggerItemWidget::load()
{
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *n=info_->node();
        Q_ASSERT(n);

        //Display trigger expression
        std::string te,ce;
        n->triggerExpr(te,ce);
        QString txt=QString::fromStdString(te);
        if(txt.isEmpty()) txt=tr("No trigger expression is available for the selected node!");
        exprTe_->setPlainText(txt);

        //Load table
        triggerTable_->beginTriggerUpdate();

        //collect the list of triggers of this node
        tgCollector_->setDependency(dependency());
        n->triggers(tgCollector_);

        tgdCollector_->setDependency(dependency());
        n->triggered(tgdCollector_,triggeredScanner());

        triggerTable_->setTriggerCollector(tgCollector_,tgdCollector_);
        triggerTable_->endTriggerUpdate();
    }
}

void TriggerItemWidget::clearContents()
{
    InfoPanelItem::clear();
    triggerTable_->clear();
    //textBrowser_->clear();
}

void TriggerItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //if(suspended_)
        //   textBrowser_->suspend();
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

void TriggerItemWidget::on_dependInfoTb__toggled(bool b)
{
    triggerTable_->slotShowDependencyInfo(b);
}

void TriggerItemWidget::slotHandleDefInfoWidgetClosure()
{
    dependInfoTb_->setChecked(false);
}


void TriggerItemWidget::on_exprTb__toggled(bool b)
{
    exprTe_->setVisible(b);
}

void TriggerItemWidget::slotViewMode(QAction* ac)
{
    /*if(ac == actionTextMode_)
    {
        textBrowser_->setTextMode();
    }
    else if(ac == actionTableMode_)
    {
        textBrowser_->setTableMode();
    }*/
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
            //textBrowser_->nodeChanged(n);
            return;
        }
    }
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");

//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "Highlighter.hpp"
#include "ServerHandler.hpp"
#include "TriggeredScanner.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include <QButtonGroup>

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    //This item will listen to any changes in nodes
    handleAnyChange_=true;

    //We will not keep the contents when the item becomes unselected
    //unselectedFlags_.clear();

    setupUi(this);

    //Scanner
    scanner_=new TriggeredScanner(this);

    connect(scanner_,SIGNAL(scanStarted()),
            this,SLOT(scanStarted()));

    connect(scanner_,SIGNAL(scanFinished()),
            this,SLOT(scanFinished()));

    connect(scanner_,SIGNAL(scanProgressed(int)),
            this,SLOT(scanProgressed(int)));

    triggerTable_->setTriggeredScanner(scanner_);
    triggerGraph_->setTriggeredScanner(scanner_);

    //Messages
    messageLabel_->hide();
    messageLabel_->setShowTypeTitle(false);

    //Dependency info inti must precede dependency init
    //since they depend on each other

    //dependency info is off by default
    dependInfoTb_->setChecked(false);
    on_dependInfoTb__toggled(false);

    //Dependency is off by default
    //dependTb_->setProperty("triggerDepend","1");
    dependTb_->setChecked(false);
    on_dependTb__toggled(false);

    //Expression
    exprTb_->setChecked(true);

    //The document becomes the owner of the highlighter
    new Highlighter(exprTe_->document(),"trigger");
    exprTe_->setReadOnly(true);
    exprTe_->setBackgroundVisible(true);

    //Set the height of the trigger expression display area
    QFont fTe;
    fTe.setBold(true);
    QFontMetrics fm(fTe);
    exprTe_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    exprHeight_ = fm.size(0,"A\nA\nA").height()+fm.height()/2;
    exprEmptyHeight_ = fm.size(0,"A").height()+fm.height()/2;
    exprTe_->setFixedHeight(exprEmptyHeight_);

    //view mode - we show the table by default
    modeGroup_ = new QButtonGroup(this);
    modeGroup_->addButton(tableTb_, TableModeIndex);
    modeGroup_->addButton(graphTb_, GraphModeIndex);
    tableTb_->setChecked(true);
    modeStacked_->setCurrentIndex(TableModeIndex);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(modeGroup_, SIGNAL(buttonToggled(QAbstractButton*,bool)),
            this, SLOT(slotChangeMode(QAbstractButton*, bool)));
#else
    connect(modeGroup_, SIGNAL(buttonToggled(int,bool)),
            this, SLOT(slotChangeMode(int, bool)));
#endif
    zoomSlider_->setMaximumWidth(120);
    triggerGraph_->setZoomSlider(zoomSlider_);
    zoomLabel_->setProperty("graphTitle", "1");
    infoTb_->hide();
    showGraphButtons(false);

    //table
    connect(triggerTable_,SIGNAL(depInfoWidgetClosureRequested()),
            this,SLOT(slotHandleDefInfoWidgetClosure()));

    connect(triggerTable_,SIGNAL(linkSelected(VInfo_ptr)),
            this,SLOT(slotLinkSelected(VInfo_ptr)));

    connect(triggerTable_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
        this,SLOT(slotInfoPanelCommand(VInfo_ptr,QString)));

    connect(triggerTable_,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
        this,SLOT(slotDashboardCommand(VInfo_ptr,QString)));

    //graph
    connect(triggerGraph_,SIGNAL(linkSelected(VInfo_ptr)),
            this,SLOT(slotLinkSelected(VInfo_ptr)));

    connect(triggerGraph_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
        this,SLOT(slotInfoPanelCommand(VInfo_ptr,QString)));

    connect(triggerGraph_,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
        this,SLOT(slotDashboardCommand(VInfo_ptr,QString)));
}

TriggerItemWidget::~TriggerItemWidget()
{
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

    if(info && info->server() && info->server()->isDisabled())
    {
        setEnabled(false);
        return;
    }
    else
    {
        setEnabled(true);
    }

    //set the info
    adjust(info);

    //messageLabel_->hide();

    //Info must be a node
    load();
}

void TriggerItemWidget::load()
{
    clearTriggers();

    if(info_ && info_->isNode() && info_->node())
    {
        VNode* n=info_->node();
        Q_ASSERT(n);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
        //Display trigger expression
        std::string te,ce;
        n->triggerExpr(te,ce);
        QString txt=QString::fromStdString(te);
        if(txt.isEmpty()) {
            txt=tr("No trigger expression is available for the selected node!");
            if (exprTe_->height() != exprEmptyHeight_) {
                exprTe_->setFixedHeight(exprEmptyHeight_);
            }
        } else if (exprTe_->height() != exprHeight_) {
            exprTe_->setFixedHeight(exprHeight_);
        }
        exprTe_->setPlainText(txt);

        if (modeGroup_->checkedId() == TableModeIndex) {
            loadTable();
        } else {
            loadGraph();
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
    }
}

void TriggerItemWidget::loadTable()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
    triggerTable_->clear();
    triggerTable_->setInfo(info_, dependency());

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
}

void TriggerItemWidget::loadGraph()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    triggerGraph_->clear();
    triggerGraph_->setInfo(info_, dependency());

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
}

void TriggerItemWidget::clearContents()
{
    InfoPanelItem::clear();
    exprTe_->clear();
    triggerTable_->clear();
    triggerGraph_->clear();

    if(!active_)
        triggerTable_->clearSelection();
}

void TriggerItemWidget::clearTriggers()
{
    exprTe_->clear();
    triggerTable_->clear();
    triggerGraph_->clear();
}

void TriggerItemWidget::rerender()
{
    if (modeGroup_->checkedId() == TableModeIndex) {
        //
    } else {
        triggerGraph_->rerender();
    }
}

void TriggerItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //If we are here this item is active but not selected!

        //When it becomes suspended we need to clear everything since the
        //tree is probably cleared at this point
        if(suspended_)
        {
            clearTriggers();
        }
        //When we leave the suspended state we need to reload everything
        else
        {
            load();
        }
    } else if(flags.isSet(SelectedChanged)) {
        if (selected_ && active_) {
            if (!info_ || !info_->node()) {
                load();
            } else {
                rerender();
            }
        }
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

void TriggerItemWidget::on_dependTb__toggled(bool b)
{   
    load();

    //when we activate the dependencies we always show the
    //dependency details as well
    if(b)
    {
        dependInfoTb_->setEnabled(true);
        if(dependInfoTb_->isChecked() == false)
        {
            dependInfoTb_->setChecked(true);
        }
    }
    else
    {
        dependInfoTb_->setChecked(false);
        dependInfoTb_->setEnabled(false);
    }
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

bool TriggerItemWidget::dependency() const
{
    return dependTb_->isChecked();
}

void TriggerItemWidget::slotLinkSelected(VInfo_ptr info)
{
   InfoPanelItem::linkSelected(info);
}

void TriggerItemWidget::slotInfoPanelCommand(VInfo_ptr info,QString cmd)
{
    InfoPanelItem::relayInfoPanelCommand(info,cmd);
}

void TriggerItemWidget::slotDashboardCommand(VInfo_ptr info,QString cmd)
{
    InfoPanelItem::relayDashboardCommand(info,cmd);
}

#if 0
void TriggerItemWidget::infoProgressStart(const std::string& text,int max)
{
    messageLabel_->showInfo(QString::fromStdString(text));
    messageLabel_->startProgress(max);
}

void TriggerItemWidget::infoProgressUpdate(const std::string& text,int value)
{
    messageLabel_->progress(QString::fromStdString(text),value);
}

#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void TriggerItemWidget::slotChangeMode(QAbstractButton*, bool)
#else
void TriggerItemWidget::slotChangeMode(int, bool)
#endif
{
    modeStacked_->setCurrentIndex(modeGroup_->checkedId());
    showGraphButtons(modeGroup_->checkedId() == GraphModeIndex);

    if (modeGroup_->checkedId() == TableModeIndex) {
        if (triggerTable_->info() != info_ ||
            triggerTable_->dependency() != dependency()) {
            triggerGraph_->becameInactive();
            loadTable();
        }
    } else if (modeGroup_->checkedId() == GraphModeIndex) {
        if (triggerGraph_->info() != info_ ||
            triggerGraph_->dependency() != dependency()) {
            loadGraph();
        }
    }
}

void TriggerItemWidget::showGraphButtons(bool b)
{
    dependInfoTb_->setVisible(!b);
    zoomLabel_->setVisible(b);
    zoomSlider_->setVisible(b);
}

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

void TriggerItemWidget::writeSettings(VComboSettings* vs)
{
    vs->beginGroup("triggers");
    vs->putAsBool("dependency",dependency());
    vs->putAsBool("dependencyInfo",dependInfoTb_->isChecked());
    vs->putAsBool("expression",exprTb_->isChecked());
    vs->put("mode", modeGroup_->checkedId());

    triggerTable_->writeSettings(vs);
    triggerGraph_->writeSettings(vs);
    vs->endGroup();
}

void TriggerItemWidget::readSettings(VComboSettings* vs)
{
    vs->beginGroup("triggers");

    dependTb_->setChecked(vs->getAsBool("dependency",dependency()));

// dependInfoTb_ is initialised by dependTb_ !!
#if 0
    if(dependTb_->isChecked())
    {
        dependInfoTb_->setChecked(vs->getAsBool("dependencyInfo",dependInfoTb_->isChecked()));
    }
#endif

    exprTb_->setChecked(vs->getAsBool("expression",exprTb_->isChecked()));

    int mode = vs->get<int>("mode", 0);
    if (mode == TableModeIndex) {
        tableTb_->setChecked(true);
    } else if (mode == GraphModeIndex) {
        graphTb_->setChecked(true);
    }

    triggerTable_->readSettings(vs);
    triggerGraph_->readSettings(vs);
    vs->endGroup();
}

//-------------------------
// Update
//-------------------------

void TriggerItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    if(!info_ || !info_->isNode())
        return;

    //If the triggers are not scanned there must have been a major change and
    //we need to reload the item
    if(!info_->node()->root()->triggeredScanned())
    {
        if(selected_ && active_) {
            load();
        } else {
            clearContents();
        }
        return;
    }

    //For certain changes we need to reload the triggers
    for(auto it : aspect)
    {
        if(it == ecf::Aspect::ADD_REMOVE_ATTR || it == ecf::Aspect::EXPR_TRIGGER)
        {
            if(selected_ && active_) {
                load();
            } else {
                clearContents();
            }
            return;
        }
    }

    //For other changes we only reload the triggers if the change happened to an item in the collected triggers
//    for(auto it : aspect)
//    {
//        if(it == ecf::Aspect::NODE_VARIABLE || it == ecf::Aspect::METER || it == ecf::Aspect::LIMIT ||
//                it == ecf::Aspect::EVENT)
//        {
//           if (modeGroup_->checkedId() ==  TableModeIndex) {
//               if (triggerTable_->contains(n)) {
//                   load();
//                   return;
//               }
//           } else if (modeGroup_->checkedId() ==  TableModeIndex) {
//               if (triggerGraph_->contains(n)) {
//                   load();
//                   return;
//               }
//           }
//        }
//    }

    //We do not track further  changes when the item is not selected
    if(!selected_ || !active_)
        return;

    //For the rest of the changes we rerender the collected items that might have changed
    if (modeGroup_->checkedId() ==  TableModeIndex)
        triggerTable_->nodeChanged(n, aspect);
    else if (modeGroup_->checkedId() ==  GraphModeIndex)
        triggerGraph_->nodeChanged(n, aspect);
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");

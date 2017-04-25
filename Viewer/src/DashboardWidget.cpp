/***************************** LICENSE START ***********************************

 Copyright 2009-2017 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "DashboardWidget.hpp"

#include "VSettings.hpp"

static QString tooltipUnchk("This panel is <b>linked</b> and will respond to selections made in other panels. Click to toggle.") ;
static QString tooltipChk("This panel is <b>detached</b> and will not respond to selections made in other panels. Click to toggle.");

static QString tooltipMaxChk("This panel is <b>maximised</b> and hides other panels. Click to restore its original size.") ;
static QString tooltipMaxUnchk("Maximise panel");

DashboardWidget::DashboardWidget(const std::string& type, QWidget* parent) :
    QWidget(parent),
    type_(type),
    acceptSetCurrent_(false),
    bcWidget_(0),
    ignoreMaximisedChange_(false),
    inDialog_(false)
{
    //detach
    detachedAction_=new QAction("Detached",this);
    QIcon ic(QPixmap(":viewer/dock_chain_closed.svg"));
    ic.addPixmap(QPixmap(":viewer/dock_chain_open.svg"),QIcon::Normal,QIcon::On);
    detachedAction_->setIcon(ic);
    detachedAction_->setCheckable(true);
    detachedAction_->setChecked(false);

    connect(detachedAction_,SIGNAL(toggled(bool)),
            this,SLOT(slotDetachedToggled(bool)));

    detachedAction_->setToolTip(tooltipUnchk);

    //maximise
    maximisedAction_=new QAction("Maximise",this);
    QIcon icM(QPixmap(":viewer/dock_max.svg"));
    icM.addPixmap(QPixmap(":viewer/dock_restore.svg"),QIcon::Normal,QIcon::On);
    icM.addPixmap(QPixmap(":viewer/dock_max_disabled.svg"),QIcon::Disabled,QIcon::Off);
    maximisedAction_->setIcon(icM);
    maximisedAction_->setCheckable(true);
    maximisedAction_->setChecked(false);
    maximisedAction_->setToolTip("max");
    connect(maximisedAction_,SIGNAL(toggled(bool)),
            this,SLOT(slotMaximisedToggled(bool)));

    maximisedAction_->setToolTip(tooltipMaxUnchk);
}

void DashboardWidget::slotDetachedToggled(bool b)
{
    detachedChanged();
    detachedAction_->setToolTip((detached())?tooltipChk:tooltipUnchk);
}

void DashboardWidget::setDetached(bool b)
{
    if(detached() != b)
    {
        detachedAction_->setChecked(b);
    }
}

bool DashboardWidget::detached() const
{
    return detachedAction_->isChecked();
}

void DashboardWidget::slotMaximisedToggled(bool b)
{
    if(!ignoreMaximisedChange_)
        Q_EMIT maximisedChanged(this);

    if(b)
        Q_EMIT titleUpdated("Panel maximised!","warning");
    else
        Q_EMIT titleUpdated("");

    maximisedAction_->setToolTip((isMaximised())?tooltipMaxChk:tooltipMaxUnchk);
}

bool DashboardWidget::isMaximised() const
{
    return maximisedAction_->isChecked();
}

//reset the state of the maximised actione without emitting a change signal
void DashboardWidget::resetMaximised()
{
    ignoreMaximisedChange_=true;
    maximisedAction_->setChecked(false);
    ignoreMaximisedChange_=false;
}

void DashboardWidget::setEnableMaximised(bool st)
{
    resetMaximised();
    maximisedAction_->setEnabled(st);
}

void DashboardWidget::setInDialog(bool b)
{
    if(inDialog_ != b)
    {
        inDialog_=b;
        QIcon ic(QPixmap(":viewer/chain_closed.svg"));
        ic.addPixmap(QPixmap(":viewer/chain_open.svg"),QIcon::Normal,QIcon::On);
        detachedAction_->setIcon(ic);
    }

    if(b)
    {
        maximisedAction_->setEnabled(false);
        maximisedAction_->setVisible(false);
    }
}

void DashboardWidget::writeSettings(VSettings* vs)
{
    vs->putAsBool("detached",detached());
}

void DashboardWidget::readSettings(VSettings* vs)
{
    setDetached(vs->getAsBool("detached",detached()));
}

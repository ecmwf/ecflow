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

DashboardWidget::DashboardWidget(const std::string& type, QWidget* parent) :
    QWidget(parent),
    type_(type),
    acceptSetCurrent_(false)
{
    detachedAction_=new QAction("Detached",this);
    QIcon ic(QPixmap(":viewer/dock_chain_closed.svg"));
    ic.addPixmap(QPixmap(":viewer/dock_chain_open.svg"),QIcon::Normal,QIcon::On);
    detachedAction_->setIcon(ic);
    detachedAction_->setCheckable(true);
    detachedAction_->setChecked(false);

    connect(detachedAction_,SIGNAL(toggled(bool)),
            this,SLOT(slotDetachedToggled(bool)));

    detachedAction_->setToolTip(tooltipUnchk);
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

void DashboardWidget::writeSettings(VSettings* vs)
{
    vs->putAsBool("detached",detached());
}

void DashboardWidget::readSettings(VSettings* vs)
{
    setDetached(vs->getAsBool("detached",detached()));
}

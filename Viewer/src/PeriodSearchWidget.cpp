//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PeriodSearchWidget.hpp"

#include "NodeQueryOption.hpp"

#include <QGridLayout>

PeriodSearchWidget::PeriodSearchWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    periodCheck_->setChecked(true);
    connect(periodCheck_,SIGNAL(stateChanged(int)),
        this,SLOT(slotUpdateItems(int)));

    //last period
    lastValueSpin_->setRange(1,100000);

    lastUnitsCb_->addItem(tr("minutes"),"minute");
    lastUnitsCb_->addItem(tr("hours"),"hour");
    lastUnitsCb_->addItem(tr("days"),"day");
    lastUnitsCb_->addItem(tr("weeks"),"week");
    lastUnitsCb_->addItem(tr("months"),"month");
    lastUnitsCb_->addItem(tr("years"),"year");

    lastItemLst_ << lastRb_ << lastValueSpin_ << lastUnitsCb_;

    connect(lastValueSpin_,SIGNAL(valueChanged(int)),
             this,SLOT(lastValueChanged(int)));

    connect(lastUnitsCb_,SIGNAL(currentIndexChanged(int)),
             this,SLOT(lastUnitsChanged(int)));

    //Period: from-to
    periodFromDe_->setDate(QDate::currentDate());
    periodFromDe_->setMaximumDate(QDate::currentDate());
    periodToDe_->setDate(QDate::currentDate());
    periodToDe_->setMaximumDate(QDate::currentDate());
    periodToDe_->setTime(QTime(23,59));

    periodItemLst_ << periodRb_ << periodFromDe_ << periodToLabel_ << periodToDe_;

    connect(periodRb_,SIGNAL(toggled(bool)),
        this,SLOT(slotPeriodRadio(bool)));

    connect(lastRb_,SIGNAL(toggled(bool)),
        this,SLOT(slotPeriodRadio(bool)));

    connect(periodFromDe_,SIGNAL(dateTimeChanged(QDateTime)),
            this,SLOT(slotFromChanged(QDateTime)));

    connect(periodToDe_,SIGNAL(dateTimeChanged(QDateTime)),
            this,SLOT(slotToChanged(QDateTime)));

    //Init
    lastUnitsCb_->setCurrentIndex(1); //minutes
    lastRb_->toggle();
    periodCheck_->setChecked(false);

    slotUpdateItems(0);
}

void PeriodSearchWidget::slotPeriodRadio(bool)
{
    if(!periodCheck_->isChecked())
    {
        return;
    }
    else
    {
        bool enableLast=true;
        bool enablePeriod=true;
        if(periodRb_->isChecked())
        {
            enableLast=false;
        }
        else
        {
            enablePeriod=false;
        }

        for(int i=1; i < lastItemLst_.count(); i++)
            lastItemLst_[i]->setEnabled(enableLast);

        for(int i=1; i < periodItemLst_.count(); i++)
            periodItemLst_[i]->setEnabled(enablePeriod);

    }

    Q_EMIT changed();
}

void PeriodSearchWidget::slotUpdateItems(int)
{
    bool st=periodCheck_->isChecked();

    Q_FOREACH(QWidget* w,lastItemLst_)
            w->setEnabled(st);

    Q_FOREACH(QWidget* w,periodItemLst_)
            w->setEnabled(st);

    //Init radio button state
    slotPeriodRadio(true);

    Q_EMIT changed();
}

void PeriodSearchWidget::lastValueChanged(int)
{
    Q_EMIT changed();
}

void PeriodSearchWidget::lastUnitsChanged(int)
{
    Q_EMIT changed();
}

void PeriodSearchWidget::slotFromChanged(QDateTime)
{
    Q_EMIT changed();
}

void PeriodSearchWidget::slotToChanged(QDateTime)
{
    Q_EMIT changed();
}

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef PERIODSEARCHWIDGET_HPP
#define PERIODSEARCHWIDGET_HPP

#include <QWidget>
#include <QList>

#include "ui_PeriodSearchWidget.h"

class PeriodSearchWidget : public QWidget, protected Ui::PeriodSearchWidget
{
    Q_OBJECT

    friend class NodeQueryPeriodOptionEdit;

public:
    explicit PeriodSearchWidget(QWidget *parent = 0);
    ~PeriodSearchWidget() {}

protected Q_SLOTS:
    void slotPeriodRadio(bool);
    void slotUpdateItems(int);
    void lastValueChanged(int);
    void lastUnitsChanged(int);
    void slotFromChanged(QDateTime);
    void slotToChanged(QDateTime);

Q_SIGNALS:
    void changed();

private:
    QList<QWidget*> lastItemLst_;
    QList<QWidget*> periodItemLst_;
};

#endif // PERIODSEARCHWIDGET_HPP

//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef CLOCKWIDGET_HPP
#define CLOCKWIDGET_HPP

#include <QLabel>

#include "VProperty.hpp"

class QTimer;
class PropertyMapper;

class ClockWidget : public QLabel, public VPropertyObserver
{
    Q_OBJECT
public:
    ClockWidget(QWidget* parent=0);
    ~ClockWidget();

    void notifyChange(VProperty*);

protected Q_SLOTS:
    void slotTimeOut();

protected:
    void renderTime();
    void adjustTimer();

    PropertyMapper* prop_;
    QTimer* timer_;
    bool showSec_;
};

#endif // CLOCKWIDGET_HPP

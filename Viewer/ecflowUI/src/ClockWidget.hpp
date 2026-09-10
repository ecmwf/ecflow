/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ClockWidget_HPP
#define ecflow_viewer_ClockWidget_HPP

#include <QLabel>

#include "VProperty.hpp"

class QTimer;
class PropertyMapper;

class ClockWidget : public QLabel, public VPropertyObserver {
    Q_OBJECT
public:
    explicit ClockWidget(QWidget* parent = nullptr);
    ~ClockWidget() override;

    void notifyChange(VProperty*) override;

protected Q_SLOTS:
    void slotTimeOut();

protected:
    void renderTime();
    void adjustTimer();

    PropertyMapper* prop_;
    QTimer* timer_;
    bool showSec_{false};
    const int timeoutInMs_{1000};
};

#endif // ecflow_viewer_ClockWidget_HPP

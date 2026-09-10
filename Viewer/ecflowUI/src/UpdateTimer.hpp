/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_UpdateTimer_HPP
#define ecflow_viewer_UpdateTimer_HPP

#include <QTimer>

class UpdateTimer : public QTimer {
public:
    explicit UpdateTimer(QObject* parent = nullptr)
        : QTimer(parent) {}
    void drift(int, int);
};

#endif /* ecflow_viewer_UpdateTimer_HPP */

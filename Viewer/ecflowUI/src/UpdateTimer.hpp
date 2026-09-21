// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTimer>

class UpdateTimer : public QTimer {
public:
    explicit UpdateTimer(QObject* parent = nullptr)
        : QTimer(parent) {}
    void drift(int, int);
};

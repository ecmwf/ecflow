// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QComboBox>

class NodeQueryCombo : public QComboBox {
    Q_OBJECT

public:
    explicit NodeQueryCombo(QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotCurrentChanged(int current);

Q_SIGNALS:
    void changed(QString);
};

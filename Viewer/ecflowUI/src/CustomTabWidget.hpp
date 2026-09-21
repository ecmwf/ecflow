// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTabWidget>

class CustomTabWidget : public QTabWidget {
public:
    explicit CustomTabWidget(QWidget* parent = nullptr);

    void setCustomIcon(int index, QPixmap pix);

protected:
    QSize maxIconSize() const;
};

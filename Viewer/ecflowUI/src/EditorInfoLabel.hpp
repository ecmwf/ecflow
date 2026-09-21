// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QLabel>

class EditorInfoLabel : public QLabel {
public:
    explicit EditorInfoLabel(QWidget* parent = nullptr);
    void setInfo(QString parent, QString type);

    static QString formatKeyLabel(QString n);
    static QString formatNodeName(QString n);
    static QString formatNodePath(QString p);
};

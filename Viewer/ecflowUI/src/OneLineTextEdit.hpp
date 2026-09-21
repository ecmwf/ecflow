// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTextEdit>

class OneLineTextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit OneLineTextEdit(QWidget* parent = nullptr);
    QSize sizeHint() const override;

Q_SIGNALS:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* e) override;
};

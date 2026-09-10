/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OneLineTextEditor_HPP
#define ecflow_viewer_OneLineTextEditor_HPP

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

#endif /* ecflow_viewer_OneLineTextEditor_HPP */

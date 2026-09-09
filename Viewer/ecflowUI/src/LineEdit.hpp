/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_LineEdit_HPP
#define ecflow_viewer_LineEdit_HPP

#include <QLineEdit>

class QLabel;
class QToolButton;

class LineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = nullptr);
    void setDecoration(QPixmap);

public Q_SLOTS:
    void slotClear();

Q_SIGNALS:
    void textCleared();

protected:
    void adjustSize();
    void resizeEvent(QResizeEvent*) override;

    QToolButton* clearTb_;
    QLabel* iconLabel_{nullptr};
};

#endif /* ecflow_viewer_LineEdit_HPP */

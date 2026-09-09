/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_GotoLineDialog_HPP
#define ecflow_viewer_GotoLineDialog_HPP

#include "ui_GotoLineDialog.h"

class GotoLineDialog : public QDialog, private Ui::GotoLineDialogQ {
    Q_OBJECT

public:
    explicit GotoLineDialog(QWidget* parent = nullptr);
    ~GotoLineDialog() override;
    void setupUIBeforeShow();

Q_SIGNALS:
    void gotoLine(int line); // emitted when the user says 'ok'

public Q_SLOTS:
    void doneIt();
    void setButtonStatus();
};

#endif /* ecflow_viewer_GotoLineDialog_HPP */

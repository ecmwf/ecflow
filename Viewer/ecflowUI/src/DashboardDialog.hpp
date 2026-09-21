// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDialog>

#include "ui_DashboardDialog.h"

class DashboardWidget;

class DashboardDialog : public QDialog, protected Ui::DashboardDialog {
    Q_OBJECT

public:
    explicit DashboardDialog(QWidget* parent = nullptr);

    void add(DashboardWidget*);
    DashboardWidget* dashboardWidget() const { return dw_; }

public Q_SLOTS:
    void reject() override;
    void slotUpdateTitle(QString, QString);
    void slotOwnerDelete();

Q_SIGNALS:
    void aboutToClose();

protected:
    void closeEvent(QCloseEvent* event) override;
    void readSettings();
    void writeSettings();

    DashboardWidget* dw_{nullptr};
};

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CommandOutputDialog_HPP
#define ecflow_viewer_CommandOutputDialog_HPP

#include <QDialog>

#include "ui_CommandOutputDialog.h"

class ShellCommand;

class CommandOutputDialog : public QDialog, protected Ui::CommandOutputDialog {
    Q_OBJECT

public:
    static void showDialog();

protected Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    explicit CommandOutputDialog(QWidget* parent = nullptr);
    ~CommandOutputDialog() override;

    void closeEvent(QCloseEvent* event) override;

private:
    void readSettings();
    void writeSettings();

    static CommandOutputDialog* dialog_;
};

#endif /* ecflow_viewer_CommandOutputDialog_HPP */

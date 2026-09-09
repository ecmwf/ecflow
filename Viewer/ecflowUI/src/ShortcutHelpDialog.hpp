/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ShortcutHelpDialog_HPP
#define ecflow_viewer_ShortcutHelpDialog_HPP

#include <QDialog>

#include "ui_ShortcutHelpDialog.h"

namespace Ui {
class ShortcutHelpDialog;
}

class ShortcutHelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutHelpDialog(QWidget* parent = nullptr);

public Q_SLOTS:
    void accept() override;

protected:
    void loadText(QString txt);
    void closeEvent(QCloseEvent* event) override;
    void writeSettings();
    void readSettings();

    Ui::ShortcutHelpDialog* ui_;
};

#endif /* ecflow_viewer_ShortcutHelpDialog_HPP */

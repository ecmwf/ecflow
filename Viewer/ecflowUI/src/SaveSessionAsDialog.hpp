/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_SaveSessionAsDialog_HPP
#define ecflow_viewer_SaveSessionAsDialog_HPP

#include <QDialog>

#include "SessionHandler.hpp"
#include "ui_SaveSessionAsDialog.h"

namespace Ui {
class SaveSessionAsDialog;
}

class SaveSessionAsDialog : public QDialog, protected Ui::SaveSessionAsDialog {
    Q_OBJECT

public:
    explicit SaveSessionAsDialog(QWidget* parent = nullptr);
    ~SaveSessionAsDialog() override = default;

public Q_SLOTS:
    void on_saveButton__clicked();
    void on_sessionNameEdit__textChanged();

private:
    // Ui::SaveSessionAsDialog *ui;
    void addSessionToTable(SessionItem* s);
    bool validSaveName(const std::string& name);
    void refreshListOfSavedSessions();
};

#endif /* ecflow_viewer_SaveSessionAsDialog_HPP */

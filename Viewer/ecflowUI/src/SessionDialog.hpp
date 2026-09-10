/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_SessionDialog_HPP
#define ecflow_viewer_SessionDialog_HPP

#include <QDialog>

#include "SessionHandler.hpp"
#include "ui_SessionDialog.h"

namespace Ui {
class SessionDialog;
}

class SessionDialog : public QDialog, protected Ui::SessionDialog {
    Q_OBJECT

public:
    explicit SessionDialog(QWidget* parent = nullptr);
    ~SessionDialog() override = default;

public Q_SLOTS:
    void on_savedSessionsList__currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void on_cloneButton__clicked();
    void on_deleteButton__clicked();
    void on_renameButton__clicked();
    void on_switchToButton__clicked();

private:
    // Ui::SaveSessionAsDialog *ui;
    void addSessionToTable(SessionItem* s);
    void refreshListOfSavedSessions();
    void setButtonsEnabledStatus();
    std::string selectedSessionName();
};

#endif /* ecflow_viewer_SessionDialog_HPP */

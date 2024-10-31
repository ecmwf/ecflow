/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    ~SessionDialog() override;

public Q_SLOTS:
    void on_savedSessionsList__currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
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

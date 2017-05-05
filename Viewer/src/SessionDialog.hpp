//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SESSIONDIALOG_HPP
#define SESSIONDIALOG_HPP

#include <QDialog>
#include "ui_SessionDialog.h"

#include "SessionHandler.hpp"

namespace Ui {
class SessionDialog;
}

class SessionDialog : public QDialog, protected Ui::SessionDialog
{
    Q_OBJECT

public:
    explicit SessionDialog(QWidget *parent = 0);
    ~SessionDialog();

public Q_SLOTS:
    void on_saveButton__clicked();
    void on_sessionNameEdit__textChanged();
    void on_savedSessionsList__currentRowChanged(int currentRow);
    void on_cloneButton__clicked();
    void on_deleteButton__clicked();
    void on_renameButton__clicked();
    void on_switchToButton__clicked();

private:
    //Ui::SaveSessionAsDialog *ui;
    void addSessionToTable(SessionItem *s);
    bool validSaveName(const std::string &name);
    void refreshListOfSavedSessions();
    void setButtonsEnabledStatus();
    std::string selectedSessionName();
};

#endif // SESSIONDIALOG_HPP

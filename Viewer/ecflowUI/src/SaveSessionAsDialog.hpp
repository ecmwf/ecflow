//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SAVESESSIONASDIALOG_HPP
#define SAVESESSIONASDIALOG_HPP

#include <QDialog>
#include "ui_SaveSessionAsDialog.h"

#include "SessionHandler.hpp"

namespace Ui {
class SaveSessionAsDialog;
}

class SaveSessionAsDialog : public QDialog, protected Ui::SaveSessionAsDialog
{
    Q_OBJECT

public:
    explicit SaveSessionAsDialog(QWidget *parent = nullptr);
    ~SaveSessionAsDialog() override;

public Q_SLOTS:
    void on_saveButton__clicked();
    void on_sessionNameEdit__textChanged();

private:
    //Ui::SaveSessionAsDialog *ui;
    void addSessionToTable(SessionItem *s);
    bool validSaveName(const std::string &name);
    void refreshListOfSavedSessions();
};

#endif // SAVESESSIONASDIALOG_HPP

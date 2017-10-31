//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMMANDOUTPUTDIALOG_HPP
#define COMMANDOUTPUTDIALOG_HPP

#include <QDialog>

#include "ui_CommandOutputDialog.h"

class ShellCommand;

class CommandOutputDialog : public QDialog, protected Ui::CommandOutputDialog
{
    Q_OBJECT

public:
    static void showDialog();

protected Q_SLOTS:
    void accept();
    void reject();

protected:
    explicit CommandOutputDialog(QWidget *parent = 0);
    ~CommandOutputDialog();

    void closeEvent(QCloseEvent * event);

private:
    void readSettings();
    void writeSettings();

    static CommandOutputDialog* dialog_;
};

#endif // COMMANDOUTPUTDIALOG_HPP

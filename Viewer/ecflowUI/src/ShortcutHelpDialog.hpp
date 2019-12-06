//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SHORTCUTHELPDIALOG_HPP
#define SHORTCUTHELPDIALOG_HPP

#include <QDialog>

#include "ui_ShortcutHelpDialog.h"

namespace Ui {
    class ShortcutHelpDialog;
}

class ShortcutHelpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShortcutHelpDialog(QWidget *parent=nullptr);

public Q_SLOTS:
    void accept() override;

protected:
    void loadText(QString txt);
    void closeEvent(QCloseEvent *event);
    void writeSettings();
    void readSettings();

    Ui::ShortcutHelpDialog* ui_;
};

#endif // SHORTCUTHELPDIALOG_HPP


//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTFILTERHANDLERDIALOG_HPP
#define TEXTFILTERHANDLERDIALOG_HPP

#include <QDialog>

#include "ui_TextFilterHandlerDialog.h"

class TextFilterHandlerDialog : public QDialog, private Ui::TextFilterHandlerDialog
{
    Q_OBJECT

public:
    explicit TextFilterHandlerDialog(QWidget *parent = 0);
    ~TextFilterHandlerDialog();

    void setItemToAdd(QString name,QString filter);

protected Q_SLOTS:
    void on_addPb__clicked();
    void on_actionEdit__triggered();
    void on_actionDuplicate__triggered();
    void on_actionRemove__triggered();

private:
    void reloadTable();
    QString settingsFile();
    void writeSettings();
    void readSettings();
};

#endif // TEXTFILTERHANDLERDIALOG_HPP


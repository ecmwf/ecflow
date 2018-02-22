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

#include "ui_TextFilterAddDialog.h"
#include "ui_TextFilterHandlerDialog.h"

class TextFilterItem;

class TextFilterAddDialog : public QDialog, private Ui::TextFilterAddDialog
{
Q_OBJECT

public:
    explicit TextFilterAddDialog(QWidget* parent=0);
    void init(const TextFilterItem& item);

public Q_SLOTS:
    void accept();

protected:
    TextFilterItem item();
};

class TextFilterEditDialog : public TextFilterAddDialog
{
Q_OBJECT

public:
    explicit TextFilterEditDialog(QWidget* parent=0);
    void init(int itemIndex,const TextFilterItem& item);

public Q_SLOTS:
    void accept();

protected:
    int itemIndex_;
};

class TextFilterHandlerDialog : public QDialog, private Ui::TextFilterHandlerDialog
{
    Q_OBJECT

public:
    explicit TextFilterHandlerDialog(QWidget *parent = 0);
    ~TextFilterHandlerDialog();

    void setItemToSaveAs(QString name,QString filter,bool matched,bool caseSensitive);
    int applyIndex() const {return applyIndex_;}

protected Q_SLOTS:
    void on_actionAdd__triggered();
    void on_actionEdit__triggered();
    void on_actionDuplicate__triggered();
    void on_actionRemove__triggered();
    void on_actionApply__triggered();
    void on_table__doubleClicked(const QModelIndex& index);

private:
    void reloadTable();
    bool addItem();
    void editItem();
    void updateStatus();
    QString settingsFile();
    void writeSettings();
    void readSettings();

    int applyIndex_;
};

#endif // TEXTFILTERHANDLERDIALOG_HPP


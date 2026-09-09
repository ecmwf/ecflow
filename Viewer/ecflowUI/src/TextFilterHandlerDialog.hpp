/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TextFilterHandlerDialog_HPP
#define ecflow_viewer_TextFilterHandlerDialog_HPP

#include <QDialog>

#include "ui_TextFilterAddDialog.h"
#include "ui_TextFilterHandlerDialog.h"

class TextFilterItem;

class TextFilterAddDialog : public QDialog, private Ui::TextFilterAddDialog {
    Q_OBJECT

public:
    explicit TextFilterAddDialog(QWidget* parent = nullptr);
    void init(const TextFilterItem& item);

public Q_SLOTS:
    void accept() override;

protected:
    TextFilterItem item();
};

class TextFilterEditDialog : public TextFilterAddDialog {
    Q_OBJECT

public:
    explicit TextFilterEditDialog(QWidget* parent = nullptr);
    void init(int itemIndex, const TextFilterItem& item);

public Q_SLOTS:
    void accept() override;

protected:
    int itemIndex_{-1};
};

class TextFilterHandlerDialog : public QDialog, private Ui::TextFilterHandlerDialog {
    Q_OBJECT

public:
    explicit TextFilterHandlerDialog(QWidget* parent = nullptr);
    ~TextFilterHandlerDialog() override;

    void setItemToSaveAs(QString name, QString filter, bool matched, bool caseSensitive);
    int applyIndex() const { return applyIndex_; }

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

    int applyIndex_{-1};
};

#endif /* ecflow_viewer_TextFilterHandlerDialog_HPP */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AddModelColumnDialog_HPP
#define ecflow_viewer_AddModelColumnDialog_HPP

#include <set>
#include <string>

#include <QDialog>
#include <QString>

class ModelColumn;

namespace Ui {
class AddModelColumnDialog;
}

class AddModelColumnDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddModelColumnDialog(QWidget* parent = nullptr);
    ~AddModelColumnDialog() override;

    void init(ModelColumn* mc, const std::set<std::string>&, QString defaultText = QString(""));

public Q_SLOTS:
    void accept() override;

protected:
    Ui::AddModelColumnDialog* ui;
    ModelColumn* modelColumn_{nullptr};
};

class ChangeModelColumnDialog : public AddModelColumnDialog {
    Q_OBJECT

public:
    explicit ChangeModelColumnDialog(QWidget* parent = nullptr);

    void setColumn(QString);

public Q_SLOTS:
    void accept() override;

protected:
    QString columnName_;
};

#endif /* ecflow_viewer_AddModelColumnDialog_HPP */

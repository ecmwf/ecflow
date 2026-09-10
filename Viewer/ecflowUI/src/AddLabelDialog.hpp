/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AddLabelDialog_HPP
#define ecflow_viewer_AddLabelDialog_HPP

#include <QDialog>

#include "VInfo.hpp"

class QCloseEvent;

namespace Ui {
class AddLabelDialog;
}

class AddLabelDialog : public QDialog {
    Q_OBJECT

public:
    AddLabelDialog(VInfo_ptr info, QString labelName, QWidget* parent = nullptr);

protected Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void writeSettings();
    void readSettings();

    Ui::AddLabelDialog* ui_;
    VInfo_ptr info_;
};

#endif /* ecflow_viewer_AddLabelDialog_HPP */

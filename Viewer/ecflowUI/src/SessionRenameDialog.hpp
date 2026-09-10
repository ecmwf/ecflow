/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_SessionRenameDialog_HPP
#define ecflow_viewer_SessionRenameDialog_HPP

#include <QDialog>

#include "SessionHandler.hpp"
#include "ui_SessionRenameDialog.h"

namespace Ui {
class SessionRenameDialog;
}

class SessionRenameDialog : public QDialog, protected Ui::SessionRenameDialog {
    Q_OBJECT

public:
    explicit SessionRenameDialog(QWidget* parent = nullptr);
    ~SessionRenameDialog() override;

    std::string newName() { return newName_; };

public Q_SLOTS:
    void on_buttonBox__accepted();
    void on_buttonBox__rejected();

private:
    std::string newName_;
};

#endif /* ecflow_viewer_SessionRenameDialog_HPP */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SessionRenameDialog.hpp"

#include <QMessageBox>

#include "SessionHandler.hpp"
#include "ui_SessionRenameDialog.h"

SessionRenameDialog::SessionRenameDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi(this);
}

SessionRenameDialog::~SessionRenameDialog() = default;

void SessionRenameDialog::on_buttonBox__accepted() {
    // store the name
    newName_ = nameEdit_->text().toStdString();

    // check it does not clash with an existing session name
    if (SessionHandler::instance()->find(newName_)) {
        QMessageBox::critical(
            nullptr, tr("Rename session"), tr("A session with that name already exists - please choose another name"));
    }
    else {
        // close the dialogue
        accept();
    }
}

void SessionRenameDialog::on_buttonBox__rejected() {
    // close the dialogue
    reject();
}

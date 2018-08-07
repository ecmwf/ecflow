//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QMessageBox>

#include "SessionHandler.hpp"

#include "SessionRenameDialog.hpp"
#include "ui_SessionRenameDialog.h"

SessionRenameDialog::SessionRenameDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
}

SessionRenameDialog::~SessionRenameDialog()
= default;


void SessionRenameDialog::on_buttonBox__accepted()
{
    // store the name
    newName_ = nameEdit_->text().toStdString();


    // check it does not clash with an existing session name
    if (SessionHandler::instance()->find(newName_))
    {
        QMessageBox::critical(0,tr("Rename session"), tr("A session with that name already exists - please choose another name"));
    }
    else
    {
        // close the dialogue
        accept();
    }
}

void SessionRenameDialog::on_buttonBox__rejected()
{
    // close the dialogue
    reject();
}

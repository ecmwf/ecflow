//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QMessageBox>


#include "SessionRenameDialog.hpp"
#include "ui_SessionRenameDialog.h"

SessionRenameDialog::SessionRenameDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
}

SessionRenameDialog::~SessionRenameDialog()
{
}


void SessionRenameDialog::on_buttonBox__accepted()
{
    // store the name
    newName_ = nameEdit_->text().toStdString();

    // close the dialogue
    accept();
}

void SessionRenameDialog::on_buttonBox__rejected()
{
    // close the dialogue
    reject();
}

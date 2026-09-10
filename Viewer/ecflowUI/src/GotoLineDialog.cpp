/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GotoLineDialog.hpp"

#include <QPushButton>

GotoLineDialog::GotoLineDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi(this); // this sets up GUI// setupFileMenu();

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(doneIt()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setButtonStatus()));
}

GotoLineDialog::~GotoLineDialog() = default;

// ---------------------------------------------------------------------------
// GotoLineDialog::setButtonStatus
// if there is text in the input box, then we can activate the 'OK' button,
// otherwise we should disable it. This function is called each time the text
// in the box is changed.
// ---------------------------------------------------------------------------

void GotoLineDialog::setButtonStatus() {
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);

    if (lineEdit->text().isEmpty()) {
        okButton->setEnabled(false);
    }
    else {
        okButton->setEnabled(true);
    }
}

// ---------------------------------------------------------------------------
// GotoLineDialog::setupUIBeforeShow
// sets up UI elements before the dialog is displayed.
// ---------------------------------------------------------------------------

void GotoLineDialog::setupUIBeforeShow() {
    lineEdit->setFocus(Qt::OtherFocusReason);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    setButtonStatus();
}

// ---------------------------------------------------------------------------
// GotoLineDialog::accept
// called when the user clicks the 'OK' button - emits a signal to tell the
// text editor to go to the chosen line
// ---------------------------------------------------------------------------

void GotoLineDialog::doneIt() {
    int line = lineEdit->text().toInt();
    Q_EMIT gotoLine(line);
    close();
}

//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifdef ECFLOW_QT5
 #include <QtWidgets>
#else
 #include <QtGui>
#endif

#include "GotoLineDialog.hpp"


GotoLineDialog::GotoLineDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this); // this sets up GUI// setupFileMenu();


    connect (buttonBox, SIGNAL(accepted()),                   this, SLOT(done()));
    connect (buttonBox, SIGNAL(rejected()),                   this, SLOT(reject()));
    connect (lineEdit,  SIGNAL(textChanged(const QString &)), this, SLOT(setButtonStatus()));
}


GotoLineDialog::~GotoLineDialog()
{

}


// ---------------------------------------------------------------------------
// GotoLineDialog::setButtonStatus
// if there is text in the input box, then we can activate the 'OK' button,
// otherwise we should disable it. This function is called each time the text
// in the box is changed.
// ---------------------------------------------------------------------------

void GotoLineDialog::setButtonStatus()
{
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);

    if (lineEdit->text().isEmpty())
    {
        okButton->setEnabled(false);
    }
    else
    {
        okButton->setEnabled(true);
    }
}


// ---------------------------------------------------------------------------
// GotoLineDialog::setupUIBeforeShow
// sets up UI elements before the dialog is displayed.
// ---------------------------------------------------------------------------

void GotoLineDialog::setupUIBeforeShow()
{
    lineEdit->setFocus(Qt::OtherFocusReason);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    
    setButtonStatus();
}


// ---------------------------------------------------------------------------
// GotoLineDialog::accept
// called when the user clicks the 'OK' button - emits a signal to tell the
// text editor to go to the chosen line
// ---------------------------------------------------------------------------

void GotoLineDialog::done()
{
    int line = lineEdit->text().toInt();
    Q_EMIT gotoLine(line);
    close();
}

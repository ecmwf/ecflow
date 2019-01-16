//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef GotoLineDialog_H
#define GotoLineDialog_H

#include "ui_GotoLineDialog.h"

using namespace std;


class GotoLineDialog : public QDialog, private Ui::GotoLineDialogQ
{
    Q_OBJECT

public:
    explicit GotoLineDialog(QWidget *parent = nullptr);
    ~GotoLineDialog() override;
    void setupUIBeforeShow();

Q_SIGNALS:
    void gotoLine(int line);   // emitted when the user says 'ok'


public Q_SLOTS:
    void done();
    void setButtonStatus();

};

#endif

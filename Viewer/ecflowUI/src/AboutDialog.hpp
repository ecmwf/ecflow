//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ABOUTDIALOG_INC_
#define ABOUTDIALOG_INC_

#include <QDialog>

#include "ui_AboutDialog.h"

class AboutDialog : public QDialog, protected Ui::AboutDialog
{
public:
    explicit AboutDialog(QWidget *parent=nullptr);
};

#endif


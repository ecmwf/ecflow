//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LABELEDITDIALOG_HPP
#define LABELEDITDIALOG_HPP

#include <QDialog>

#include "ui_LabelEditDialog.h"

#include "VInfo.hpp"

class LabelEditDialog : public QDialog, private Ui::LabelEditDialog
{
Q_OBJECT

public:
    LabelEditDialog(VInfo_ptr,QWidget* parent=0);

    //QString name() const;
    //QString value() const;

//public Q_SLOTS:
//    void accept();

protected:

    VInfo_ptr info_;

};

#endif // LABELEDITDIALOG_HPP


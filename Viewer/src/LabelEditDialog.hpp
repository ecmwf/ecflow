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

#include "ui_LabelEditDialog.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LabelEditDialog : private Ui::LabelEditDialog, public AttributeEditor
{
public:
    LabelEditDialog(VInfo_ptr,QWidget* parent=0);

    //QString name() const;
    //QString value() const;

//public Q_SLOTS:
//    void accept();

protected:


};

#endif // LABELEDITDIALOG_HPP


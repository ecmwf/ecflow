//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LIMITEDITDIALOG_HPP
#define LIMITEDITDIALOG_HPP

#include "ui_LimitEditDialog.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LimitEditDialog : private Ui::LimitEditDialog, public AttributeEditor
{
public:
    LimitEditDialog(VInfo_ptr,QWidget* parent=0);

protected:
    void apply();

    int valOri_;
    int maxOri_;
};

#endif // LIMITEDITDIALOG_HPP



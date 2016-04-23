//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LABELEDITOR_HPP
#define LABELEDITOR_HPP

#include "ui_LabelEditWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LabelEditor;

class LabelEditWidget :  public QWidget, protected Ui::LabelEditWidget
{
friend class LabelEditor;
public:
    LabelEditWidget(QWidget *parent=0);
};

class LabelEditor : public AttributeEditor
{
public:
    LabelEditor(VInfo_ptr,QWidget* parent=0);

protected:
    void apply();
    LabelEditWidget* w_;
};

#endif // LABELEDITOR_HPP


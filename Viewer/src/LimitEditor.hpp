//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LIMITEDITOR_HPP
#define LIMITEDITOR_HPP

#include "ui_LimitEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LimitEditor;

class LimitEditorWidget :  public QWidget, protected Ui::LimitEditorWidget
{
friend class LimitEditor;
public:
    LimitEditorWidget(QWidget *parent=0);
};

class LimitEditor : public AttributeEditor
{
public:
    LimitEditor(VInfo_ptr,QWidget* parent=0);

protected:
    void apply();
    void readSettings();
    void writeSettings();

    LimitEditorWidget* w_;
    int valOri_;
    int maxOri_;
};

#endif // LIMITEDITOR_HPP



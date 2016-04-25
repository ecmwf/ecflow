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

#include "ui_LabelEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LabelEditor;

class LabelEditorWidget :  public QWidget, protected Ui::LabelEditorWidget
{
friend class LabelEditor;
public:
    LabelEditorWidget(QWidget *parent=0);
};

class LabelEditor : public AttributeEditor
{
Q_OBJECT
public:
    LabelEditor(VInfo_ptr,QWidget* parent=0);    
    ~LabelEditor();

protected Q_SLOTS:
    void slotResetValue();
    void slotValueChanged();

protected:
    void apply();
    void checkButtonStatus();
    void readSettings();
    void writeSettings();

    LabelEditorWidget* w_;
    QString oriVal_;
};

#endif // LABELEDITOR_HPP


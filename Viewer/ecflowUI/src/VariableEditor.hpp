//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VARIABLEEDITOR_HPP
#define VARIABLEEDITOR_HPP

#include "ui_VariableEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class VariableEditor;

class VariableEditorWidget :  public QWidget, protected Ui::VariableEditorWidget
{
friend class VariableEditor;
public:
    VariableEditorWidget(QWidget *parent=nullptr);
};

class VariableEditor : public AttributeEditor
{
Q_OBJECT
public:
    VariableEditor(VInfo_ptr,QWidget* parent=nullptr);
    ~VariableEditor() override;

protected Q_SLOTS:
    void slotValueChanged();

protected:
    void apply() override;
    void resetValue() override;
    bool isValueChanged() override;
    void readSettings();
    void writeSettings();

    VariableEditorWidget* w_;
    QString oriVal_;
    bool readOnly_;
};

#endif // VARIABLEEDITOR_HPP

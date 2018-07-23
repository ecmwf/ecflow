#ifndef TRIGGEREDITOR_HPP
#define TRIGGEREDITOR_HPP

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ui_TriggerEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class LabelEditor;

class TriggerEditorWidget :  public QWidget, protected Ui::TriggerEditorWidget
{
friend class TriggerEditor;
public:
    TriggerEditorWidget(QWidget *parent=0);
};

class TriggerEditor : public AttributeEditor
{
Q_OBJECT
public:
    TriggerEditor(VInfo_ptr,QWidget* parent=0);
    ~TriggerEditor();

protected Q_SLOTS:
    void slotValueChanged();

protected:
    void apply();
    void resetValue();
    bool isValueChanged();
    void readSettings();
    void writeSettings();

    TriggerEditorWidget* w_;
    QString oriText_;
    QString typeInCmd_;
};

#endif // TRIGGEREDITOR_HPP


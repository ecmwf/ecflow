//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef METEREDITOR_HPP
#define METEREDITOR_HPP

#include "ui_MeterEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class MeterEditor;

class MeterEditorWidget :  public QWidget, protected Ui::MeterEditorWidget
{
friend class MeterEditor;
public:
    MeterEditorWidget(QWidget *parent=0);
};

class MeterEditor : public AttributeEditor
{
Q_OBJECT

public:
    MeterEditor(VInfo_ptr,QWidget* parent=0);
    ~MeterEditor();

protected Q_SLOTS:
    void slotValueChanged(int);

protected:
    void apply();
    void resetValue();
    bool isValueChanged();
    void readSettings();
    void writeSettings();

    MeterEditorWidget* w_;
    int oriVal_;
};

#endif // METEREDITOR_HPP



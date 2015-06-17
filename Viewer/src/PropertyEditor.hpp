//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef PROPERTYEDITOR_INC_
#define PROPERTYEDITOR_INC_

#include <QWidget>

#include "ui_PropertyEditor.h"

class QGridLayout;

class PropertyLine;
class VProperty;

class PropertyEditor : public QWidget, protected Ui::PropertyEditor
{
public:
    PropertyEditor(QWidget *parent=0);
    ~PropertyEditor();

    void edit(VProperty*);
    void editAccepted();

private:
    void build();
    void addItem(VProperty* vProp,QGridLayout* grid);
    //void syncToConfig(QtProperty*);


    //This is a map between the properties in the editor and in the config
    //QMap<QtProperty*,VProperty*> confMap_;

    VProperty* group_;

};


#endif


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

#include <QMap>
#include <QWidget>

class QtTreePropertyBrowser;
class QtVariantEditorFactory;
class QtProperty;

class VProperty;

class PropertyEditor : public QWidget
{
public:
    PropertyEditor(QWidget *parent=0);
    ~PropertyEditor();

    void editAccepted();

private:
    void build();
    void addItem(VProperty* vProp,QtProperty* parentProp);
    void syncToConfig(QtProperty*);

    QtVariantEditorFactory* factory_;
    QtTreePropertyBrowser* browser_;

    //This is a map between the properties in the editor and in the config
    QMap<QtProperty*,VProperty*> confMap_;
};

#endif


//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include <QStringList>

class LimitEditor;
class QStringListModel;

class LimitEditorWidget :  public QWidget, protected Ui::LimitEditorWidget
{
friend class LimitEditor;
public:
    LimitEditorWidget(QWidget *parent=0);
};

class LimitEditor : public AttributeEditor
{
Q_OBJECT
public:
    LimitEditor(VInfo_ptr,QWidget* parent=0);
    ~LimitEditor();

protected Q_SLOTS:
    void slotMaxChanged(int);
    void slotRemove();
    void slotRemoveAll();

protected:
    void resetValue();
    void apply();
    bool isValueChanged();
    void buildList(VAttribute *a);
    void remove(bool all);
    void nodeChanged(const std::vector<ecf::Aspect::Type>& a);
    void setModelData(QStringList lst);
    void readSettings();
    void writeSettings();

    LimitEditorWidget* w_;
    int oriVal_;
    int oriMax_;
    QStringListModel* model_;
    QStringList modelData_;
};

#endif // LIMITEDITOR_HPP



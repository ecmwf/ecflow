//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EventEditor_HPP
#define EventEditor_HPP

#include "ui_EventEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

#include <QStringList>

class EventEditor;
class QStringListModel;

class EventEditorWidget :  public QWidget, protected Ui::EventEditorWidget
{
friend class EventEditor;
public:
    EventEditorWidget(QWidget *parent=0);
};

class EventEditor : public AttributeEditor
{
Q_OBJECT
public:
    EventEditor(VInfo_ptr,QWidget* parent=0);
    ~EventEditor();

protected Q_SLOTS:  
    void slotLookUp();
    void slotDoubleClicked(const QModelIndex &index);
    void slotStatusToggled(bool);
    void scanStarted();
    void scanFinished();
    void scanProgressed(int);
    void buildList();

protected:
    void paintEvent(QPaintEvent *e);
    void resetValue();
    void apply();
    bool isValueChanged();
    void nodeChanged(const std::vector<ecf::Aspect::Type>& a);
    void setModelData(QStringList lst);
    void lookup(const QModelIndex &index);
    void readSettings();
    void writeSettings();

    EventEditorWidget* w_;
    int oriVal_;
    QStringListModel* model_;
    QStringList modelData_;
    bool scanned_;
};

#endif // EventEditor_HPP



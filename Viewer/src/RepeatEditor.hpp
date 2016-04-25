//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef REPEATEDITOR_HPP
#define REPEATEDITOR_HPP

#include "ui_RepeatEditorWidget.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class QModelIndex;
class QStringList;
class QStringListModel;
class VRepeat;


class RepeatEditor;

class RepeatEditorWidget :  public QWidget, protected Ui::RepeatEditorWidget
{
friend class RepeatEditor;
public:
    RepeatEditorWidget(QWidget *parent=0);
};


class RepeatEditor : public AttributeEditor
{
Q_OBJECT

public:
    RepeatEditor(VInfo_ptr,QWidget* parent=0);
    ~RepeatEditor();

protected Q_SLOTS:
    void slotSelected(const QModelIndex&);
    void slotValueEdited(QString txt);
    void slotResetValue();

protected:
    void buildList();
    bool isListMode() const;
    void checkButtonStatus();
    void apply();
    void readSettings();
    void writeSettings();

    RepeatEditorWidget* w_;
    VRepeat* repeat_;
    QStringListModel* model_;
    QStringList modelData_;
    QString oriVal_;
};

#endif // REPEATEDITOR_HPP



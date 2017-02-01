//============================================================================
// Copyright 2009-2017 ECMWF.
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
friend class RepeatIntEditor;
friend class RepeatStringEditor;
friend class RepeatDateEditor;
public:
    RepeatEditorWidget(QWidget *parent=0);
protected:
    void hideRow(QWidget* w);
};

class RepeatEditor : public AttributeEditor
{
Q_OBJECT

public:
    RepeatEditor(VInfo_ptr,QWidget* parent=0);
    ~RepeatEditor();

protected Q_SLOTS:
    void slotSelectedInView(const QModelIndex&);

protected:
    void buildList();
    bool isListMode() const;   
    virtual void setValue(QString)=0;
    void readSettings();
    void writeSettings();

    RepeatEditorWidget* w_;
    VRepeat* repeat_;
    QStringListModel* model_;
    QStringList modelData_;
    QString oriVal_;
};

class RepeatIntEditor : public RepeatEditor
{
Q_OBJECT
public:
    RepeatIntEditor(VInfo_ptr,QWidget* parent=0);

protected Q_SLOTS:
    void slotValueChanged(int);

protected:
    void apply();
    void setValue(QString val);
    void resetValue();
    bool isValueChanged();
};

class RepeatStringEditor : public RepeatEditor
{
Q_OBJECT
public:
    RepeatStringEditor(VInfo_ptr,QWidget* parent=0);

protected Q_SLOTS:
    void slotValueEdited(QString);

protected:
    void apply();
    void setValue(QString val);
    void resetValue();
    bool isValueChanged();
};

class RepeatDateEditor : public RepeatEditor
{
Q_OBJECT
public:
    RepeatDateEditor(VInfo_ptr,QWidget* parent=0);

protected Q_SLOTS:
    void slotValueEdited(QString);

protected:
    void apply();
    void setValue(QString val);
    void resetValue();
    bool isValueChanged();
};

#endif // REPEATEDITOR_HPP



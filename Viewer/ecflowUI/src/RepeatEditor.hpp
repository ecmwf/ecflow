/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_RepeatEditor_HPP
#define ecflow_viewer_RepeatEditor_HPP

#include <QStringList>

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_RepeatEditorWidget.h"

class QModelIndex;
class QStringListModel;
class VRepeatAttr;
class RepeatEditor;

class RepeatEditorWidget : public QWidget, protected Ui::RepeatEditorWidget {
    friend class RepeatEditor;
    friend class RepeatIntEditor;
    friend class RepeatStringEditor;
    friend class RepeatDateEditor;
    friend class RepeatDateTimeEditor;

public:
    explicit RepeatEditorWidget(QWidget* parent = nullptr);

protected:
    void hideRow(QWidget* w);
};

class RepeatEditor : public AttributeEditor {
    Q_OBJECT

public:
    explicit RepeatEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~RepeatEditor() override;

protected Q_SLOTS:
    void slotSelectedInView(const QModelIndex&, const QModelIndex&);

protected:
    void buildList(VRepeatAttr* rep);
    bool isListMode() const;
    virtual void setValue(QString) = 0;
    void readSettings();
    void writeSettings();

    RepeatEditorWidget* w_;
    // VRepeat* repeat_;
    QStringListModel* model_;
    QStringList modelData_;
    QString oriVal_;
};

class RepeatIntEditor : public RepeatEditor {
    Q_OBJECT
public:
    RepeatIntEditor(VInfo_ptr, QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotValueChanged(int);

protected:
    void initSpinner();
    void apply() override;
    void setValue(QString val) override;
    void resetValue() override;
    bool isValueChanged() override;
};

class RepeatStringEditor : public RepeatEditor {
    Q_OBJECT
public:
    RepeatStringEditor(VInfo_ptr, QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotValueEdited(QString);

protected:
    void apply() override;
    void setValue(QString val) override;
    void resetValue() override;
    bool isValueChanged() override;
};

class RepeatDateEditor : public RepeatEditor {
    Q_OBJECT
public:
    RepeatDateEditor(VInfo_ptr, QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotValueEdited(QString);

protected:
    void apply() override;
    void setValue(QString val) override;
    void resetValue() override;
    bool isValueChanged() override;
};

class RepeatDateTimeEditor : public RepeatEditor {
    Q_OBJECT
public:
    explicit RepeatDateTimeEditor(VInfo_ptr, QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotValueEdited(QString);

protected:
    void apply() override;
    void setValue(QString val) override;
    void resetValue() override;
    bool isValueChanged() override;
};

#endif /* ecflow_viewer_RepeatEditor_HPP */

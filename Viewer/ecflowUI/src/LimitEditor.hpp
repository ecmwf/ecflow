/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_LimitEditor_HPP
#define ecflow_viewer_LimitEditor_HPP

#include <QStringList>

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_LimitEditorWidget.h"

class LimitEditor;
class QStringListModel;

class LimitEditorWidget : public QWidget, protected Ui::LimitEditorWidget {
    friend class LimitEditor;

public:
    explicit LimitEditorWidget(QWidget* parent = nullptr);
};

class LimitEditor : public AttributeEditor {
    Q_OBJECT
public:
    explicit LimitEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~LimitEditor() override;

protected Q_SLOTS:
    void slotMaxChanged(int);
    void slotRemove();
    void slotRemoveAll();
    void slotKill();
    void slotSelection(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/);
    void slotLookUp();
    void slotDoubleClicked(const QModelIndex& index);

protected:
    void resetValue() override;
    void apply() override;
    bool isValueChanged() override;
    void buildList(VAttribute* a);
    void remove(bool all);
    void nodeChanged(const std::vector<ecf::Aspect::Type>& a) override;
    void setModelData(QStringList lst);
    void lookup(const QModelIndex& index);
    void readSettings();
    void writeSettings();

    LimitEditorWidget* w_;
    int oriVal_;
    int oriMax_;
    QStringListModel* model_;
    QStringList modelData_;
};

#endif /* ecflow_viewer_LimitEditor_HPP */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_EventEditor_HPP
#define ecflow_viewer_EventEditor_HPP

#include <QStringList>

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_EventEditorWidget.h"

class EventEditor;
class QStringListModel;

class EventEditorWidget : public QWidget, protected Ui::EventEditorWidget {
    friend class EventEditor;

public:
    explicit EventEditorWidget(QWidget* parent = nullptr);
};

class EventEditor : public AttributeEditor {
    Q_OBJECT
public:
    explicit EventEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~EventEditor() override;

protected Q_SLOTS:
    void slotLookUp();
    void slotDoubleClicked(const QModelIndex& index);
    void slotStatusToggled(bool);
    void scanStarted();
    void scanFinished();
    void scanProgressed(int);
    void buildList();

protected:
    void paintEvent(QPaintEvent* e) override;
    void resetValue() override;
    void apply() override;
    bool isValueChanged() override;
    void nodeChanged(const std::vector<ecf::Aspect::Type>& a) override;
    void setModelData(QStringList lst);
    void lookup(const QModelIndex& index);
    void readSettings();
    void writeSettings();

    EventEditorWidget* w_;
    int oriVal_;
    QStringListModel* model_;
    QStringList modelData_;
    bool scanned_;
};

#endif /* ecflow_viewer_EventEditor_HPP */

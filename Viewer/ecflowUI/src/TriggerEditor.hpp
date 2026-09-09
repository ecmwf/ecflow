/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerEditor_HPP
#define ecflow_viewer_TriggerEditor_HPP

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_TriggerEditorWidget.h"

class LabelEditor;

class TriggerEditorWidget : public QWidget, protected Ui::TriggerEditorWidget {
    friend class TriggerEditor;

public:
    explicit TriggerEditorWidget(QWidget* parent = nullptr);
};

class TriggerEditor : public AttributeEditor {
    Q_OBJECT
public:
    explicit TriggerEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~TriggerEditor() override;

protected Q_SLOTS:
    void slotValueChanged();

protected:
    void apply() override;
    void resetValue() override;
    bool isValueChanged() override;
    void readSettings();
    void writeSettings();

    TriggerEditorWidget* w_;
    QString oriText_;
    QString typeInCmd_;
};

#endif /* ecflow_viewer_TriggerEditor_HPP */

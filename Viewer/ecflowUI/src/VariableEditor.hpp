/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VariableEditor_HPP
#define ecflow_viewer_VariableEditor_HPP

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_VariableEditorWidget.h"

class VariableEditor;

class VariableEditorWidget : public QWidget, protected Ui::VariableEditorWidget {
    friend class VariableEditor;

public:
    explicit VariableEditorWidget(QWidget* parent = nullptr);
};

class VariableEditor : public AttributeEditor {
    Q_OBJECT
public:
    explicit VariableEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~VariableEditor() override;

protected Q_SLOTS:
    void slotValueChanged();

protected:
    void apply() override;
    void resetValue() override;
    bool isValueChanged() override;
    void readSettings();
    void writeSettings();

    VariableEditorWidget* w_;
    QString oriVal_;
    bool readOnly_;
};

#endif /* ecflow_viewer_VariableEditor_HPP */

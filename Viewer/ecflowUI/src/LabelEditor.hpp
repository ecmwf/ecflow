/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_LabelEditor_HPP
#define ecflow_viewer_LabelEditor_HPP

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_LabelEditorWidget.h"

class LabelEditor;

class LabelEditorWidget : public QWidget, protected Ui::LabelEditorWidget {
    friend class LabelEditor;
    friend class AddLabelDialog;

public:
    explicit LabelEditorWidget(QWidget* parent = nullptr);
};

class LabelEditor : public AttributeEditor {
    Q_OBJECT
public:
    explicit LabelEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~LabelEditor() override;

protected Q_SLOTS:
    void slotValueChanged();

protected:
    void apply() override;
    void resetValue() override;
    bool isValueChanged() override;
    void readSettings();
    void writeSettings();

    LabelEditorWidget* w_;
    QString oriVal_;
};

#endif /* ecflow_viewer_LabelEditor_HPP */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_MeterEditor_HPP
#define ecflow_viewer_MeterEditor_HPP

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_MeterEditorWidget.h"

class MeterEditor;

class MeterEditorWidget : public QWidget, protected Ui::MeterEditorWidget {
    friend class MeterEditor;

public:
    explicit MeterEditorWidget(QWidget* parent = nullptr);
};

class MeterEditor : public AttributeEditor {
    Q_OBJECT

public:
    explicit MeterEditor(VInfo_ptr, QWidget* parent = nullptr);
    ~MeterEditor() override;

protected Q_SLOTS:
    void slotValueChanged(int);

protected:
    void apply() override;
    void resetValue() override;
    bool isValueChanged() override;
    void readSettings();
    void writeSettings();

    MeterEditorWidget* w_;
    int oriVal_;
};

#endif /* ecflow_viewer_MeterEditor_HPP */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CustomCommandDialog_HPP
#define ecflow_viewer_CustomCommandDialog_HPP

#include <QDialog>

#include "ui_CustomCommandDialog.h"

class CustomCommandDialog : public QDialog, private Ui::CustomCommandDialog {
    Q_OBJECT

public:
    explicit CustomCommandDialog(QWidget* parent = nullptr);
    ~CustomCommandDialog() override = default;

    MenuItem& menuItem() { return commandDesigner_->menuItem(); }
    void setNodes(const std::vector<VInfo_ptr>& nodes) { commandDesigner_->setNodes(nodes); }
    const std::vector<VInfo_ptr>& selectedNodes() { return commandDesigner_->selectedNodes(); }
};

#endif /* ecflow_viewer_CustomCommandDialog_HPP */

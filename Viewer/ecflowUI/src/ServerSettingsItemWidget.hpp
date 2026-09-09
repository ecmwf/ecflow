/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerSettingsItemWidget_HPP
#define ecflow_viewer_ServerSettingsItemWidget_HPP

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "PropertyEditor.hpp"
#include "VInfo.hpp"
#include "ui_ServerSettingsItemWidget.h"

class QAbstractButton;

class VNode;

class ServerSettingsItemWidget : public QWidget, public InfoPanelItem, protected Ui::ServerSettingsItemWidget {
    Q_OBJECT

public:
    explicit ServerSettingsItemWidget(QWidget* parent = nullptr);

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void slotClicked(QAbstractButton* button);
    void slotEditorChanged();

protected:
    void updateState(const ChangeFlags&) override;
};

#endif /* ecflow_viewer_ServerSettingsItemWidget_HPP */

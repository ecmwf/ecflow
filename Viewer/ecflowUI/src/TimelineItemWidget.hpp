/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelineItemWidget_HPP
#define ecflow_viewer_TimelineItemWidget_HPP

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VNode;
class TimelineWidget;

class TimelineItemWidget : public QWidget, public InfoPanelItem {
public:
    explicit TimelineItemWidget(QWidget* parent = nullptr);
    ~TimelineItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;
    bool hasSameContents(VInfo_ptr info) override;
    void notifyInfoChanged(const std::string& path) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    void writeSettings(VComboSettings* vs) override;
    void readSettings(VComboSettings* vs) override;

protected:
    void updateState(const ChangeFlags&) override;
    void serverSyncFinished() override;
    void connectStateChanged() override;

private:
    void load();

    TimelineWidget* w_;
    bool delayedLoad_;
};

#endif /* ecflow_viewer_TimelineItemWidget_HPP */

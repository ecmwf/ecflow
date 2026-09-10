/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OverviewItemWidget_HPP
#define ecflow_viewer_OverviewItemWidget_HPP

#include "CodeItemWidget.hpp"
#include "InfoPanelItem.hpp"

class OverviewItemWidget : public CodeItemWidget, public InfoPanelItem {
public:
    explicit OverviewItemWidget(QWidget* parent = nullptr);
    ~OverviewItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    // From VInfoPresenter
    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;
    void infoProgress(VReply*) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override;
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override;
    void connectStateChanged() override;

protected:
    void reload();
    void updateState(const ChangeFlags&) override;
    void reloadRequested() override;

    int lastScrollPos_{0};
};

#endif /* ecflow_viewer_OverviewItemWidget_HPP */

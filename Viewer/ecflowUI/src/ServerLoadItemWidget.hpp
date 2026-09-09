/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerLoadItemWidget_HPP
#define ecflow_viewer_ServerLoadItemWidget_HPP

#include <QPlainTextEdit>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VNode;
class LogLoadWidget;
class MessageLabel;

class ServerLoadItemWidget : public QWidget, public InfoPanelItem {
public:
    explicit ServerLoadItemWidget(QWidget* parent = nullptr);
    ~ServerLoadItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;
    bool hasSameContents(VInfo_ptr info) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    void readSettings(VComboSettings* vs) override;
    void writeSettings(VComboSettings* vs) override;

protected:
    void updateState(const ChangeFlags&) override;
    void serverSyncFinished() override;
    void connectStateChanged() override;

private:
    void load();
#ifdef ECFLOW_LOGVIEW
    LogLoadWidget* w_;
    bool delayedLoad_{false};
#else
    MessageLabel* w_;
#endif
};

#endif /* ecflow_viewer_ServerLoadItemWidget_HPP */

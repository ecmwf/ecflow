/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_HistoryItemWidget_HPP
#define ecflow_viewer_HistoryItemWidget_HPP

#include "InfoPanelItem.hpp"
#include "ServerHandler.hpp"
#include "VInfo.hpp"
#include "ui_HistoryItemWidget.h"

class LogModel;

class HistoryItemWidget : public QWidget, public InfoPanelItem, protected Ui::HistoryItemWidget {
    Q_OBJECT

public:
    explicit HistoryItemWidget(QWidget* parent = nullptr);

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;
    void infoProgress(VReply*) override;
    void infoAppended(VReply*) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void on_reloadTb__clicked(bool);
    void on_actionCopyEntry__triggered();
    void on_actionCopyRow__triggered();

protected:
    void updateState(const ChangeFlags&) override;
    void adjustColumnSize();
    void checkActionState();
    void toClipboard(QString txt) const;

    LogModel* model_;
};

#endif /* ecflow_viewer_HistoryItemWidget_HPP */

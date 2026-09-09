/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_MessageItemWidget_HPP
#define ecflow_viewer_MessageItemWidget_HPP

#include "InfoPanelItem.hpp"
#include "ui_MessageItemWidget.h"

class LogModel;

class MessageItemWidget : public QWidget, public InfoPanelItem, protected Ui::MessageItemWidget {
    Q_OBJECT
public:
    explicit MessageItemWidget(QWidget* parent = nullptr);

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    // From VInfoPresenter
    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;
    void infoProgress(VReply*) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void on_actionCopyEntry__triggered();
    void on_actionCopyRow__triggered();

protected:
    void updateState(const ChangeFlags&) override {}
    void toClipboard(QString txt) const;

    LogModel* model_;
};

#endif /* ecflow_viewer_MessageItemWidget_HPP */

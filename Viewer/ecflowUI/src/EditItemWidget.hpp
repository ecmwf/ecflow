/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_EditItemWidget_HPP
#define ecflow_viewer_EditItemWidget_HPP

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"
#include "ui_EditItemWidget.h"

class EditItemWidget : public QWidget, public InfoPanelItem, protected Ui::EditItemWidget {
    Q_OBJECT

public:
    explicit EditItemWidget(QWidget* parent = nullptr);

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
    void on_preprocTb__toggled(bool);
    void on_submitTb__clicked(bool);
    void on_searchTb__clicked();
    void on_gotoLineTb__clicked();
    void on_fontSizeUpTb__clicked();
    void on_fontSizeDownTb__clicked();

protected:
    bool preproc() const;
    bool alias() const;
    void updateState(const ChangeFlags&) override {}
};

#endif /* ecflow_viewer_EditItemWidget_HPP */

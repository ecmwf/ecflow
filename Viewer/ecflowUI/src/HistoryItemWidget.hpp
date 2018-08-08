//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef HISTORYITEMWIDGET_HPP_
#define HISTORYITEMWIDGET_HPP_

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include "ServerHandler.hpp"

#include "ui_HistoryItemWidget.h"

class LogModel;

class HistoryItemWidget :  public QWidget, public InfoPanelItem, protected Ui::HistoryItemWidget
{
Q_OBJECT

public:
	explicit HistoryItemWidget(QWidget *parent=nullptr);

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

#endif

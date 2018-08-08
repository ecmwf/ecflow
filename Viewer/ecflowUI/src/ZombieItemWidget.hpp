//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ZOMBIEITEMWIDGET_HPP_
#define ZOMBIEITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include "ui_ZombieItemWidget.h"

class ZombieModel;
class QSortFilterProxyModel;

class ZombieItemWidget : public QWidget, public InfoPanelItem, protected Ui::ZombieItemWidget
{
Q_OBJECT

public:
	explicit ZombieItemWidget(QWidget *parent=nullptr);
    ~ZombieItemWidget() override;
	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
    void clearContents() override;
    bool hasSameContents(VInfo_ptr info) override;

	//From VInfoPresenter
	void infoReady(VReply*) override;
	void infoFailed(VReply*) override;
    void infoProgress(VReply*) override {}

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
	void on_actionTerminate_triggered();
	void on_actionRescue_triggered();
	void on_actionFoboff_triggered();
	void on_actionDelete_triggered();
    void on_actionKill_triggered();
	void on_reloadTb__clicked(bool);
    void on_actionLookup_triggered();
    void slotDoubleClicked(const QModelIndex &index);
    void slotItemSelected(QModelIndex,QModelIndex);

protected:
    void updateState(const ChangeFlags&) override;
	void serverSyncFinished() override;

private:
	void command(const std::string& cmdName);
    void lookup(const QModelIndex& idx);
    void updateContents();
	void checkActionState();

	ZombieModel *model_;
	QSortFilterProxyModel* sortModel_;
	bool commandSent_;
	QStringList lastSelection_;
};

#endif


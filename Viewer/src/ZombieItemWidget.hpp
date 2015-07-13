//============================================================================
// Copyright 2014 ECMWF.
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

class ZombieItemWidget : public QWidget, public InfoPanelItem, protected Ui::ZombieItemWidget
{
Q_OBJECT

public:
	explicit ZombieItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	//From VInfoPresenter
	void infoReady(VReply*);
	void infoFailed(VReply*);
	void infoProgress(VReply*) {};

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected Q_SLOTS:
	void on_terminateTb_toggled(bool);
	void on_rescueTb_toggled(bool);
	void on_foboffTb_clicked(bool);
	void on_deleteTb_clicked(bool);
	void on_killTb_clicked(bool);

protected:
	void updateWidgetState() {};

private:
	void command(const std::string& cmdName);

	ZombieModel *model_;
};

#endif


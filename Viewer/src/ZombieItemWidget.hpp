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

#include "ui_SuiteItemWidget.h"

//class ZombieModel;

class ZombieItemWidget : public QWidget, public InfoPanelItem, protected Ui::SuiteItemWidget
{
Q_OBJECT

public:
	ZombieItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	//From VInfoPresenter
	void infoReady(VReply*) {};
	void infoFailed(VReply*) {};
	void infoProgress(VReply*) {};

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected Q_SLOTS:
	void on_autoTb_toggled(bool);
	void on_enableTb_toggled(bool);
	void on_okTb_clicked(bool);

protected:
	//ZombieModel *model_;
};

#endif


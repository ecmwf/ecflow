//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTITEMWIDGET_HPP_
#define OUTPUTITEMWIDGET_HPP_

#include "InfoPanelItem.hpp"

#include "VDir.hpp"

#include "ui_OutputItemWidget.h"

class OutputModel;
class OutputSortModel;

class OutputItemWidget : public QWidget, public InfoPanelItem, protected Ui::OutputItemWidget
{
Q_OBJECT

public:
	explicit OutputItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	//From VInfoPresenter
	void infoReady(VReply*);
	void infoFailed(VReply*);
	void infoProgress(VReply*);

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected Q_SLOTS:
	void slotOutputSelected(QModelIndex,QModelIndex);
	void on_searchTb__toggled(bool b);

protected:
	void updateDir(VDir_ptr dir);
	void enableDir(bool);
	void updateWidgetState() {};

	OutputModel* dirModel_;
	OutputSortModel* dirSortModel_;

};

#endif


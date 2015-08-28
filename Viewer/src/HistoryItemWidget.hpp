//============================================================================
// Copyright 2014 ECMWF.
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
public:
	explicit HistoryItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	//From VInfoPresenter
	 //From VInfoPresenter
    void infoReady(VReply*);
    void infoFailed(VReply*);
    void infoProgress(VReply*);

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected:
	void updateWidgetState() {};

	LogModel* model_;

};

#endif

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

class Highlighter;
class OutputModel;
class OutputSortModel;
class QTime;

class OutputItemWidget : public QWidget, public InfoPanelItem, protected Ui::OutputItemWidget
{
Q_OBJECT

public:
	explicit OutputItemWidget(QWidget *parent=0);
	~OutputItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	//From VInfoPresenter
	void infoReady(VReply*);
	void infoFailed(VReply*);
	void infoProgress(VReply*);

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

	bool automaticSearchForKeywords();

protected Q_SLOTS:
	void slotOutputSelected(QModelIndex,QModelIndex);
	void slotUpdateDir();
	void on_searchTb__toggled(bool b);
	void on_gotoLineTb__clicked();
	void on_reloadTb__clicked();

protected:
	void updateDir(bool);
	void updateDir(bool,const std::string&);
	void enableDir(bool);
	void updateWidgetState() {};
	void searchOnReload();
	void getCurrentFile();
	void getLatestFile();
	std::string currentFullName() const;

	OutputModel* dirModel_;
	OutputSortModel* dirSortModel_;
	bool userClickedReload_;
	bool ignoreOutputSelection_;
	QTimer* updateDirTimer_;
	Highlighter* jobHighlighter_;
	static int updateDirTimeout_;

};

#endif


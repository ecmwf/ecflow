//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef DASHBOARDDIALOG_HPP_
#define DASHBOARDDIALOG_HPP_

#include <QDialog>

#include "ui_DashboardDialog.h"

class DashboardWidget;

class DashboardDialog : public QDialog, protected Ui::DashboardDialog
{

public:
	explicit DashboardDialog(QWidget *parent=0);

	void add(DashboardWidget*);
	DashboardWidget* dashboardWidget() const {return dw_;}

protected:
	void closeEvent(QCloseEvent * event);

	DashboardWidget* dw_;

};

#endif

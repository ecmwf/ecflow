//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef CHANGENOTIFYDIALOG_HPP_
#define CHNAGENOTIFYDIALOG_HPP_

#include <QDialog>

#include <map>

#include "ui_ChangeNotifyDialog.h"

class ChangeNotifyModel;
class VProperty;

class ChangeNotifyDialog : public QDialog, protected Ui::ChangeNotifyDialog
{
Q_OBJECT

public:
	explicit ChangeNotifyDialog(QWidget *parent=0);

	void init(VProperty*,ChangeNotifyModel*);
	void addTab(const std::string id,VProperty* prop, ChangeNotifyModel* model);
	void setCurrentTab(const std::string&);

public Q_SLOTS:
	void on_closePb__clicked(bool b);
	void on_clearCPb__clicked(bool b);

protected:
	ChangeNotifyModel* model_;
	std::map<std::string,int> tabMap_;
};

#endif

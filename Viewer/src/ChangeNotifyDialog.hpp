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

#include "ui_ChangeNotifyDialog.h"

class ChangeNotifyModel;
class VProperty;

class ChangeNotifyDialog : public QDialog, protected Ui::ChangeNotifyDialog
{
Q_OBJECT

public:
	explicit ChangeNotifyDialog(QWidget *parent=0);

public:
	void init(VProperty*,ChangeNotifyModel*);

protected Q_SLOTS:
	void on_closePb__clicked(bool b);
	void on_clearCPb__clicked(bool b);

protected:
	ChangeNotifyModel* model_;
};



#endif

//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef CUSTOMCOMMANDDIALOG_HPP_
#define CUSTOMCOMMANDDIALOG_HPP_

#include <QDialog>

#include "ui_CustomCommandDialog.h"

class CustomCommandDialog : public QDialog, private Ui::CustomCommandDialog
{
	Q_OBJECT

public:
	explicit CustomCommandDialog(QWidget *parent = 0);
	~CustomCommandDialog() {};

    QString command() {return commandDesigner_->command();};
	void setNodes(std::vector<VInfo_ptr> nodes) {commandDesigner_->setNodes(nodes);};



//public Q_SLOTS:
//	void insertCurrentText();
};

#endif

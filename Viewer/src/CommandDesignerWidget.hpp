//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef COMMANDDESIGNERWIDGET_HPP_
#define COMMANDDESIGNERWIDGET_HPP_

#include <QDialog>

#include "ui_CommandDesignerWidget.h"

class CommandDesignerWidget : public QWidget, private Ui::commandDesignerWidget
{
	Q_OBJECT

public:
	CommandDesignerWidget(QWidget *parent = 0);
	~CommandDesignerWidget() {};

	QString command() {return commandLineEdit_->text();};


public slots:
	void insertCurrentText();
};

#endif

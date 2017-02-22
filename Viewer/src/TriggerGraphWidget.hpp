//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef TRIGGERGRAPHWIDGET_HPP_
#define TRIGGERGRAPHWIDGET_HPP_

#include <QDialog>

#include "ui_TriggerGraphWidget.h"
#include "TriggerGraphModel.hpp"

class TriggerGraphWidget : public QWidget, private Ui::triggerGraphWidget
{
	Q_OBJECT

public:
	explicit TriggerGraphWidget(QWidget *parent = 0);
	~TriggerGraphWidget();

	void setTriggerExpression(std::string &exp);
	void setTriggerCollector(TriggerListCollector *tc) {model_->setTriggerCollector(tc);}
	void clear();
	void beginUpdate();
	void endUpdate();


public Q_SLOTS:


private:

	TriggerGraphModel *model_;
};


#endif

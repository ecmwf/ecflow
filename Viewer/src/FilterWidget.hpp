//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef FILTERWIDGET_HPP_
#define FILTERWIDGET_HPP_

#include <QMap>
#include <QSet>
#include <QWidget>

#include "DState.hpp"

class QToolButton;

class FilterWidget : public QWidget
{
Q_OBJECT

public:
	FilterWidget(QWidget* parent=0);

protected slots:
	void slotChanged(bool);

signals:
	void filterChanged(QSet<DState::State>);

private:
	QToolButton* createButton(QString,QString,QColor);

	QMap<DState::State,QToolButton*> items_;
};

#endif

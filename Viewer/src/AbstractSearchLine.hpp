//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef ABSTRACTSEARCHLINE_HPP_
#define ABSTRACTSEARCHLINE_HPP_

#include "ui_SearchLineWidget.h"

#include <QWidget>

class AbstractSearchLine : public QWidget, protected Ui::SearchLineWidget
{
   Q_OBJECT

public:
	explicit AbstractSearchLine(QWidget *parent=0);
	~AbstractSearchLine();
	virtual void clear();
	virtual bool isEmpty();

	bool caseSensitive() {return caseSensitive_;};
	bool wholeWords()    {return wholeWords_;};

public Q_SLOTS:
	virtual void slotFind(QString)=0;
	virtual void slotFindNext()=0;
	virtual void slotFindPrev()=0;
	void on_actionCaseSensitive__toggled(bool);
	void on_actionWholeWords__toggled(bool);

protected:
	void updateButtons(bool);

	bool status_;
	bool caseSensitive_;
	bool wholeWords_;

	QColor   oriColour_;
	QColor   redColour_;
	QColor   greenColour_;
};

#endif

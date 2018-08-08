//============================================================================
// Copyright 2009-2017 ECMWF.
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
	explicit AbstractSearchLine(QWidget *parent=nullptr);
	virtual ~AbstractSearchLine();
	virtual void clear();
	virtual bool isEmpty();
	void selectAll();
    void setConfirmSearch(bool);
    bool confirmSearch() const {return confirmSearch_;}
    QString confirmSearchText() const;

    bool caseSensitive()  {return caseSensitive_;}
    bool wholeWords()     {return wholeWords_;}
    bool highlightAll()   {return highlightAll_;}

public Q_SLOTS:
	virtual void slotFind(QString)=0;
	virtual void slotFindNext()=0;
	virtual void slotFindPrev()=0;
	virtual void slotClose();
	virtual void on_actionCaseSensitive__toggled(bool);
	virtual void on_actionWholeWords__toggled(bool);
	virtual void on_actionHighlightAll__toggled(bool);

Q_SIGNALS:
	void visibilityChanged();

protected:
	void updateButtons(bool);
	void toDefaultState();
	void hideEvent(QHideEvent* event);
	void showEvent(QShowEvent* event);

	bool status_;
	bool caseSensitive_;
	bool wholeWords_;
	bool highlightAll_;
    StringMatchMode matchMode_;

    QBrush oriBrush_;
    QBrush redBrush_;
    QBrush greenBrush_;

	bool confirmSearch_;
};

#endif

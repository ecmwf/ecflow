//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef PLAINTEXTSEARCHLINE_HPP_
#define PLAINTEXTSEARCHLINE_HPP_

#include <QPlainTextEdit>

#include "TextEditSearchLine.hpp"

class AbstractTextSearchInterface;

class PlainTextSearchLine : public TextEditSearchLine
{
public:
	explicit PlainTextSearchLine(QWidget *parent=nullptr);
	~PlainTextSearchLine();
	void setEditor(QPlainTextEdit*);

private:
	//The interface is set internally
    void setSearchInterface(AbstractTextSearchInterface*) {}

};


#if 0
class PlainTextSearchLine : public AbstractSearchLine
{
	Q_OBJECT

public:
	 explicit PlainTextSearchLine(QWidget *parent);
	~PlainTextSearchLine();
	void setEditor(QPlainTextEdit*);

public Q_SLOTS:
	void slotFind(QString);
	void slotFindNext();
	void slotFindPrev();
	void slotFindNext(bool) {slotFindNext();}
	void slotFindPrev(bool) {slotFindPrev();}
	void matchModeChanged(int newIndex);
	void on_actionCaseSensitive__toggled(bool);
	void on_actionWholeWords__toggled(bool);
	void on_actionHighlightAll__toggled(bool);
	void slotClose();
	void slotHighlight();

protected:
	QTextDocument::FindFlags findFlags();
	bool findString (QString str, bool highlightAll, QTextDocument::FindFlags extraFlags, bool gotoStartOfWord, int iteration);
	void refreshSearch();
	void highlightMatches(QString txt);
    void clearHighlights();
    QTimer highlightAllTimer_;
	QPlainTextEdit* editor_;
	QColor highlightColour_;
};
#endif


#endif

//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_TEXTEDITSEARCHLINE_HPP_
#define VIEWER_SRC_TEXTEDITSEARCHLINE_HPP_

#include <QTextDocument>
#include <QTimer>

#include "AbstractSearchLine.hpp"

class AbstractTextEditSearchInterface;

class TextEditSearchLine : public AbstractSearchLine
{
	Q_OBJECT

public:
	 explicit TextEditSearchLine(QWidget *parent);
	~TextEditSearchLine();
	void setSearchInterface(AbstractTextEditSearchInterface*);
	void searchOnReload(bool userClickedReload);

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

    AbstractTextEditSearchInterface* interface_;
    QTimer highlightAllTimer_;
	QColor highlightColour_;
};

#endif /* VIEWER_SRC_TEXTEDITSEARCHLINE_HPP_ */

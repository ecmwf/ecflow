//============================================================================
// Copyright 2009-2017 ECMWF.
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
	~TextEditSearchLine() override;
	void setSearchInterface(AbstractTextEditSearchInterface*);
	void searchOnReload(bool userClickedReload);
    bool hasInterface() const {return interface_ != nullptr;}

public Q_SLOTS:
	void slotFind(QString) override;
	void slotFindNext() override;
	void slotFindPrev() override;
	void slotFindNext(bool) {slotFindNext();}
	void slotFindPrev(bool) {slotFindPrev();}
	void matchModeChanged(int newIndex);
	void on_actionCaseSensitive__toggled(bool) override;
	void on_actionWholeWords__toggled(bool) override;
	void on_actionHighlightAll__toggled(bool) override;
	void slotClose() override;
	void slotHighlight();

protected:
	QTextDocument::FindFlags findFlags();
	bool findString (QString str, bool highlightAll, QTextDocument::FindFlags extraFlags, QTextCursor::MoveOperation move, int iteration);
	void refreshSearch();
	void highlightMatches(QString txt);
    void clearHighlights();
    void disableHighlights();
    bool lastFindSuccessful() {return lastFindSuccessful_;}

    AbstractTextEditSearchInterface* interface_;
    QTimer highlightAllTimer_;
	QColor highlightColour_;
	bool lastFindSuccessful_;
};

#endif /* VIEWER_SRC_TEXTEDITSEARCHLINE_HPP_ */

/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_PlainTextSearchLine_HPP
#define ecflow_viewer_PlainTextSearchLine_HPP

#include <QPlainTextEdit>

#include "TextEditSearchLine.hpp"

class AbstractTextSearchInterface;

class PlainTextSearchLine : public TextEditSearchLine {
public:
    explicit PlainTextSearchLine(QWidget* parent = nullptr);
    ~PlainTextSearchLine() override;
    void setEditor(QPlainTextEdit*);

private:
    // The interface is set internally
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

#endif /* ecflow_viewer_PlainTextSearchLine_HPP */

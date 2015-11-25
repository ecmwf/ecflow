//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef PLAINTEXTSEARCHLINE_HPP_
#define PLAINTEXTSEARCHLINE_HPP_

#include <QPlainTextEdit>

#include "AbstractSearchLine.hpp"

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

protected:
	QTextDocument::FindFlags findFlags();
	bool findString (QString str, QTextDocument::FindFlags extraFlags, bool gotoStartOfWord, int iteration);
	QPlainTextEdit* editor_;

};

#endif
